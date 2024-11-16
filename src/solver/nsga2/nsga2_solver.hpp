#pragma once

#include "solver/solver.hpp"

namespace mopop {
/**
 * @class NSGA2_Solver
 * @brief The NSGA2_Solver represents a solver for the Multi-Objective Portfolio
 * Optimization Problem using the Non-Dominated Sorting Genetic Algorithm II.
 */
class NSGA2_Solver : public Solver {
 public:
  /**
   * @brief The size of the population.
   */
  unsigned population_size = 300;

  /**
   * @brief The crossover probability.
   */
  double crossover_probability = 0.95;

  /**
   * @brief The distribution index for crossover.
   */
  double crossover_distribution = 10.00;

  /**
   * @brief The mutation probability.
   */
  double mutation_probability = 0.01;

  /**
   * @brief The distribution index for mutation.
   */
  double mutation_distribution = 50.00;

  /**
   * @brief Constructs a new solver.
   *
   * @param instance The instance to be solved.
   */
  NSGA2_Solver(const Instance& instance);

  /**
   * @brief Constructs a new empty solver.
   */
  NSGA2_Solver();

  /**
   * @brief Solves the instance.
   */
  void solve();

  /**
   * @brief Standard stream operator.
   *
   * @param os The standard output stream object.
   * @param solver The solver.
   * @return The stream object.
   */
  friend std::ostream& operator<<(std::ostream& os, const NSGA2_Solver& solver);
};

}  // namespace mopop
