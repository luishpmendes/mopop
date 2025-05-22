import pandas as pd
import yfinance as yf
from argparse import ArgumentParser, Namespace
import os
import logging
from typing import List, Optional, Union

# Set up logging
logging.basicConfig(
    level=logging.INFO, format="%(asctime)s - %(levelname)s - %(message)s"
)


def download_data(
    tickers: List[str], interval: str, start: str, end: Optional[str]
) -> pd.DataFrame:
  """
  Download adjusted close price data for a list of tickers using yfinance.

  Parameters:
  tickers (List[str]): A list of ticker symbols to download data for.
  interval (str): The interval at which to download data (e.g., '1d', '1wk', '1mo').
  start (str): The start date for the data in 'YYYY-MM-DD' format.
  end (Optional[str]): The end date for the data in 'YYYY-MM-DD' format. If None, data is downloaded up to the current date.

  Returns:
  pd.DataFrame: A DataFrame containing the adjusted close price data for the specified tickers and date range.
  """
  try:
    data : pd.DataFrame = yf.download(tickers=tickers, interval=interval, start=start, end=end)[
        "Adj Close"
    ]

    if data.empty:
      raise ValueError(
          "No data returned from yfinance. Please check your tickers and date range."
      )
    return data
  except Exception as e:
    logging.error(f"Error downloading data: {e}")
    raise


def save_to_csv(
    data: Union[pd.DataFrame, pd.Series], filename: Optional[str], description: str
) -> None:
  """
  Save a pandas DataFrame or Series to a CSV file if the filename is provided.

  Parameters:
  data (Union[pd.DataFrame, pd.Series]): The data to be saved, either as a DataFrame or Series.
  filename (Optional[str]): The name of the file to save the data to. If None, the data will not be saved.
  description (str): A description of the data being saved, used for logging purposes.

  Returns:
  None
  """
  if filename:
    logging.info(f"Saving {description} to {filename}")
    try:
      data.to_csv(filename, index=True)
    except Exception as e:
      logging.error(f"Failed to save {description} to {filename}: {e}")
      raise


def calculate_returns(adj_close: pd.DataFrame) -> pd.DataFrame:
  """
  Calculate daily returns based on adjusted close prices.

  Parameters:
  adj_close (pd.DataFrame): A DataFrame containing the adjusted close prices of stocks.

  Returns:
  pd.DataFrame: A DataFrame containing the daily returns calculated from the adjusted close prices.
  """
  logging.info("Calculating daily returns.")
  return adj_close.dropna().pct_change().dropna()


def calculate_expected_returns(returns: pd.DataFrame) -> pd.Series:
  """
  Calculate the expected returns from a DataFrame of daily returns.

  Args:
      returns (pd.DataFrame): A DataFrame containing daily returns for various assets.

  Returns:
      pd.Series: A Series containing the expected returns for each asset.
  """
  logging.info("Calculating expected returns.")
  return returns.mean()


def calculate_covariance_matrix(returns: pd.DataFrame) -> pd.DataFrame:
  """
  Calculate the covariance matrix of returns.

  This function takes a DataFrame of returns and calculates the covariance matrix.

  Args:
      returns (pd.DataFrame): A DataFrame where each column represents the returns of a different asset.

  Returns:
      pd.DataFrame: A DataFrame representing the covariance matrix of the returns.
  """
  logging.info("Calculating covariance matrix.")
  return returns.cov()


def main() -> None:
  """
  Main function to download stock data and calculate statistics.
  This function sets up argument parsing for various parameters such as tickers,
  interval, start date, end date, and file paths for saving adjusted close prices,
  returns, expected returns, and covariance matrix. It then performs the following steps:
  1. Parses command-line arguments.
  2. Checks if the tickers CSV file exists.
  3. Reads tickers from the CSV file.
  4. Downloads historical adjusted close prices for the specified tickers.
  5. Saves the adjusted close prices to a CSV file if specified.
  6. Calculates and saves daily returns, expected returns, and covariance matrix
      if the corresponding file paths are provided.
  """
  # Set up argument parsing
  parser : ArgumentParser = ArgumentParser(
      description="Download stock data and calculate statistics.")
  parser.add_argument(
      "--tickers",
      type=str,
      default="input/tickers.csv",
      help="CSV file containing the tickers",
  )
  parser.add_argument(
      "--interval",
      type=str,
      default="1d",
      help="The interval for historical data (e.g., 1d, 1wk)",
  )
  parser.add_argument(
      "--start",
      type=str,
      default="2020-01-01",
      help="Start date for historical data (YYYY-MM-DD)",
  )
  parser.add_argument(
      "--end", type=str, help="End date for historical data (YYYY-MM-DD)"
  )
  parser.add_argument(
      "--adj_close", type=str, help="CSV file to save the adjusted close prices"
  )
  parser.add_argument(
      "--returns", type=str, help="CSV file to save the daily returns"
  )
  parser.add_argument(
      "--expected_returns", type=str, help="CSV file to save the expected returns"
  )
  parser.add_argument(
      "--covariance_matrix", type=str, help="CSV file to save the covariance matrix"
  )

  args: Namespace = parser.parse_args()

  # Check if the tickers CSV file exists
  if not os.path.exists(args.tickers):
    logging.error(f"The tickers file '{args.tickers}' does not exist.")
    raise FileNotFoundError(
        f"The tickers file '{args.tickers}' does not exist.")

  # Read tickers from the CSV file
  try:
    tickers : pd.DataFrame = pd.read_csv(
        args.tickers).iloc[:, 0].dropna().astype(str).tolist()
  except Exception as e:
    logging.error(f"Error reading tickers from file: {e}")
    raise

  if not tickers:
    raise ValueError(
        "The tickers list is empty. Please check the CSV file.")

  # Download historical adjusted close prices
  adj_close : pd.DataFrame = download_data(tickers, args.interval, args.start, args.end)

  # Save adjusted close prices if specified
  save_to_csv(adj_close, args.adj_close, "adjusted close prices")

  # Calculate and save returns if requested
  if args.returns or args.expected_returns or args.covariance_matrix:
    returns : pd.DataFrame = calculate_returns(adj_close)
    save_to_csv(returns, args.returns, "daily returns")

    # Calculate and save expected returns if requested
    if args.expected_returns:
      expected_returns : pd.Series = calculate_expected_returns(returns)
      save_to_csv(expected_returns, args.expected_returns,
                  "expected returns")

    # Calculate and save covariance matrix if requested
    if args.covariance_matrix:
      covariance_matrix : pd.DataFrame = calculate_covariance_matrix(returns)
      save_to_csv(covariance_matrix, args.covariance_matrix,
                  "covariance matrix")


if __name__ == "__main__":
  main()
