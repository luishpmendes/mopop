#pragma once

#include "solver/solver.hpp"

namespace mopop {
/**
 * @class IHS_Solver
 * @brief The IHS_Solver represents a solver for the Multi-Objective Portfolio Optimization Problem using the Improved Harmony Search.
 */
class IHS_Solver : public Solver {
 public:
  /**
   * @brief The size of the population.
   */
  unsigned population_size = 300;

  /**
   * @brief The probability of choosing from memory.
   */
  double phmcr = 0.85;

  /**
   * @brief The minimum pitch adjustment rate.
   */
  double ppar_min = 0.35;

  /**
   * @brief The maximum pitch adjustment rate.
   */
  double ppar_max = 0.99;

  /**
   * @brief The minimum distance bandwidth.
   */
  double bw_min = 1E-5;

  /**
   * @brief The maximum distance bandwidth.
   */
  double bw_max = 1.0;

  /**
   * @brief Constructs a new solver.
   *
   * @param instance The instance to be solved.
   */
  IHS_Solver(const Instance& instance);

  /**
   * @brief Constructs a new empty solver.
   */
  IHS_Solver();

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
  friend std::ostream& operator<<(std::ostream& os, const IHS_Solver& solver);
};

}  // namespace mopop
