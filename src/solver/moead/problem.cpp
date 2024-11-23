#include "solver/moead/problem.hpp"

#include "solution/solution.hpp"

namespace mopop {

Problem::Problem(const Instance& instance) : instance(instance) {}

Problem::Problem() {}

pagmo::vector_double Problem::fitness(const pagmo::vector_double& dv) const {
  Solution solution(this->instance, dv);
  return solution.value;
}

std::pair<pagmo::vector_double, pagmo::vector_double> Problem::get_bounds()
    const {
  return std::make_pair(pagmo::vector_double(this->instance.num_assets, 0.0),
                        pagmo::vector_double(this->instance.num_assets, 1.0));
}

pagmo::vector_double::size_type Problem::get_nobj() const { return 4; }

}  // namespace mopop
