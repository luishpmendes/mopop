#pragma once

#include "solver/solver.hpp"

namespace mopop {
/**
 * @class MOEAD_Solver
 * @brief The MOEAD_Solver represents a solver for the Multi-Objective Portfolio
 * Optimization Problem using the Multi-objective Evolutionary Algorithm by
 * Decomposition.
 */
class MOEAD_Solver : public Solver {
 public:
  /**
   * @brief The size of the population.
   */
  unsigned population_size = 300;

  /**
   * @brief The method used to generate the weights, one of “grid”, “low
   * discrepancy” or “random”.
   */
  std::string weight_generation = "random";

  /**
   * @brief The decomposition method: one of “weighted”, “tchebycheff” or “bi”.
   */
  std::string decomposition = "tchebycheff";

  /**
   * @brief The size of the weight’s neighborhood.
   */
  unsigned neighbours = 20;

  /**
   * @brief The crossover parameter in the Differential Evolution operator.
   */
  double cr = 1.0;

  /**
   * @brief The parameter for the Differential Evolution operator.
   */
  double f = 0.5;

  /**
   * @brief The distribution index used by the polynomial mutation.
   */
  double eta_m = 20.0;

  /**
   * @brief The chance that the neighbourhood is considered at each generation,
   * rather than the whole population (only if preserve_diversity is true).
   */
  double realb = 0.9;

  /**
   * @brief The maximum number of copies reinserted in the population (only if
   * m_preserve_diversity is true).
   */
  unsigned limit = 2;

  /**
   * @brief When true activates the two diversity preservation mechanisms
   * described in Li, Hui, and Qingfu Zhang paper.
   */
  bool preserve_diversity = true;

  /**
   * @brief Constructs a new solver.
   *
   * @param instance The instance to be solved.
   */
  MOEAD_Solver(const Instance& instance);

  /**
   * @brief Constructs a new empty solver.
   */
  MOEAD_Solver();

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
  friend std::ostream& operator<<(std::ostream& os, const MOEAD_Solver& solver);
};

}  // namespace mopop
