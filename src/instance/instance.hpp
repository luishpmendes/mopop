#pragma once

#define NSBRKGA_MULTIPLE_INCLUSIONS

#include <istream>
#include <ostream>
#include <vector>

#include "nsbrkga.hpp"

namespace mopop {
/**
 * @class Instance
 * @brief Represents a financial instance with assets, tickers, expected
 * returns, and covariance matrix.
 *
 * The Instance class encapsulates data related to financial assets, including
 * their ticker symbols, expected returns, and the covariance matrix. It
 * provides functionality to load this data from files and supports various
 * constructors for initialization.
 */
class Instance {
 public:
  /**
   * @brief Number of assets.
   *
   * This variable holds the total count of assets.
   */
  unsigned num_assets;

  /**
   * @brief A vector that stores a list of ticker symbols.
   *
   * This vector holds strings representing ticker symbols, which are
   * typically used to uniquely identify publicly traded companies on
   * stock exchanges.
   */
  std::vector<std::string> tickers;

  /**
   * @brief A vector to store expected returns.
   *
   * This vector holds the expected return values for a series of investments or
   * financial instruments. Each element in the vector represents the expected
   * return for a specific investment.
   */
  std::vector<double> expected_returns;

  /**
   * @brief A 2D vector representing the covariance matrix.
   *
   * This member variable stores the covariance matrix, which is a
   * square matrix giving the covariance between each pair of elements
   * in a dataset. The outer vector represents the rows of the matrix,
   * and the inner vector represents the columns.
   */
  std::vector<std::vector<double>> covariance_matrix;

  /**
   * @brief A vector that holds the senses for the NSBRKGA algorithm.
   *
   * This vector contains elements of type NSBRKGA::Sense, which represent
   * the sense (e.g., minimization or maximization) of the objectives in
   * the optimization problem being solved by the NSBRKGA algorithm.
   */
  std::vector<NSBRKGA::Sense> senses;

 private:
  /**
   * @brief Loads the instance data from the given files.
   *
   * This function reads the expected returns and covariance matrix from the
   * specified files and populates the corresponding member variables.
   *
   * @param expected_returns_filename The path to the file containing the
   * expected returns.
   * @param covariance_filename The path to the file containing the covariance
   * matrix.
   *
   * @throws std::runtime_error If either the expected returns file or the
   * covariance file cannot be opened.
   */
  void load_instance(const std::string& expected_returns_filename,
                     const std::string& covariance_filename);

 public:
  /**
   * @brief Constructs an Instance object with the given tickers, expected
   * returns, and covariance matrix.
   *
   * @param tickers A vector of strings representing the asset tickers.
   * @param expected_returns A vector of doubles representing the expected
   * returns for each asset.
   * @param covariance_matrix A 2D vector of doubles representing the covariance
   * matrix of the assets.
   */
  Instance(const std::vector<std::string>& tickers,
           const std::vector<double>& expected_returns,
           const std::vector<std::vector<double>>& covariance_matrix);

  /**
   * @brief Constructs an Instance object and initializes its data members.
   *
   * This constructor initializes the number of assets, tickers, expected
   * returns, covariance matrix, and senses. It then loads the instance data
   * from the specified files.
   *
   * @param returns_filename The filename containing the expected returns data.
   * @param covariance_filename The filename containing the covariance matrix
   * data.
   */
  Instance(const std::string& expected_returns_filename,
           const std::string& covariance_filename);

  /**
   * @brief Copy constructor for the Instance class.
   *
   * This constructor creates a new Instance object by copying the contents
   * of an existing Instance object.
   *
   * @param instance The Instance object to be copied.
   */
  Instance(const Instance& instance);

  /**
   * @brief Default constructor for the Instance class.
   *
   * This constructor initializes an Instance object with default values:
   * - num_assets is set to 0.
   * - tickers is initialized as an empty container.
   * - expected_returns is initialized as an empty container.
   * - covariance_matrix is initialized as an empty container.
   * - senses is initialized as an empty container.
   */
  Instance();

  Instance& operator=(const Instance& instance);

  /**
   * @brief Checks if the instance is valid.
   *
   * This function verifies the validity of the instance by checking the
   * following conditions:
   * - The number of assets (`num_assets`) must be greater than 0.
   * - The size of the `tickers` vector must be equal to the number of assets.
   * - The size of the `expected_returns` vector must be equal to the number of
   * assets.
   * - The size of the `covariance_matrix` vector must be equal to the number of
   * assets.
   * - Each row in the `covariance_matrix` must have a size equal to the number
   * of assets.
   * - The size of the `senses` vector must be equal to 2.
   *
   * @return true if all conditions are met, false otherwise.
   */
  bool is_valid() const;

  /**
   * @brief Overloads the << operator to print the details of an Instance
   * object.
   *
   * This function outputs the number of assets, tickers with their expected
   * returns, and the covariance matrix of the given Instance object to the
   * provided output stream.
   *
   * @param os The output stream to which the Instance details will be written.
   * @param instance The Instance object whose details are to be printed.
   * @return A reference to the output stream.
   */
  friend std::ostream& operator<<(std::ostream& os, const Instance& instance);
};

}  // namespace mopop
