#!/usr/bin/env python3
"""Generates the rolling IBOVESPA instances for mopop from a price cache.

The script has two subcommands and the split between them is deliberate:

  fetch   The only code path that touches the network. Downloads the adjusted
          close series of every ticker that appears in
          ibovespa_tickers_2011_2025/ over one maximal span and stores one CSV
          per ticker under cache/prices/.

  build   Reads cache/prices/ and nothing else. There is no network code path
          here: a missing ticker is an error, never an implicit download. This
          is what makes regenerating the instances fully offline.

The cache is the reproducibility guarantee. Yahoo Finance re-adjusts its
auto_adjust close series retroactively whenever a new corporate action occurs,
so re-issuing the same request at a later date returns different numbers. The
committed cache is the only thing that pins the instances, and through them
every metric and every published result.

All statistics are DAILY: the mean and covariance of daily returns, with no
annualization applied anywhere.
"""

import argparse
import csv
import hashlib
import json
import logging
import os
import re
import shutil
import sys
import time
from typing import Dict, List, Optional, Sequence, Tuple

import pandas as pd

# --- Paths and constants ---

REPO_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
TICKERS_DIR = os.path.join(REPO_ROOT, "ibovespa_tickers_2011_2025")
CACHE_DIR = os.path.join(REPO_ROOT, "cache")
PRICES_DIR = os.path.join(CACHE_DIR, "prices")
MANIFEST_PATH = os.path.join(CACHE_DIR, "manifest.json")
INSTANCES_DIR = os.path.join(REPO_ROOT, "instances")

# One maximal span covering every training and out-of-sample window. The left
# margin exists so that the first window (ibov_2011, training from 2011-01-01)
# still has a trading day before its start to seed the first return.
CACHE_START = "2010-12-01"
CACHE_END = "2026-01-01"

# ibov_2011 .. ibov_2020, each with a 5-year training window followed by a
# 1-year out-of-sample window.
INSTANCE_YEARS = list(range(2011, 2021))
TRAIN_YEARS = 5
OOS_YEARS = 1

# B3 tickers are a 4-character root plus a share-class digit, with Yahoo's .SA
# suffix. The root is alphanumeric, not purely alphabetic: B3SA3.SA is real.
TICKER_PATTERN = re.compile(r"^[A-Z0-9]{4}\d{1,2}\.SA$")

# A ticker is kept in an instance only if it has at least this fraction of the
# window's trading days and has data at both ends of the window. The endpoint
# test is what catches constituents delisted or renamed mid-window, which a
# percentage alone can pass.
DEFAULT_MIN_COVERAGE = 0.95
ENDPOINT_WINDOW_DAYS = 5

# A date counts as a trading day for a window if at least this fraction of the
# window's candidate tickers quote a price on it. Deriving the calendar from a
# quorum rather than from the union of all observed dates keeps a single
# misbehaving series from inventing trading days.
CALENDAR_QUORUM = 0.5

EXPECTED_RETURNS_FILENAME = "expected_returns.csv"
COVARIANCE_FILENAME = "covariance_matrix.csv"
EXPECTED_RETURNS_COLUMN = "ExpectedDailyReturn"


def setup_logging() -> None:
  """Configures basic logging for the script."""
  logging.basicConfig(
      level=logging.INFO,
      format="%(asctime)s - %(levelname)s - %(message)s",
      datefmt="%Y-%m-%d %H:%M:%S",
  )


def sha256_file(file_path: str) -> str:
  """Returns the hex SHA-256 digest of a file."""
  digest = hashlib.sha256()
  with open(file_path, "rb") as handle:
    for chunk in iter(lambda: handle.read(1 << 20), b""):
      digest.update(chunk)
  return digest.hexdigest()


# --- Ticker files ---


