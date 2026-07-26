#!/usr/bin/env python3
"""Validates the generated instances under instances/.

The files are parsed the way `Instance::load_instance` parses them — one header
line, then comma-separated fields with the ticker first — rather than through
pandas, so that anything this script accepts is guaranteed to load on the C++
side. Two properties matter most and neither is checked by the solver:

  * The expected-returns rows and the covariance rows/columns are aligned
    positionally, with no matching by name, so their orders must be identical.
  * `std::stod("nan")` succeeds silently, so a NaN would propagate into every
    objective value instead of failing loudly. NaNs have to be caught here.

Exits nonzero if any instance fails.
"""

import argparse
import csv
import json
import logging
import math
import os
import sys
from typing import Dict, List, Optional, Tuple

REPO_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
INSTANCES_DIR = os.path.join(REPO_ROOT, "instances")

INSTANCE_YEARS = list(range(2011, 2021))
WINDOWS = ("train", "oos")

EXPECTED_RETURNS_FILENAME = "expected_returns.csv"
COVARIANCE_FILENAME = "covariance_matrix.csv"

# Covariance matrices are symmetric by construction, so any asymmetry is float
# formatting noise rather than a real defect. The tolerance is relative to the
# magnitude of the entries being compared.
SYMMETRY_TOLERANCE = 1e-12


def setup_logging() -> None:
  """Configures basic logging for the script."""
  logging.basicConfig(
      level=logging.INFO,
      format="%(asctime)s - %(levelname)s - %(message)s",
      datefmt="%Y-%m-%d %H:%M:%S",
  )


def read_expected_returns(file_path: str,
                          errors: List[str]) -> Optional[Tuple[List[str],
                                                               List[float]]]:
  """
  Reads an expected returns file exactly as the C++ loader does.

  @return The tickers and their expected returns, or None if the file is
  unreadable or structurally invalid.
  """
  with open(file_path, newline="", encoding="utf-8") as handle:
    rows = list(csv.reader(handle))

  if len(rows) < 2:
    errors.append(f"{file_path}: no data rows")
    return None

  tickers, values = [], []
  for number, row in enumerate(rows[1:], start=2):
    if len(row) < 2:
      errors.append(f"{file_path}:{number}: expected 'ticker,value', "
                    f"got {row}")
      return None
    try:
      value = float(row[1])
    except ValueError:
      errors.append(f"{file_path}:{number}: '{row[1]}' is not a number")
      return None
    if not math.isfinite(value):
      errors.append(f"{file_path}:{number}: {row[0]} has non-finite value "
                    f"{row[1]}")
      return None
    tickers.append(row[0])
    values.append(value)

  return tickers, values


def read_covariance(file_path: str,
                    errors: List[str]) -> Optional[Tuple[List[str],
                                                         List[str],
                                                         List[List[float]]]]:
  """
  Reads a covariance file exactly as the C++ loader does.

  @return The column tickers from the header, the row tickers, and the matrix,
  or None if the file is unreadable or structurally invalid.
  """
  with open(file_path, newline="", encoding="utf-8") as handle:
    rows = list(csv.reader(handle))

  if len(rows) < 2:
    errors.append(f"{file_path}: no data rows")
    return None

  columns = rows[0][1:]
  row_tickers, matrix = [], []
  for number, row in enumerate(rows[1:], start=2):
    values = []
    for position, cell in enumerate(row[1:]):
      try:
        value = float(cell)
      except ValueError:
        errors.append(f"{file_path}:{number}: column {position + 1} "
                      f"('{cell}') is not a number")
        return None
      if not math.isfinite(value):
        errors.append(f"{file_path}:{number}: column {position + 1} is "
                      f"non-finite ({cell})")
        return None
      values.append(value)
    row_tickers.append(row[0])
    matrix.append(values)

  return columns, row_tickers, matrix


