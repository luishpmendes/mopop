#include "solution/solution.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace mopop {

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
bool Solution::dominates(const std::vector<double>& valueA,
                         const std::vector<double>& valueB,
                         const std::vector<NSBRKGA::Sense>& senses) {
  if (valueA.size() != valueB.size()) {
    return false;
  }

  bool at_least_as_good = true, better = false;

  for (std::size_t i = 0; i < valueA.size() && at_least_as_good; i++) {
    if (senses[i] == NSBRKGA::Sense::MINIMIZE) {
      if (valueA[i] > valueB[i] + std::numeric_limits<double>::epsilon()) {
        at_least_as_good = false;
      } else if (valueA[i] <
                 valueB[i] - std::numeric_limits<double>::epsilon()) {
        better = true;
      }
    } else {
      if (valueA[i] < valueB[i] - std::numeric_limits<double>::epsilon()) {
        at_least_as_good = false;
      } else if (valueA[i] >
                 valueB[i] + std::numeric_limits<double>::epsilon()) {
        better = true;
      }
    }
  }

  return at_least_as_good && better;
}

/**
 * @brief Constructs a new Solution object.
 *
 * This constructor initializes the Solution object with the given instance and
 * key. It normalizes the weights based on the key and calculates the expected
 * return and risk.
 *
 * @param instance Reference to an Instance object containing the problem data.
 * @param key A vector of doubles representing the weights for each asset.
 *
 * @throws std::runtime_error if the size of the key does not match the number
 * of assets in the instance.
 */
Solution::Solution(Instance& instance, const std::vector<double>& key)
    : instance(instance), value(2, 0.0), weight(instance.num_assets, 0.0) {
  if (key.size() != instance.num_assets) {
    throw std::runtime_error("Invalid key size");
  }

  double total_weight = 0.0;

  for (unsigned i = 0; i < instance.num_assets; i++) {
    this->weight[i] = key[i];
    total_weight += this->weight[i];
  }

  for (unsigned i = 0; i < instance.num_assets; i++) {
    this->weight[i] /= total_weight;
  }

  for (unsigned i = 0; i < instance.num_assets; i++) {
    this->value[0] += this->weight[i] * instance.expected_returns[i];

    for (unsigned j = 0; j < instance.num_assets; j++) {
      this->value[1] +=
          this->weight[i] * this->weight[j] * instance.covariance_matrix[i][j];
    }
  }
}

/**
 * @brief Constructs a Solution object by reading asset weights from a file.
 *
 * This constructor initializes the Solution object with the given instance and
 * reads asset weights from the specified file. The file is expected to have a
 * specific format where each line contains an asset ticker and its
 * corresponding weight separated by a comma.
 *
 * @param instance Reference to an Instance object containing the number of
 * assets.
 * @param filename The name of the file containing asset weights.
 *
 * @throws std::runtime_error If the file cannot be opened.
 */
Solution::Solution(Instance& instance, const std::string& filename)
    : instance(instance), value(2, 0.0), weight(instance.num_assets, 0.0) {
  std::ifstream file(filename);
  std::string line;

  if (!file.is_open()) {
    throw std::runtime_error("Unable to open file");
  }

  std::getline(file, line);

  for (unsigned i = 0; i < instance.num_assets; i++) {
    std::getline(file, line);
    std::istringstream linestream(line);
    std::string ticker, weight_str;

    if (std::getline(linestream, ticker, ',') &&
        std::getline(linestream, weight_str, ',')) {
      this->weight[i] = std::stod(weight_str);
    }
  }
}

/**
 * @brief Constructs a new Solution object.
 *
 * This constructor initializes a Solution object using the provided Instance
 * reference.
 *
 * @param instance A reference to an Instance object that contains the data
 * needed to initialize the Solution.
 */
Solution::Solution(Instance& instance)
    : instance(instance), value(2, 0.0), weight(instance.num_assets, 0.0) {}

/**
 * @brief Default constructor for the Solution class.
 *
 * This constructor initializes a Solution object with the following default
 * values:
 * - `instance`: A new Instance object.
 * - `value`: A vector of size 2, initialized with 0.0.
 * - `weight`: Initialized to 0.
 */
Solution::Solution() : instance(*(new Instance())), value(2, 0.0), weight(0) {}

/**
 * @brief Assignment operator for the Solution class.
 *
 * This operator assigns the values from another Solution object to the current
 * object. It performs a deep copy of the instance, value, and weight members.
 *
 * @param solution The Solution object to be copied.
 * @return A reference to the current Solution object.
 */
Solution& Solution::operator=(const Solution& solution) {
  if (this != &solution) {
    this->instance = solution.instance;
    this->value = solution.value;
    this->weight = solution.weight;
  }

  return *this;
}

/**
 * @brief Checks if the solution is feasible.
 *
 * This function verifies the feasibility of the solution by performing the
 * following checks:
 * 1. Ensures the instance is valid.
 * 2. Ensures the value vector has exactly 2 elements.
 * 3. Ensures the weight vector has the same number of elements as the number of
 * assets in the instance.
 * 4. Ensures each weight is between 0.0 and 1.0 (inclusive).
 * 5. Ensures the sum of all weights does not exceed 1.0.
 *
 * @return true if the solution is feasible, false otherwise.
 */
bool Solution::is_feasible() const {
  if (!this->instance.is_valid()) {
    return false;
  }

  if (this->value.size() != 2) {
    return false;
  }

  if (this->weight.size() != this->instance.num_assets) {
    return false;
  }

  for (unsigned j = 0; j < this->instance.num_assets; j++) {
    if (this->weight[j] < 0.0) {
      return false;
    }

    if (this->weight[j] > 1.0) {
      return false;
    }
  }

  double sum_weight = 0;

  for (unsigned i = 0; i < this->instance.num_assets; i++) {
    sum_weight += this->weight[i];
  }

  if (sum_weight > 1.0) {
    return false;
  }

  return true;
}

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
bool Solution::dominates(const Solution& solution) const {
  return Solution::dominates(this->value, solution.value,
                             this->instance.senses);
}

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
 * solution, and weight1, weight2, ..., weightN are their corresponding weights.
 */
std::ostream& operator<<(std::ostream& os, const Solution& solution) {
  os << "Ticker,0" << std::endl;

  for (unsigned i = 0; i < solution.instance.num_assets; i++) {
    os << solution.instance.tickers[i] << "," << solution.weight[i]
       << std::endl;
  }

  return os;
}

}  // namespace mopop