def load_tickers(file_path: str) -> List[str]:
  """
  Loads the ticker symbols of one yearly constituent file.

  The files carry a header `ticker,company` and the company column contains
  commas, so it must be parsed as real CSV rather than split on commas.

  @raises ValueError If the file lacks a `ticker` column, is empty, contains
  duplicates, or holds a symbol that is not a valid Yahoo Finance B3 ticker.
  """
  with open(file_path, newline="", encoding="utf-8") as handle:
    reader = csv.DictReader(handle)
    if reader.fieldnames is None or "ticker" not in reader.fieldnames:
      raise ValueError(
          f"{file_path}: expected a 'ticker' column, found "
          f"{reader.fieldnames}")
    tickers = [row["ticker"].strip() for row in reader if row.get("ticker")]

  if not tickers:
    raise ValueError(f"{file_path}: no tickers found")

  duplicates = sorted({t for t in tickers if tickers.count(t) > 1})
  if duplicates:
    raise ValueError(f"{file_path}: duplicate tickers {duplicates}")

  invalid = [t for t in tickers if not TICKER_PATTERN.match(t)]
  if invalid:
    raise ValueError(f"{file_path}: malformed ticker symbols {invalid}")

  return sorted(tickers)


def tickers_file(year: int) -> str:
  """Returns the path of the constituent file of the given year."""
  return os.path.join(TICKERS_DIR, f"tickers_{year}.csv")


def load_all_tickers() -> List[str]:
  """Returns the sorted union of every ticker in every constituent file."""
  union = set()
  for name in sorted(os.listdir(TICKERS_DIR)):
    if name.startswith("tickers_") and name.endswith(".csv"):
      union.update(load_tickers(os.path.join(TICKERS_DIR, name)))
  return sorted(union)


# --- Cache ---


def cache_path(ticker: str) -> str:
  """Returns the cache file path of a ticker."""
  return os.path.join(PRICES_DIR, f"{ticker}.csv")


def read_manifest() -> Dict:
  """Reads the cache manifest, returning an empty one if it does not exist."""
  if not os.path.exists(MANIFEST_PATH):
    return {"span": {"start": CACHE_START, "end": CACHE_END}, "tickers": {}}
  with open(MANIFEST_PATH, encoding="utf-8") as handle:
    return json.load(handle)


def write_manifest(manifest: Dict) -> None:
  """Writes the cache manifest with stable key ordering."""
  with open(MANIFEST_PATH, "w", encoding="utf-8") as handle:
    json.dump(manifest, handle, indent=2, sort_keys=True)
    handle.write("\n")


def write_cached_series(ticker: str, prices: pd.Series) -> None:
  """
  Writes one ticker's price series to the cache.

  The index is written as a plain YYYY-MM-DD date. Timezone-aware stamps would
  make the cache depend on how the local pandas build resolves the exchange
  timezone, which defeats the point of caching.
  """
  frame = prices.to_frame(name="close")
  frame.index.name = "date"
  frame.to_csv(cache_path(ticker), date_format="%Y-%m-%d", lineterminator="\n")


def read_cached_series(ticker: str) -> pd.Series:
  """
  Reads one ticker's price series from the cache.

  @raises FileNotFoundError If the ticker has never been fetched.
  """
  path = cache_path(ticker)
  if not os.path.exists(path):
    raise FileNotFoundError(
        f"{ticker} is not in the price cache ({path}). Run "
        f"`generate_instances.py fetch` first; `build` never downloads.")
  frame = pd.read_csv(path, index_col=0, parse_dates=[0])
  series = frame["close"] if "close" in frame.columns else pd.Series(
      dtype="float64")
  # A header-only file (a ticker Yahoo has no data for) reads back with an
  # object-dtype empty index. Left alone, that degrades the whole wide frame's
  # index from DatetimeIndex to object once the columns are aligned.
  series = pd.Series(series.to_numpy(dtype="float64"),
                     index=pd.DatetimeIndex(series.index), name=ticker)
  return series


