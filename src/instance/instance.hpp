#pragma once

#define NSBRKGA_MULTIPLE_INCLUSIONS

#include <istream>
#include <ostream>
#include <vector>

#include "nsbrkga.hpp"

namespace mopop {
/**
 * @class Instance
 * @brief The Instance class represents an instance of the Multi-Objective
 * Portfolio Optimization Problem.
 */
class Instance {
 public:
  /**
   * @brief The number of assets.
   */
  unsigned num_assets;

  /**
   * @brief The vector that stores a list of ticker symbols.
   */
  std::vector<std::string> tickers;

  /**
   * @brief The vector to store expected returns.
   */
  std::vector<double> expected_returns;

  /**
   * @brief The 2D vector to store the covariance matrix.
   */
  std::vector<std::vector<double>> covariance_matrix;

  /**
   * @brief The vector that holds the senses for the optimization algorithm.
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

  /**
   * @brief Assignment operator for the Instance class.
   *
   * This operator assigns the values from the given instance to the current
   * instance. It performs a deep copy of the member variables to ensure that
   * the current instance has the same values as the given instance.
   *
   * @param instance The instance to be copied.
   * @return A reference to the current instance after assignment.
   */
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
   * - The size of the `senses` vector must be equal to 4.
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
