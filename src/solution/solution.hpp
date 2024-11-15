#pragma once

#include "instance/instance.hpp"

namespace mopop {
class Solution {
 public:
  static bool dominates(const std::vector<double>& valueA,
                        const std::vector<double>& valueB,
                        const std::vector<NSBRKGA::Sense>& senses);

  Instance& instance;

  std::vector<double> weight;

  std::vector<double> value;

  Solution(Instance& instance, const std::vector<double>& key);

  Solution(Instance& instance, const std::string& filename);

  Solution(Instance& instance);

  Solution();

  Solution& operator=(const Solution& solution);

  bool is_feasible() const;

  bool dominates(const Solution& solution) const;

  friend std::istream& operator>>(std::istream& is, Solution& solution);

  friend std::ostream& operator<<(std::ostream& os, const Solution& solution);
};

}  // namespace mopop