def normalize_index(series: pd.Series) -> pd.Series:
  """Reduces a downloaded series to a deduplicated, sorted, date-only index."""
  index = pd.to_datetime(series.index)
  if getattr(index, "tz", None) is not None:
    # Drop the timezone keeping local wall time: Yahoo reports in the exchange
    # timezone, so the local date is the trading date.
    index = index.tz_localize(None)
  series = pd.Series(series.to_numpy(), index=index.normalize())
  series = series[~series.index.duplicated(keep="first")]
  return series.sort_index().dropna()


# --- fetch ---


def download_ticker(ticker: str, start: str, end: str,
                    attempts: int = 3) -> Tuple[Optional[pd.Series], str]:
  """
  Downloads one ticker's adjusted close series from Yahoo Finance.

  @return A (series, status) pair. The status is "ok" when data came back,
  "no_data" when Yahoo answered authoritatively that it has none for the
  symbol, and "error" when every attempt failed in a way that could be
  transient.

  Separating "no_data" from "error" matters more than it looks. Yahoo drops
  delisted, renamed and merged symbols entirely rather than retaining their
  history, so "no_data" is a permanent and expected answer for a large share of
  historical index constituents. A network or throttling failure looks
  identical at the call site but must not be cached as if it were the same
  thing, or a bad afternoon would permanently shrink every instance.
  """
  import yfinance as yf

  for attempt in range(1, attempts + 1):
    try:
      history = yf.Ticker(ticker).history(
          start=start, end=end, interval="1d", auto_adjust=True)
      if history.empty or "Close" not in history.columns:
        return pd.Series(dtype="float64"), "no_data"
      return normalize_index(history["Close"]), "ok"
    except Exception as error:  # noqa: BLE001 - yfinance raises broadly
      # A 404 is Yahoo stating the symbol does not exist, which is as
      # authoritative as an empty response. Anything else may be transient.
      if "404" in str(error):
        return pd.Series(dtype="float64"), "no_data"
      logging.warning(f"{ticker}: attempt {attempt}/{attempts} failed: {error}")
      if attempt < attempts:
        time.sleep(2 ** attempt)
  return None, "error"


