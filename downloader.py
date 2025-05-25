import pandas as pd
import yfinance as yf
import argparse
import logging
import os
from typing import List, Optional, Dict, Union

# --- Logging Configuration ---


def setup_logging():
  """Configures basic logging for the script."""
  logging.basicConfig(
      level=logging.INFO,
      format="%(asctime)s - %(levelname)s - %(message)s",
      datefmt="%Y-%m-%d %H:%M:%S",
  )

# --- Argument Parsing ---


def setup_arg_parser() -> argparse.ArgumentParser:
  """Sets up and returns the argument parser for command-line options."""
  parser = argparse.ArgumentParser(
      description="Download financial data, clean it, and calculate returns, expected returns, and covariance matrix."
  )
  parser.add_argument(
      "--tickers",
      type=str,
      default="input/tickers.csv",
      help="Path to the CSV file containing ticker symbols. Expected to have a column named 'Ticker'. Default: input/tickers.csv",
  )
  parser.add_argument(
      "--start",
      type=str,
      required=True,
      help="Start date for historical data in YYYY-MM-DD format.",
  )
  parser.add_argument(
      "--end",
      type=str,
      default=None,  # yfinance will fetch up to the most recent trading day if None
      help="End date for historical data in YYYY-MM-DD format. Default: Most recent data.",
  )
  parser.add_argument(
      "--interval",
      type=str,
      default="1d",
      help="Data interval (e.g., '1d', '1wk', '1mo'). Default: '1d'",
  )
  parser.add_argument(
      "--returns",
      type=str,
      default="input/returns.csv",
      help="File path to save the calculated returns. Default: input/returns.csv",
  )
  parser.add_argument(
      "--expected_returns",
      type=str,
      default="input/expected_returns.csv",
      help="File path to save the calculated annualized expected returns. Default: input/expected_returns.csv",
  )
  parser.add_argument(
      "--covariance_matrix",
      type=str,
      default="input/covariance_matrix.csv",
      help="File path to save the calculated annualized covariance matrix. Default: input/covariance_matrix.csv",
  )
  return parser

# --- Data Loading and Processing Functions ---


def load_tickers(file_path: str) -> List[str]:
  """
  Loads ticker symbols from a CSV file.
  Assumes the CSV has a header and a column named 'Ticker'.
  """
  if not os.path.exists(file_path):
    logging.error(f"Tickers file not found: {file_path}")
    return []
  try:
    df = pd.read_csv(file_path)
    if 'Ticker' not in df.columns:
      logging.error(
          f"'Ticker' column not found in {file_path}. Please ensure the CSV has a 'Ticker' column.")
      # Attempt to use the first column if 'Ticker' is not found, with a warning.
      if not df.empty and len(df.columns) > 0:
        tickers = df.iloc[:, 0].dropna().astype(str).unique().tolist()
        logging.warning(
            f"Using the first column as tickers from {file_path} as 'Ticker' column was not found.")
        if tickers:
          logging.info(
              f"Loaded {len(tickers)} tickers from the first column of {file_path}.")
          return tickers
      return []

    tickers = df['Ticker'].dropna().astype(
        str).unique().tolist()  # Use unique to avoid duplicates
    if not tickers:
      logging.warning(f"No tickers found in 'Ticker' column of {file_path}.")
      return []
    logging.info(
        f"Successfully loaded {len(tickers)} unique tickers from {file_path}: {tickers}")
    return tickers
  except pd.errors.EmptyDataError:
    logging.error(f"The tickers file {file_path} is empty.")
    return []
  except Exception as e:
    logging.error(f"Error reading tickers from {file_path}: {e}")
    return []


