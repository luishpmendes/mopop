#pragma once

#include "instance/instance.hpp"

namespace mopop {
/**
 * @class Solution
 * @brief Represents a solution for an optimization problem.
 *
 * The Solution class encapsulates the data and methods required to represent
 * and manipulate a solution for a given optimization problem. It includes
 * methods to check domination between solutions, verify feasibility, and
 * perform various initializations.
 *
 * @details
 * The Solution class provides multiple constructors to initialize a solution
 * object using different data sources, such as vectors of weights or files
 * containing asset weights. It also includes methods to check if one solution
 * dominates another, verify the feasibility of a solution, and output the
 * solution to an ostream.
 *
 * The class contains the following members:
 * - `instance`: A reference to an Instance object containing the problem data.
 * - `weight`: A vector of double values representing the weights of assets.
 * - `value`: A vector of double values representing the solution's values.
 *
 * The class provides the following methods:
 * - `dominates(const std::vector<double>&, const std::vector<double>&, const std::vector<NSBRKGA::Sense>&)`: 
 *   Checks if one vector of values dominates another based on given senses.
 * - `Solution(Instance&, const std::vector<double>&)`: Constructs a Solution object with the given instance and key.
 * - `Solution(Instance&, const std::string&)`: Constructs a Solution object by reading asset weights from a file.
 * - `Solution(Instance&)`: Constructs a Solution object using the provided Instance reference.
 * - `Solution()`: Default constructor for the Solution class.
 * - `operator=(const Solution&)`: Assignment operator for the Solution class.
 * - `is_feasible() const`: Checks if the solution is feasible.
 * - `dominates(const Solution&) const`: Determines if the current solution dominates another solution.
 * - `operator<<(std::ostream&, const Solution&)`: Overloads the << operator to output the Solution object to an ostream.
 */
class Solution {
 public:
  /**
   * @brief Determines if one vector of values dominates another based on given
   * senses.
   *
   * This function checks if `valueA` dominates `valueB` according to the
   * optimization senses provided. Domination is defined as `valueA` being at
   * least as good as `valueB` in all objectives and strictly better in at least
   * one objective.
   *
   * @param valueA A vector of double values representing the first solution.
   * @param valueB A vector of double values representing the second solution.
   * @param senses A vector of NSBRKGA::Sense enums indicating whether each
   * objective should be minimized or maximized.
   * @return true if `valueA` dominates `valueB`, false otherwise.
   */
  static bool dominates(const std::vector<double>& valueA,
                        const std::vector<double>& valueB,
                        const std::vector<NSBRKGA::Sense>& senses);

  /**
   * @brief Reference to an Instance object.
   */
  Instance& instance;

  /**
   * @brief A vector to store weights as double precision floating point
   * numbers.
   */
  std::vector<double> weight;

  /**
   * @brief A vector to store double precision floating point values.
   */
  std::vector<double> value;

  /**
   * @brief Constructs a new Solution object.
   *
   * This constructor initializes the Solution object with the given instance
   * and key. It normalizes the weights based on the key and calculates the
   * expected return and risk.
   *
   * @param instance Reference to an Instance object containing the problem
   * data.
   * @param key A vector of doubles representing the weights for each asset.
   *
   * @throws std::runtime_error if the size of the key does not match the number
   * of assets in the instance.
   */
  Solution(Instance& instance, const std::vector<double>& key);

  /**
   * @brief Constructs a Solution object by reading asset weights from a file.
   *
   * This constructor initializes the Solution object with the given instance
   * and reads asset weights from the specified file. The file is expected to
   * have a specific format where each line contains an asset ticker and its
   * corresponding weight separated by a comma.
   *
   * @param instance Reference to an Instance object containing the number of
   * assets.
   * @param filename The name of the file containing asset weights.
   *
   * @throws std::runtime_error If the file cannot be opened.
   */
  Solution(Instance& instance, const std::string& filename);

  /**
   * @brief Constructs a new Solution object.
   *
   * This constructor initializes a Solution object using the provided Instance
   * reference.
   *
   * @param instance A reference to an Instance object that contains the data
   * needed to initialize the Solution.
   */
  Solution(Instance& instance);

  /**
   * @brief Default constructor for the Solution class.
   *
   * This constructor initializes a Solution object with the following default
   * values:
   * - `instance`: A new Instance object.
   * - `value`: A vector of size 2, initialized with 0.0.
   * - `weight`: Initialized to 0.
   */
  Solution();

  /**
   * @brief Assignment operator for the Solution class.
   *
   * This operator assigns the values from another Solution object to the
   * current object. It performs a deep copy of the instance, value, and weight
   * members.
   *
   * @param solution The Solution object to be copied.
   * @return A reference to the current Solution object.
   */
  Solution& operator=(const Solution& solution);

  /**
   * @brief Checks if the solution is feasible.
   *
   * This function verifies the feasibility of the solution by performing the
   * following checks:
   * 1. Ensures the instance is valid.
   * 2. Ensures the value vector has exactly 2 elements.
   * 3. Ensures the weight vector has the same number of elements as the number
   * of assets in the instance.
   * 4. Ensures each weight is between 0.0 and 1.0 (inclusive).
   * 5. Ensures the sum of all weights does not exceed 1.0.
   *
   * @return true if the solution is feasible, false otherwise.
   */
  bool is_feasible() const;

  /**
   * @brief Determines if the current solution dominates another solution.
   *
   * This function checks if the current solution instance dominates the given
   * solution based on their values and the instance's senses.
   *
   * @param solution The solution to compare against the current solution.
   * @return true if the current solution dominates the given solution, false
   * otherwise.
   */
  bool dominates(const Solution& solution) const;

  /**
   * Overloads the << operator to output the Solution object to an ostream.
   *
   * @param os The output stream to which the Solution object will be written.
   * @param solution The Solution object to be written to the output stream.
   * @return A reference to the output stream.
   *
   * The output format is:
   * Ticker,0
   * ticker1,weight1
   * ticker2,weight2
   * ...
   * tickerN,weightN
   *
   * Where ticker1, ticker2, ..., tickerN are the tickers of the assets in the
   * solution, and weight1, weight2, ..., weightN are their corresponding
   * weights.
   */
  friend std::ostream& operator<<(std::ostream& os, const Solution& solution);
};

}  // namespace mopop