def command_fetch(args: argparse.Namespace) -> int:
  """Populates the price cache. This is the only networked step."""
  import yfinance as yf

  os.makedirs(PRICES_DIR, exist_ok=True)
  tickers = args.tickers if args.tickers else load_all_tickers()
  manifest = read_manifest()
  manifest["span"] = {"start": CACHE_START, "end": CACHE_END}
  manifest["yfinance_version"] = yf.__version__
  entries = manifest.setdefault("tickers", {})

  if args.mark_no_data:
    # An explicit, recorded human judgement. Some long-dead symbols fail
    # deterministically inside yfinance rather than returning an empty result,
    # so they never settle on their own. Settling them by hand keeps the
    # automatic path conservative while leaving an auditable note in the
    # committed manifest.
    for ticker in args.mark_no_data:
      write_cached_series(ticker, pd.Series(dtype="float64"))
      entries[ticker] = {
          "status": "no_data",
          "span_start": CACHE_START,
          "span_end": CACHE_END,
          "first_date": None,
          "last_date": None,
          "n_rows": 0,
          "sha256": sha256_file(cache_path(ticker)),
          "fetched_at": time.strftime("%Y-%m-%dT%H:%M:%S%z"),
          "note": args.note or "manually marked as unavailable",
      }
      logging.info(f"{ticker}: marked as permanently unavailable")
    write_manifest(manifest)
    return 0

  failures = []
  for position, ticker in enumerate(tickers, start=1):
    cached = entries.get(ticker)
    # "error" entries are always retried: they record a failure to find out,
    # not a finding. "ok" and "no_data" are settled answers.
    if (cached and not args.refresh
            and cached.get("status") in ("ok", "no_data")
            and cached.get("span_start") == CACHE_START
            and cached.get("span_end") == CACHE_END
            and os.path.exists(cache_path(ticker))):
      logging.info(f"[{position}/{len(tickers)}] {ticker}: cached, skipping")
      continue

    logging.info(f"[{position}/{len(tickers)}] {ticker}: downloading")
    series, status = download_ticker(ticker, CACHE_START, CACHE_END)

    if status == "error":
      # Deliberately leaves any existing cache file untouched.
      logging.error(f"{ticker}: every attempt failed; leaving the cache as is")
      entries[ticker] = dict(cached or {}, status="error")
      failures.append(ticker)
      write_manifest(manifest)
      continue

    if status == "no_data":
      logging.warning(f"{ticker}: Yahoo Finance has no data for this symbol")
      if cached and cached.get("n_rows", 0) > 0:
        # Refuse to trade real history for an empty response. Yahoo can drop a
        # symbol it previously served, and the cache exists precisely so that
        # such a change cannot quietly rewrite the instances.
        logging.error(
            f"{ticker}: Yahoo now returns nothing but {cached['n_rows']} rows "
            f"are already cached. Keeping the cached series; delete "
            f"{cache_path(ticker)} by hand if dropping it is intended.")
        failures.append(ticker)
        continue

    # Empty series are written as a header-only file so the cache stays
    # uniform: `build` then sees a ticker with zero coverage and drops it
    # through the normal screening rule instead of failing on a missing file.
    write_cached_series(ticker, series)
    entries[ticker] = {
        "status": status,
        "span_start": CACHE_START,
        "span_end": CACHE_END,
        "first_date": (series.index[0].strftime("%Y-%m-%d")
                       if not series.empty else None),
        "last_date": (series.index[-1].strftime("%Y-%m-%d")
                      if not series.empty else None),
        "n_rows": int(series.size),
        "sha256": sha256_file(cache_path(ticker)),
        "fetched_at": time.strftime("%Y-%m-%dT%H:%M:%S%z"),
    }
    write_manifest(manifest)
    time.sleep(args.delay)

  write_manifest(manifest)

  empty = sorted(t for t, e in entries.items()
                 if e.get("status") == "no_data")
  logging.info(f"Cache holds {len(entries)} tickers, {len(empty)} of which "
               f"Yahoo Finance no longer serves.")
  if empty:
    logging.info(f"No data available for: {empty}")
  if failures:
    logging.error(f"Unresolved for {len(failures)} tickers: {failures}")
    return 1
  return 0


# --- build ---


def load_prices(tickers: Sequence[str], verify: bool) -> pd.DataFrame:
  """
  Loads the cached price series of the given tickers into one wide frame.

  @param verify Whether to check each file against the digest recorded in the
  cache manifest. A mismatch means the cache was edited or Yahoo restated the
  series, either of which silently changes every downstream result.

  @raises RuntimeError If a checksum does not match.
  """
  manifest = read_manifest()
  entries = manifest.get("tickers", {})
  columns = {}

  # A ticker whose last fetch ended in an error has an unknown history, not an
  # empty one. Building anyway would silently drop it through the coverage rule
  # and produce a smaller instance that looks perfectly valid.
  unresolved = sorted(t for t in tickers
                      if entries.get(t, {}).get("status") == "error")
  if unresolved:
    raise RuntimeError(
        f"{len(unresolved)} tickers are in an unresolved state in "
        f"{MANIFEST_PATH}: {unresolved}. Re-run `fetch` until they settle as "
        f"either 'ok' or 'no_data'; building now would silently drop them.")

  for ticker in tickers:
    series = read_cached_series(ticker)
    if verify:
      recorded = entries.get(ticker, {}).get("sha256")
      if recorded is None:
        raise RuntimeError(
            f"{ticker} has no checksum in {MANIFEST_PATH}; re-run `fetch`.")
      actual = sha256_file(cache_path(ticker))
      if actual != recorded:
        raise RuntimeError(
            f"{ticker}: cache checksum mismatch (manifest {recorded[:12]}…, "
            f"file {actual[:12]}…). The cached prices changed since they were "
            f"recorded; re-run `fetch --refresh` deliberately if that is "
            f"intended.")
    columns[ticker] = series

  prices = pd.DataFrame(columns)
  prices = prices.reindex(sorted(prices.columns), axis=1)
  prices.columns.name = None
  prices.index.name = None
  return prices.sort_index()