def validate_window(directory: str, window: str,
                    errors: List[str]) -> Optional[List[str]]:
  """
  Validates the two files of one window.

  @return The window's ticker order, or None if validation failed.
  """
  expected_returns_path = os.path.join(directory, EXPECTED_RETURNS_FILENAME)
  covariance_path = os.path.join(directory, COVARIANCE_FILENAME)

  for path in (expected_returns_path, covariance_path):
    if not os.path.exists(path):
      errors.append(f"{path}: missing")
      return None

  expected = read_expected_returns(expected_returns_path, errors)
  covariance = read_covariance(covariance_path, errors)
  if expected is None or covariance is None:
    return None

  tickers, _ = expected
  columns, row_tickers, matrix = covariance

  if row_tickers != tickers:
    mismatch = next(
        (i for i, (a, b) in enumerate(zip(row_tickers, tickers)) if a != b),
        min(len(row_tickers), len(tickers)))
    errors.append(
        f"{window}: covariance row order differs from the expected returns "
        f"order, first at index {mismatch}. The C++ loader aligns them "
        f"positionally, so this would silently pair each asset with another "
        f"asset's row.")
    return None

  if columns != tickers:
    errors.append(f"{window}: covariance header order differs from the row "
                  f"order")
    return None

  size = len(tickers)
  for index, row in enumerate(matrix):
    if len(row) != size:
      errors.append(f"{window}: covariance row {index} has {len(row)} values, "
                    f"expected {size} (matrix is not square)")
      return None

  for i in range(size):
    if matrix[i][i] < 0.0:
      errors.append(f"{window}: negative variance for {tickers[i]}: "
                    f"{matrix[i][i]}")
      return None
    for j in range(i + 1, size):
      scale = max(abs(matrix[i][j]), abs(matrix[j][i]), 1e-300)
      if abs(matrix[i][j] - matrix[j][i]) / scale > SYMMETRY_TOLERANCE:
        errors.append(f"{window}: covariance is asymmetric at "
                      f"({tickers[i]}, {tickers[j]}): {matrix[i][j]} vs "
                      f"{matrix[j][i]}")
        return None

  return tickers


def validate_metadata(directory: str, tickers: List[str],
                      errors: List[str]) -> Optional[Dict]:
  """
  Validates metadata.json against the files it describes.

  Checks the recorded ticker list and per-file digests, and that the training
  and out-of-sample return series do not overlap in time.
  """
  path = os.path.join(directory, "metadata.json")
  if not os.path.exists(path):
    errors.append(f"{path}: missing")
    return None

  with open(path, encoding="utf-8") as handle:
    metadata = json.load(handle)

  if metadata.get("tickers") != tickers:
    errors.append(f"{path}: the recorded ticker list does not match the "
                  f"instance files")

  if metadata.get("num_assets") != len(tickers):
    errors.append(f"{path}: num_assets is {metadata.get('num_assets')}, "
                  f"files hold {len(tickers)}")

  windows = metadata.get("windows", {})
  for window in WINDOWS:
    if window not in windows:
      errors.append(f"{path}: no '{window}' window recorded")
      return metadata
    if windows[window].get("num_observations", 0) <= 0:
      errors.append(f"{path}: the {window} window records no observations")

  train, oos = windows["train"], windows["oos"]
  if train["end"] != oos["start"]:
    errors.append(f"{path}: the windows are not contiguous "
                  f"(train ends {train['end']}, oos starts {oos['start']})")
  if train["last_return_date"] >= oos["first_return_date"]:
    errors.append(
        f"{path}: the training and out-of-sample return series overlap "
        f"(train ends {train['last_return_date']}, oos starts "
        f"{oos['first_return_date']})")

  return metadata


def validate_instance(name: str, errors: List[str]) -> bool:
  """Validates one instance directory. Returns True when it is sound."""
  directory = os.path.join(INSTANCES_DIR, name)
  if not os.path.isdir(directory):
    errors.append(f"{directory}: missing")
    return False

  before = len(errors)
  orders = {}
  for window in WINDOWS:
    tickers = validate_window(os.path.join(directory, window), window, errors)
    if tickers is None:
      return False
    orders[window] = tickers

  if orders["train"] != orders["oos"]:
    errors.append(
        f"{name}: the train and oos ticker orders differ, so a portfolio "
        f"optimized on train would be evaluated against the wrong assets "
        f"out of sample")
    return False

  validate_metadata(directory, orders["train"], errors)

  if len(errors) > before:
    return False

  logging.info(f"{name}: OK ({len(orders['train'])} assets)")
  return True


def main() -> int:
  """Validates every instance and reports a summary."""
  setup_logging()
  parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
  parser.add_argument(
      "--years", nargs="+", type=int, default=None,
      help=f"instance years to validate. Default: {INSTANCE_YEARS[0]}"
           f"..{INSTANCE_YEARS[-1]}")
  args = parser.parse_args()

  years = args.years if args.years else INSTANCE_YEARS
  errors: List[str] = []
  passed = sum(validate_instance(f"ibov_{year}", errors) for year in years)

  for error in errors:
    logging.error(error)

  logging.info(f"{passed}/{len(years)} instances passed validation.")
  return 0 if passed == len(years) else 1


if __name__ == "__main__":
  sys.exit(main())
