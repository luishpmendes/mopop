#pragma once

#include "solver/solver.hpp"

namespace mopop {
/**
 * @class MHACO_Solver
 * @brief The MHACO_Solver represents a solver for the Multi-Objective Portfolio
 * Optimization Problem using the Multi-Objective Hypervolume-based Ant Colony
 * Optimizer.
 */
class MHACO_Solver : public Solver {
 public:
  /**
   * @brief The size of the population.
   */
  unsigned population_size = 300;

  /**
   * @brief The number of solutions stored in the solution archive (which is
   * maintained independently from the population).
   */
  unsigned ker = 63;

  /**
   * @brief This parameter is called convergence speed parameter, and it is
   * useful for managing the convergence speed towards the best found solution
   * (in terms of non-dominated front and hypervolume value). The smaller the
   * parameter, the faster the convergence and the higher the chance to get
   * stuck to local minima.
   */
  double q = 1.0;

  /**
   * @brief When the generations reach the threshold, then q is set to 0.01
   * automatically, thus increasing consistently the convergence speed towards
   * the best found value.
   */
  unsigned threshold = 1;

  /**
   * @brief This parameter regulates the convergence speed of the standard
   * deviation values.
   */
  unsigned n_gen_mark = 7;

  /**
   * @brief If a positive integer is assigned here, the algorithm will count the
   * runs without improvements (in terms of ideal point), if this number will
   * exceed the eval_stop value, the algorithm will be stopped and will return
   * the evolved population until than moment.
   */
  unsigned eval_stop = 0;

  /**
   * @brief This parameter makes the search for the optimum greedier and more
   * focused on local improvements (the higher the greedier). If the value is
   * very high, the search is more focused around the currently found best
   * solutions.
   */
  double focus = 0.0;

  /**
   * @brief The memory parameter. If true, memory is activated in the algorithm
   * for multiple calls.
   */
  bool memory = true;

  /**
   * @brief Constructs a new solver.
   *
   * @param instance The instance to be solved.
   */
  MHACO_Solver(const Instance& instance);

  /**
   * @brief Constructs a new empty solver.
   */
  MHACO_Solver();

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
  friend std::ostream& operator<<(std::ostream& os, const MHACO_Solver& solver);
};

}  // namespace mopop