def window_bounds(year: int) -> Tuple[pd.Timestamp, pd.Timestamp,
                                      pd.Timestamp]:
  """Returns the (train_start, split, oos_end) timestamps of an instance."""
  train_start = pd.Timestamp(year=year, month=1, day=1)
  split = pd.Timestamp(year=year + TRAIN_YEARS, month=1, day=1)
  oos_end = pd.Timestamp(year=year + TRAIN_YEARS + OOS_YEARS, month=1, day=1)
  return train_start, split, oos_end


def window_slice(prices: pd.DataFrame, start: pd.Timestamp,
                 end: pd.Timestamp) -> pd.DataFrame:
  """Returns the rows of a frame inside the half-open interval [start, end)."""
  return prices[(prices.index >= start) & (prices.index < end)]


def trading_calendar(window: pd.DataFrame) -> pd.DatetimeIndex:
  """
  Derives the trading calendar of a window from a quorum of its tickers.

  A date counts as a trading day when at least CALENDAR_QUORUM of the candidate
  tickers quote a price on it, which keeps one stray series from inventing
  trading days that would then depress everyone else's coverage.
  """
  if window.empty or window.shape[1] == 0:
    return pd.DatetimeIndex([])

  # The quorum counts only tickers that have any data in the window. Including
  # symbols Yahoo Finance no longer serves at all would put the threshold out
  # of reach whenever many constituents are missing, collapsing the calendar to
  # nothing and taking the surviving tickers down with it.
  active = window.columns[window.notna().any()]
  if len(active) == 0:
    return pd.DatetimeIndex([])

  quorum = CALENDAR_QUORUM * len(active)
  return window.index[window[active].notna().sum(axis=1) >= quorum]


def screen_window(window: pd.DataFrame, calendar: pd.DatetimeIndex,
                  min_coverage: float) -> Dict[str, Dict]:
  """
  Scores every ticker of a window against the coverage and endpoint rules.

  Returns one record per ticker holding its coverage, whether it is present at
  each end of the window, and whether it passes.
  """
  scores = {}
  if len(calendar) == 0:
    return {t: {"coverage": 0.0, "observations": 0, "has_start": False,
                "has_end": False, "passes": False} for t in window.columns}

  on_calendar = window.loc[calendar]
  endpoint_days = min(ENDPOINT_WINDOW_DAYS, len(calendar))
  head = on_calendar.iloc[:endpoint_days]
  tail = on_calendar.iloc[-endpoint_days:]

  for ticker in window.columns:
    observations = int(on_calendar[ticker].notna().sum())
    coverage = float(observations) / len(calendar)
    has_start = bool(head[ticker].notna().any())
    has_end = bool(tail[ticker].notna().any())
    scores[ticker] = {
        "coverage": coverage,
        "observations": observations,
        "has_start": has_start,
        "has_end": has_end,
        "passes": coverage >= min_coverage and has_start and has_end,
    }
  return scores


def drop_reason(score: Dict, min_coverage: float) -> str:
  """
  Renders a human-readable reason for dropping a ticker.

  A ticker with no observations at all is reported separately from one that
  merely falls short of the threshold. The two have different causes and
  different consequences: the former means the price source no longer serves
  the symbol, which is the dominant reason constituents disappear and the
  source of the instances' survivorship bias.
  """
  if score["observations"] == 0:
    return "no data available from the price source"

  reasons = []
  if score["coverage"] < min_coverage:
    reasons.append(f"coverage {score['coverage']:.3f} < {min_coverage:.3f}")
  if not score["has_start"]:
    reasons.append("no data at window start")
  if not score["has_end"]:
    reasons.append("no data at window end")
  return "; ".join(reasons) if reasons else "unknown"