def download_stock_data_sequentially(
    tickers: List[str], start_date: str, end_date: Optional[str], interval: str
) -> Dict[str, pd.Series]:
  """
  Downloads historical 'Close' (adjusted) price data for each ticker sequentially.
  Uses auto_adjust=True, so 'Close' is the adjusted close price.
  """
  adj_close_prices_dict: Dict[str, pd.Series] = {}
  if not tickers:
    logging.warning("Ticker list is empty. No data to download.")
    return adj_close_prices_dict

  logging.info(f"Starting sequential download for {len(tickers)} tickers...")
  for ticker_symbol in tickers:
    try:
      logging.info(f"Downloading data for {ticker_symbol}...")
      ticker_data = yf.Ticker(ticker_symbol)
      # auto_adjust=True handles splits and dividends; 'Close' becomes adjusted close.
      hist_data = ticker_data.history(
          start=start_date, end=end_date, interval=interval, auto_adjust=True
      )

      if hist_data.empty:
        logging.warning(
            f"No data found for {ticker_symbol} for the given period/interval.")
        continue

      if 'Close' not in hist_data.columns:
        logging.warning(
            f"'Close' column not found in data for {ticker_symbol}. Skipping.")
        continue

      adj_close_prices_dict[ticker_symbol] = hist_data['Close']
      logging.info(
          f"Successfully downloaded data for {ticker_symbol} ({len(hist_data)} rows).")

    except Exception as e:
      logging.error(f"Failed to download data for {ticker_symbol}: {e}")

  if not adj_close_prices_dict:
    logging.warning("Failed to download data for any of the provided tickers.")

  return adj_close_prices_dict


def preprocess_data(
    adj_close_prices_dict: Dict[str, pd.Series]
) -> Optional[pd.DataFrame]:
  """
  Combines individual ticker Series into a single DataFrame and
  keeps only common time periods (dates) where all tickers have data.
  """
  if not adj_close_prices_dict:
    logging.warning("No data to preprocess as the input dictionary is empty.")
    return None

  try:
    # Concatenate all series. Pandas aligns them by index (date).
    combined_df = pd.concat(adj_close_prices_dict.values(),
                            axis=1, keys=adj_close_prices_dict.keys())
    logging.info(f"Combined data shape before cleaning: {combined_df.shape}")

    # Drop rows (dates) where any ticker has a NaN value
    cleaned_df = combined_df.dropna(how='any', axis=0)
    logging.info(
        f"Combined data shape after cleaning (keeping common dates): {cleaned_df.shape}")

    if cleaned_df.empty:
      logging.warning(
          "Resulting DataFrame is empty after removing rows with missing values. "
          "This means there are no common dates with data for all selected tickers."
      )
      return None

    # Re-check for tickers that might have become all NaN after initial combination (if not caught by dropna)
    # This typically happens if a ticker had data but not on the common dates.
    # The .dropna(axis=1, how='all') would remove columns that are entirely NaN.
    cleaned_df = cleaned_df.dropna(axis=1, how='all')
    if cleaned_df.empty:  # Check again if removing all-NaN columns made it empty
      logging.warning(
          "Resulting DataFrame became empty after removing columns that were all NaN on common dates.")
      return None
    logging.info(
        f"Final data shape after potentially removing all-NaN columns: {cleaned_df.shape}")

    return cleaned_df
  except Exception as e:
    logging.error(f"Error during data preprocessing: {e}")
    return None

# --- Financial Calculation Functions ---


def calculate_returns(adj_close_df: pd.DataFrame) -> Optional[pd.DataFrame]:
  """Calculates percentage returns."""
  if adj_close_df.empty:
    logging.warning("Cannot calculate returns from an empty DataFrame.")
    return None
  try:
    returns = adj_close_df.pct_change()
    # The first row will be NaN after pct_change, so drop it.
    returns = returns.iloc[1:]
    if returns.empty:
      logging.warning(
          "Returns DataFrame is empty after dropping the first NaN row. Likely only one data point was available.")
      return None
    logging.info(f"Calculated returns. Shape: {returns.shape}")
    return returns
  except Exception as e:
    logging.error(f"Error calculating returns: {e}")
    return None


def calculate_expected_returns(
    returns_df: pd.DataFrame
) -> Optional[pd.Series]:
  """Calculates annualized expected returns (mean of returns)."""
  if returns_df.empty:
    logging.warning(
        "Cannot calculate expected returns from an empty returns DataFrame.")
    return None
  try:
    expected_returns = returns_df.mean()
    logging.info(
        f"Calculated expected returns for {len(expected_returns)} tickers.")
    return expected_returns
  except Exception as e:
    logging.error(f"Error calculating expected returns: {e}")
    return None


