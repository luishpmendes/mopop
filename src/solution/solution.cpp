#include "solution/solution.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace mopop {

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
      this->value[1] += this->weight[i] * this->weight[j] *
                        instance.covariance_matrix[i][j];
    }
  }
}

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

Solution::Solution(Instance& instance)
    : instance(instance), value(2, 0.0), weight(instance.num_assets, 0.0) {}

Solution::Solution() : instance(*(new Instance())), value(2, 0.0), weight(0) {}

Solution& Solution::operator=(const Solution& solution) {
  if (this != &solution) {
    this->instance = solution.instance;
    this->value = solution.value;
    this->weight = solution.weight;
  }

  return *this;
}

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

bool Solution::dominates(const Solution& solution) const {
  return Solution::dominates(this->value, solution.value,
                             this->instance.senses);
}

std::ostream& operator<<(std::ostream& os, const Solution& solution) {
  os << "Ticker,0" << std::endl;

  for (unsigned i = 0; i < solution.instance.num_assets; i++) {
    os << solution.instance.tickers[i] << "," << solution.weight[i]
       << std::endl;
  }

  return os;
}

}  // namespace mopop