def window_returns(prices: pd.DataFrame, tickers: Sequence[str],
                   start: pd.Timestamp, end: pd.Timestamp,
                   lookback_days: int = 60) -> pd.DataFrame:
  """
  Computes the daily returns of one window.

  The price slice deliberately reaches back to the last trading day before the
  window, so that the first in-window day still produces a return. Computing
  pct_change() on the window alone would silently discard it. Because the extra
  day only seeds the first return and is never itself a return date, the
  training and out-of-sample return series remain disjoint.
  """
  lookback = start - pd.Timedelta(days=lookback_days)
  frame = window_slice(prices[list(tickers)], lookback, end).dropna(how="any")
  if frame.empty:
    return frame

  in_window = frame.index >= start
  if not in_window.any():
    return frame.iloc[0:0]

  first = int(in_window.argmax())
  frame = frame.iloc[max(first - 1, 0):]
  # fill_method=None is explicit rather than load-bearing: the slice is already
  # complete-case. It pins the behaviour against the pandas default changing.
  returns = frame.pct_change(fill_method=None).iloc[1:]
  return returns[returns.index >= start]


def calculate_expected_returns(returns: pd.DataFrame) -> pd.Series:
  """Calculates the daily expected returns, i.e. the mean of daily returns."""
  return returns.mean()


def calculate_covariance_matrix(returns: pd.DataFrame) -> pd.DataFrame:
  """Calculates the daily covariance matrix of the daily returns."""
  return returns.cov()


def write_window(directory: str, returns: pd.DataFrame,
                 tickers: Sequence[str]) -> Dict[str, str]:
  """
  Writes the two instance files of one window and returns their digests.

  Both files are emitted in the same ticker order because
  `Instance::load_instance` aligns expected-returns row i with covariance
  row/column i positionally, without matching names.
  """
  os.makedirs(directory, exist_ok=True)
  ordered = list(tickers)

  expected_returns = calculate_expected_returns(returns).reindex(ordered)
  covariance = calculate_covariance_matrix(returns).reindex(
      index=ordered, columns=ordered)

  expected_returns_path = os.path.join(directory, EXPECTED_RETURNS_FILENAME)
  covariance_path = os.path.join(directory, COVARIANCE_FILENAME)

  frame = expected_returns.to_frame(name=EXPECTED_RETURNS_COLUMN)
  frame.index.name = None
  frame.to_csv(expected_returns_path, lineterminator="\n")

  covariance.index.name = None
  covariance.columns.name = None
  covariance.to_csv(covariance_path, lineterminator="\n")

  return {
      EXPECTED_RETURNS_FILENAME: sha256_file(expected_returns_path),
      COVARIANCE_FILENAME: sha256_file(covariance_path),
  }


def window_metadata(name: str, start: pd.Timestamp, end: pd.Timestamp,
                    returns: pd.DataFrame, digests: Dict[str, str]) -> Dict:
  """Assembles the metadata record of one window."""
  return {
      "name": name,
      "start": start.strftime("%Y-%m-%d"),
      "end": end.strftime("%Y-%m-%d"),
      "first_return_date": returns.index[0].strftime("%Y-%m-%d"),
      "last_return_date": returns.index[-1].strftime("%Y-%m-%d"),
      "num_observations": int(returns.shape[0]),
      "checksums": digests,
  }


