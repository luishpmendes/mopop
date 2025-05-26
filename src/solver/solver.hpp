#pragma once

#include <pagmo/population.hpp>

#include "solution/solution.hpp"

namespace mopop {
class Solver {
 public:
  /**
   * @brief The instance been solved.
   */
  const Instance instance;

  /**
   * @brief The seed for the pseudo-random numbers generator.
   */
  unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();

  /**
   * @brief The pseudo-random number generator.
   */
  std::mt19937 rng;

  /**
   * @brief The time limit in seconds.
   */
  double time_limit = std::numeric_limits<double>::max();

  /**
   * @brief The iterations limit.
   */
  unsigned iterations_limit = std::numeric_limits<unsigned>::max();

  /**
   * @brief The maximum number of solutions.
   */
  unsigned max_num_solutions = std::numeric_limits<unsigned>::max();

  /**
   * @brief The maximum number of snapshots to take during optimization.
   */
  unsigned max_num_snapshots = 0;

  /**
   * @brief The number of iterations executed.
   */
  unsigned num_iterations = 0;

  /**
   * @brief The best individuals found.
   */
  std::vector<std::pair<std::vector<double>, std::vector<double>>>
      best_individuals = {};

  /**
   * @brief The solutions found.
   */
  std::vector<Solution> best_solutions = {};

  /**
   * @brief The solving time in seconds.
   */
  double solving_time = 0.0;

  /**
   * @brief The number of snapshots taken during optimization.
   */
  unsigned num_snapshots = 0;

  /**
   * @brief The factor at which the time snapshots are increased.
   */
  double time_snapshot_factor = 1.0;

  /**
   * @brief The factor at which the iterations snapshots are increased.
   */
  double iteration_snapshot_factor = 1.0;

  /**
   * @brief The time when the next snapshot will be taken.
   */
  double time_next_snapshot = 0.0;

  /**
   * @brief The time when the last snapshot was taken.
   */
  double time_last_snapshot = 0.0;

  /**
   * @brief The iteration when the next snapshot will be taken.
   */
  unsigned iteration_next_snapshot = 0;

  /**
   * @brief The iteration when the last snapshot was taken.
   */
  unsigned iteration_last_snapshot = 0;

  /**
   * @brief The snapshots of the best solutions, containing the iteration, time
   * and Solutions' costs.
   */
  std::vector<std::tuple<unsigned, double, std::vector<std::vector<double>>>>
      best_solutions_snapshots = {};

  /**
   * @brief The snapshots of the number of non-dominated individuals in each
   * population, containing the iterations, time and number of non-dominated
   * individuals in each population.
   */
  std::vector<std::tuple<unsigned, double, std::vector<unsigned>>>
      num_non_dominated_snapshots = {};

  /**
   * @brief The snapshots of the number of non-dominated fronts in each
   * population, containing the iterations, time and number of non-dominated
   * fronts in each population.
   */
  std::vector<std::tuple<unsigned, double, std::vector<unsigned>>>
      num_fronts_snapshots = {};

  /**
   * @brief The snapshots of the populations, containing the iteration, time and
   * Solutions' costs.
   */
  std::vector<std::tuple<unsigned, double,
                         std::vector<std::vector<std::vector<double>>>>>
      populations_snapshots = {};

  /**
   * @brief The start time.
   */
  std::chrono::steady_clock::time_point start_time;

  /**
   * @brief The current individuals.
   */
  std::vector<std::pair<std::vector<double>, std::vector<double>>>
      current_individuals;

  /**
   * @brief The current fronts.
   */
  std::vector<std::vector<std::pair<std::vector<double>, std::vector<double>>>>
      fronts;

  /**
   * @brief The fitnesses of the current individuals.
   */
  std::vector<std::vector<double>> f;

  /**
   * @brief Constructs a new solver.
   *
   * @param instance The instance to be solved.
   */
  Solver(const Instance& instance);

  /**
   * @brief Constructs a new empty solver.
   */
  Solver();

  /**
   * @brief Sets the seed for the pseudo-random numbers generator.
   *
   * @param seed The new seed for the pseudo-random numbers generator.
   */
  void set_seed(unsigned seed);

  /**
   * @brief Returns the elapsed time in seconds.
   *
   * @param start_time The start time
   * @return The elapsed time in seconds.
   */
  static double elapsed_time(
      const std::chrono::steady_clock::time_point& start_time);

  /**
   * @brief Returns the elapsed time in seconds.
   *
   * @return The elapsed time in seconds.
   */
  double elapsed_time() const;

  /**
   * @brief Returns the remaining time in seconds.
   *
   * @param start_time The start time.
   * @param time_limit The time limit in seconds.
   * @return The remaining time in seconds.
   */
  static double remaining_time(
      const std::chrono::steady_clock::time_point& start_time,
      double time_limit);

  /**
   * @brief Returns the remaining time in seconds.
   *
   * @return The remaining time in seconds.
   */
  double remaining_time() const;

  /**
   * @brief Verifies whether the termination criteria have been met.
   *
   * @return true if the termination criteria have been met; false otherwise.
   */
  bool are_termination_criteria_met() const;

  /**
   * @brief Updates the best individuals found so far.
   *
   * @param best_individuals The best individuals found so far.
   * @param new_individuals The new individuals found.
   * @param senses The optimisation senses.
   * @return true if the best individual are modified; false otherwise.
   */
  static bool update_best_individuals(
      std::vector<std::pair<std::vector<double>, std::vector<double>>>&
          best_individuals,
      const std::vector<std::pair<std::vector<double>, std::vector<double>>>&
          new_individuals,
      const std::vector<NSBRKGA::Sense>& senses);

  /**
   * @brief Updates the best individuals found so far.
   *
   * @param best_individuals The best individuals found so far.
   * @param new_individuals The new individuals found.
   * @param senses The optimisation senses.
   * @param max_num_solutions The maximum number of solutions.
   * @return true if the best individual are modified; false otherwise.
   */
  static bool update_best_individuals(
      std::vector<std::pair<std::vector<double>, std::vector<double>>>&
          best_individuals,
      const std::vector<std::pair<std::vector<double>, std::vector<double>>>&
          new_individuals,
      const std::vector<NSBRKGA::Sense>& senses, unsigned max_num_solutions);

  /**
   * @brief Updates the best individuals found so far.
   *
   * @param new_individuals The new individuals found.
   * @return true if the best individual are modified; false otherwise.
   */
  bool update_best_individuals(
      const std::vector<std::pair<std::vector<double>, std::vector<double>>>&
          new_individuals);

  /**
   * @brief Updates the best individuals found so far.
   *
   * @param pop The new solutions.
   * @return true if the best individuals are modified; false otherwise.
   */
  bool update_best_individuals(const pagmo::population& pop);

  /**
   * @brief Captures a snapshot of the current population.
   *
   * @param pop The current population.
   */
  void capture_snapshot(const pagmo::population& pop);

  /**
   * @brief Solves the instance.
   */
  virtual void solve() = 0;

  /**
   * @brief Standard stream operator.
   *
   * @param os The standard output stream object.
   * @param solver The solver.
   * @return The stream object.
   */
  friend std::ostream& operator<<(std::ostream& os, const Solver& solver);
};

}  // namespace mopop