def calculate_covariance_matrix(
    returns_df: pd.DataFrame
) -> Optional[pd.DataFrame]:
  """Calculates the annualized covariance matrix of returns."""
  if returns_df.empty:
    logging.warning(
        "Cannot calculate covariance matrix from an empty returns DataFrame.")
    return None
  try:
    covariance = returns_df.cov()
    logging.info(
        f"Calculated covariance matrix. Shape: {covariance.shape}")
    return covariance
  except Exception as e:
    logging.error(f"Error calculating covariance matrix: {e}")
    return None

# --- Data Saving Function ---


def save_data_to_csv(
    data: Union[pd.DataFrame, pd.Series],
    file_path: Optional[str],
    description: str,
    is_series_to_frame: bool = False,  # For saving Series as a named DataFrame column
    series_frame_name: Optional[str] = None
) -> None:
  """Helper function to save data (DataFrame or Series) to a CSV file."""
  if file_path is None:
    logging.info(f"No file path provided for {description}, skipping save.")
    return
  if data is None or (hasattr(data, 'empty') and data.empty):
    logging.warning(
        f"Data for {description} is None or empty, skipping save to {file_path}.")
    return

  try:
    output_data = data
    if is_series_to_frame and isinstance(data, pd.Series):
      frame_name = series_frame_name if series_frame_name else description.replace(
          " ", "")
      output_data = data.to_frame(name=frame_name)

    output_data.to_csv(file_path)
    logging.info(f"Successfully saved {description} to {file_path}")
  except Exception as e:
    logging.error(f"Error saving {description} to {file_path}: {e}")


# --- Main Execution Logic ---
def main():
  """Main function to orchestrate the data processing pipeline."""
  setup_logging()
  parser = setup_arg_parser()
  args = parser.parse_args()

  logging.info("Starting financial data processing script.")
  logging.info(f"Arguments: {args}")

  # 1. Load Tickers
  tickers = load_tickers(args.tickers)
  if not tickers:
    logging.error("No tickers loaded. Exiting script.")
    return

  # 2. Download Data Sequentially
  adj_close_prices_dict = download_stock_data_sequentially(
      tickers, args.start, args.end, args.interval
  )
  if not adj_close_prices_dict:
    logging.error("Failed to download data for any tickers. Exiting script.")
    return

  # 3. Preprocess Data (Combine and Clean)
  adj_close_cleaned_df = preprocess_data(adj_close_prices_dict)
  if adj_close_cleaned_df is None or adj_close_cleaned_df.empty:
    logging.error(
        "Data preprocessing resulted in no usable data (e.g., no common dates or all tickers failed). Exiting script."
    )
    return

  active_tickers = adj_close_cleaned_df.columns.tolist()
  logging.info(f"Data successfully preprocessed for tickers: {active_tickers}")

  # 4. Calculate Returns
  returns_df = calculate_returns(adj_close_cleaned_df)
  if returns_df is None or returns_df.empty:
    logging.error(
        "Failed to calculate returns or returns data is empty. Exiting further calculations.")
    return  # Cannot proceed without returns
  save_data_to_csv(returns_df, args.returns, "Returns")

  # 5. Calculate Expected Returns
  expected_returns_series = calculate_expected_returns(returns_df)
  if expected_returns_series is not None:
    save_data_to_csv(
        expected_returns_series,
        args.expected_returns,
        "Annualized Expected Returns",
        is_series_to_frame=True,
        series_frame_name="ExpectedAnnualReturn"
    )

  # 6. Calculate Covariance Matrix
  covariance_matrix_df = calculate_covariance_matrix(returns_df)
  if covariance_matrix_df is not None:
    save_data_to_csv(
        covariance_matrix_df, args.covariance_matrix, "Annualized Covariance Matrix"
    )

  logging.info("Financial data processing script finished.")


if __name__ == "__main__":
  main()