def build_instance(year: int, prices: pd.DataFrame, candidates: List[str],
                   min_coverage: float, allow_partial: bool) -> Dict:
  """
  Builds one instance, writing its two windows and its metadata.

  @raises RuntimeError If any constituent fails the screening rule and
  allow_partial is False, or if a window ends up with no usable data.
  """
  name = f"ibov_{year}"
  train_start, split, oos_end = window_bounds(year)
  windows = {"train": (train_start, split), "oos": (split, oos_end)}

  # A ticker must clear the rule in both windows: the two windows have to share
  # one asset set and one order, since the solver reads them with the same
  # positional loader and the out-of-sample evaluation reuses the weights.
  scores = {}
  for window_name, (start, end) in windows.items():
    window = window_slice(prices[candidates], start, end)
    calendar = trading_calendar(window)
    if len(calendar) == 0:
      raise RuntimeError(
          f"{name}: the {window_name} window [{start.date()}, {end.date()}) "
          f"has no trading days in the cache")
    scores[window_name] = screen_window(window, calendar, min_coverage)

  kept = sorted(t for t in candidates
                if all(scores[w][t]["passes"] for w in windows))
  dropped = []
  for ticker in candidates:
    if ticker in kept:
      continue
    failing = [w for w in windows if not scores[w][ticker]["passes"]]
    unavailable = all(scores[w][ticker]["observations"] == 0 for w in windows)
    dropped.append({
        "ticker": ticker,
        "cause": "no_data" if unavailable else "coverage",
        "windows": failing,
        "reasons": {w: drop_reason(scores[w][ticker], min_coverage)
                    for w in failing},
        "coverage": {w: round(scores[w][ticker]["coverage"], 6)
                     for w in windows},
    })

  no_data = [d["ticker"] for d in dropped if d["cause"] == "no_data"]
  low_coverage = [d for d in dropped if d["cause"] == "coverage"]

  if dropped:
    detail = "\n".join(
        f"    {d['ticker']}: " +
        "; ".join(f"{w}: {d['reasons'][w]}" for w in d["windows"])
        for d in low_coverage)
    message = (
        f"{name}: {len(dropped)} of {len(candidates)} constituents dropped. "
        f"{len(no_data)} are not served by the price source at all "
        f"({', '.join(no_data)}).")
    if low_coverage:
      message += (f"\n  {len(low_coverage)} fail the coverage rule:\n"
                  f"{detail}")
    if not allow_partial:
      raise RuntimeError(
          message + "\n  Pass --allow-partial to drop them and continue, or "
          "lower --min-coverage.")
    logging.warning(message)

  if not kept:
    raise RuntimeError(f"{name}: no constituent survives the coverage rule")

  instance_dir = os.path.join(INSTANCES_DIR, name)
  os.makedirs(instance_dir, exist_ok=True)
  shutil.copyfile(tickers_file(year), os.path.join(instance_dir, "tickers.csv"))

  metadata = {
      "instance": name,
      "num_assets": len(kept),
      "num_constituents": len(candidates),
      "tickers": kept,
      "dropped": dropped,
      "num_dropped_no_data": len(no_data),
      "num_dropped_coverage": len(low_coverage),
      "survivorship_bias_warning": (
          f"{len(no_data)} of {len(candidates)} constituents of the "
          f"{year} IBOVESPA are no longer served by the price source, having "
          f"been delisted, renamed or merged since. This instance therefore "
          f"contains only assets that survived to the date the cache was "
          f"built, and its expected returns are biased upward. It is a valid "
          f"fixed benchmark for comparing solvers, which all see identical "
          f"data, but it does not reproduce the {year} index."),
      "min_coverage": min_coverage,
      "annualization": "none (daily statistics)",
      "source_tickers_file": os.path.relpath(tickers_file(year), REPO_ROOT),
      "source_tickers_sha256": sha256_file(tickers_file(year)),
      "windows": {},
  }

  for window_name, (start, end) in windows.items():
    returns = window_returns(prices, kept, start, end)
    if returns.empty:
      raise RuntimeError(
          f"{name}: the {window_name} window yielded no returns")
    digests = write_window(
        os.path.join(instance_dir, window_name), returns, kept)
    metadata["windows"][window_name] = window_metadata(
        window_name, start, end, returns, digests)
    logging.info(
        f"{name}/{window_name}: {len(kept)} assets, "
        f"{returns.shape[0]} observations "
        f"({metadata['windows'][window_name]['first_return_date']} .. "
        f"{metadata['windows'][window_name]['last_return_date']})")

  # No timestamp is recorded here on purpose: metadata.json has to be
  # byte-identical across rebuilds for the offline determinism check to mean
  # anything.
  with open(os.path.join(instance_dir, "metadata.json"), "w",
            encoding="utf-8") as handle:
    json.dump(metadata, handle, indent=2, sort_keys=True)
    handle.write("\n")

  return metadata


