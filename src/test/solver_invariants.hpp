#pragma once

#include <cassert>
#include <cmath>
#include <limits>
#include <tuple>
#include <vector>

#include "solver/solver.hpp"

namespace mopop {

/**
 * @brief Returns the largest single-asset variance of the instance.
 *
 * @param instance The instance.
 * @return The largest value in the diagonal of the covariance matrix.
 */
inline double max_single_asset_variance(const Instance& instance) {
  double max_variance = 0.0;

  for (unsigned i = 0; i < instance.num_assets; i++) {
    if (instance.covariance_matrix[i][i] > max_variance) {
      max_variance = instance.covariance_matrix[i][i];
    }
  }

  return max_variance;
}

/**
 * @brief Asserts the invariants that every archived solution must satisfy.
 *
 * A portfolio whose weights lie in the unit simplex has a variance of at most
 * the largest single-asset variance (the quadratic form of a positive
 * semi-definite covariance matrix is convex, hence maximized at a vertex of the
 * simplex) and a Shannon entropy in [0, log2(num_assets)]. Chromosome entries
 * outside [0, 1] break both bounds, which is how BUG 5 manifested.
 *
 * The weights are also required to sum to 1: a degenerate all-zero chromosome
 * used to decode to an empty portfolio, whose zero variance and zero entropy
 * made it permanently non-dominated in the archive.
 *
 * @param solver The solver whose archive is to be verified.
 */
inline void assert_solver_invariants(const Solver& solver) {
  const double tolerance = std::numeric_limits<float>::epsilon();
  const double max_variance = max_single_asset_variance(solver.instance);

  for (const Solution& solution : solver.best_solutions) {
    assert(solution.is_feasible());

    for (const double& value : solution.value) {
      assert(std::isfinite(value));
    }

    assert(solution.value[1] <= max_variance + tolerance);
    assert(solution.value[3] >= -tolerance);

    double total_weight = 0.0;

    for (const double& weight : solution.weight) {
      total_weight += weight;
    }

    assert(std::fabs(total_weight - 1.0) <= tolerance);
  }

  for (const auto& snapshot : solver.best_solutions_snapshots) {
    for (const std::vector<double>& value : std::get<2>(snapshot)) {
      for (const double& v : value) {
        assert(std::isfinite(v));
      }
    }
  }
}

}  // namespace mopop
