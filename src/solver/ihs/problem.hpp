#pragma once

#include <pagmo/types.hpp>

#include "instance/instance.hpp"

namespace mopop {

class Problem {
 public:
  const Instance instance;

  Problem(const Instance& instance);

  Problem();

  pagmo::vector_double fitness(const pagmo::vector_double& dv) const;

  std::pair<pagmo::vector_double, pagmo::vector_double> get_bounds() const;

  pagmo::vector_double::size_type get_nobj() const;
};

}  // namespace mopop