def command_build(args: argparse.Namespace) -> int:
  """Builds the instances from the cache. Never touches the network."""
  years = args.years if args.years else INSTANCE_YEARS
  candidates_by_year = {year: load_tickers(tickers_file(year))
                        for year in years}
  needed = sorted({t for tickers in candidates_by_year.values()
                   for t in tickers})

  logging.info(
      f"Loading {len(needed)} cached tickers for {len(years)} instances.")
  prices = load_prices(needed, verify=not args.no_verify)

  failures = []
  for year in years:
    try:
      build_instance(year, prices, candidates_by_year[year],
                     args.min_coverage, args.allow_partial)
    except RuntimeError as error:
      logging.error(str(error))
      failures.append(f"ibov_{year}")

  if failures:
    logging.error(f"Failed to build: {failures}")
    return 1
  logging.info(f"Built {len(years)} instances under {INSTANCES_DIR}.")
  return 0


# --- Entry point ---


def setup_arg_parser() -> argparse.ArgumentParser:
  """Sets up and returns the argument parser for command-line options."""
  parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
  subparsers = parser.add_subparsers(dest="command", required=True)

  fetch = subparsers.add_parser(
      "fetch", help="download prices into the cache (the only network step)")
  fetch.add_argument(
      "--tickers", nargs="+", default=None,
      help="fetch only these symbols instead of every known constituent")
  fetch.add_argument(
      "--refresh", action="store_true",
      help="re-download tickers that are already cached")
  fetch.add_argument(
      "--delay", type=float, default=0.5,
      help="seconds to wait between downloads. Default: 0.5")
  fetch.add_argument(
      "--mark-no-data", nargs="+", default=None,
      help="record these symbols as permanently unavailable without "
           "downloading, for long-dead tickers that fail deterministically "
           "inside yfinance instead of returning an empty result")
  fetch.add_argument(
      "--note", type=str, default=None,
      help="note stored alongside --mark-no-data entries in the manifest")
  fetch.set_defaults(handler=command_fetch)

  build = subparsers.add_parser(
      "build", help="build the instances from the cache (offline)")
  build.add_argument(
      "--years", nargs="+", type=int, default=None,
      help=f"instance years to build. Default: {INSTANCE_YEARS[0]}"
           f"..{INSTANCE_YEARS[-1]}")
  build.add_argument(
      "--min-coverage", type=float, default=DEFAULT_MIN_COVERAGE,
      help=f"minimum fraction of a window's trading days a ticker must quote. "
           f"Default: {DEFAULT_MIN_COVERAGE}")
  build.add_argument(
      "--allow-partial", action="store_true",
      help="drop constituents that fail the coverage rule instead of failing")
  build.add_argument(
      "--no-verify", action="store_true",
      help="skip the cache checksum verification")
  build.set_defaults(handler=command_build)

  return parser


def main() -> int:
  """Main function to orchestrate the instance generation pipeline."""
  setup_logging()
  args = setup_arg_parser().parse_args()
  try:
    return args.handler(args)
  except (ValueError, RuntimeError, FileNotFoundError) as error:
    logging.error(str(error))
    return 1


if __name__ == "__main__":
  sys.exit(main())
