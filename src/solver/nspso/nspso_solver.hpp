#pragma once

#include "solver/solver.hpp"

namespace mopop {
/**
 * @class NSPSO_Solver
 * @brief The NSPSO_Solver represents a solver for the Multi-Objective
 * Travelling Salesman Problem using the Non-Dominated Sorting Particle Swarm
 * Optmizer.
 */
class NSPSO_Solver : public Solver {
 public:
  /**
   * @brief The size of the population.
   */
  unsigned population_size = 300;

  /**
   * @brief The particles' inertia weight.
   */
  double omega = 0.6;

  /**
   * @brief The magnitude of the force, applied to the particle’s velocity, in
   * the direction of its previous best position.
   */
  double c1 = 2.0;

  /**
   * @brief The magnitude of the force, applied to the particle’s velocity, in
   * the direction of its global best (i.e., leader).
   */
  double c2 = 2.0;

  /**
   * @brief The velocity scaling factor.
   */
  double chi = 1.0;

  /**
   * @brief The velocity coefficient (determining the maximum allowed particle
   * velocity).
   */
  double v_coeff = 0.5;

  /**
   * @brief Leader selection range parameter (i.e., the leader of each particle
   * is selected among the best leader_selection_range % individuals).
   */
  unsigned leader_selection_range = 60;

  /**
   * @brief The diversity mechanism used to maintain diversity on the Pareto
   * front.
   */
  std::string diversity_mechanism = "crowding distance";

  /**
   * @brief Memory parameter. If true, memory is activated in the algorithm for
   * multiple calls.
   */
  bool memory = true;

  /**
   * @brief Constructs a new solver.
   *
   * @param instance The instance to be solved.
   */
  NSPSO_Solver(const Instance& instance);

  /**
   * @brief Constructs an empty solver.
   */
  NSPSO_Solver();

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
  friend std::ostream& operator<<(std::ostream& os, const NSPSO_Solver& solver);
};

}  // namespace mopop
