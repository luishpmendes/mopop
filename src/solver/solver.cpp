#include "solver/solver.hpp"

namespace mopop {
/**
 * @brief Constructs a new solver.
 *
 * @param instance The instance to be solved.
 */
Solver::Solver(const Instance& instance) : instance(instance) {
  this->set_seed(this->seed);
}

/**
 * @brief Constructs a new empty solver.
 */
Solver::Solver() = default;

/**
 * @brief Sets the seed for the pseudo-random numbers generator.
 *
 * @param seed The new seed for the pseudo-random numbers generator.
 */
void Solver::set_seed(unsigned seed) {
  this->seed = seed;
  this->rng.seed(this->seed);
  this->rng.discard(10000);
}

/**
 * @brief Returns the elapsed time in seconds.
 *
 * @param start_time The start time
 * @return The elapsed time in seconds.
 */
double Solver::elapsed_time(
    const std::chrono::steady_clock::time_point& start_time) {
  std::chrono::steady_clock::time_point current_time =
      std::chrono::steady_clock::now();
  std::chrono::nanoseconds elapsed_time =
      std::chrono::duration_cast<std::chrono::nanoseconds>(current_time -
                                                           start_time);
  return (double(elapsed_time.count())) / (double(1e9));
}

/**
 * @brief Returns the elapsed time in seconds.
 *
 * @return The elapsed time in seconds.
 */
double Solver::elapsed_time() const {
  return Solver::elapsed_time(this->start_time);
}

/**
 * @brief Returns the remaining time in seconds.
 *
 * @param start_time The start time.
 * @param time_limit The time limit in seconds.
 * @return The remaining time in seconds.
 */
double Solver::remaining_time(
    const std::chrono::steady_clock::time_point& start_time,
    double time_limit) {
  return time_limit - Solver::elapsed_time(start_time);
}

/**
 * @brief Returns the remaining time in seconds.
 *
 * @return The remaining time in seconds.
 */
double Solver::remaining_time() const {
  return Solver::remaining_time(this->start_time, this->time_limit);
}

/**
 * @brief Verifies whether the termination criteria have been met.
 *
 * @return true if the termination criteria have been met; false otherwise.
 */
bool Solver::are_termination_criteria_met() const {
  return (this->elapsed_time() >= this->time_limit ||
          this->num_iterations >= this->iterations_limit);
}

/**
 * @brief Updates the best individuals found so far.
 *
 * @param best_individuals The best individuals found so far.
 * @param new_individuals The new individuals found.
 * @param senses The optimisation senses.
 * @return true if the best individual are modified; false otherwise.
 */
bool Solver::update_best_individuals(
    std::vector<std::pair<std::vector<double>, std::vector<double>>>&
        best_individuals,
    const std::vector<std::pair<std::vector<double>, std::vector<double>>>&
        new_individuals,
    const std::vector<NSBRKGA::Sense>& senses) {
  bool result = false;

  if (new_individuals.empty()) {
    return result;
  }

  auto non_dominated_new_individuals =
      NSBRKGA::Population::nonDominatedSort<std::vector<double>>(
          new_individuals, senses)
          .front();

  for (const auto& new_individual : non_dominated_new_individuals) {
    bool is_dominated_or_equal = false;

    for (auto it = best_individuals.begin(); it != best_individuals.end();) {
      auto individual = *it;

      if (Solution::dominates(new_individual.first, individual.first, senses)) {
        it = best_individuals.erase(it);
      } else {
        if (Solution::dominates(individual.first, new_individual.first,
                                senses) ||
            std::equal(individual.first.begin(), individual.first.end(),
                       new_individual.first.begin(),
                       [](const double& a, const double& b) {
                         return fabs(a - b) <
                                std::numeric_limits<double>::epsilon();
                       })) {
          is_dominated_or_equal = true;
          break;
        }

        it++;
      }
    }

    if (!is_dominated_or_equal) {
      best_individuals.push_back(new_individual);
      result = true;
    }
  }

  return result;
}

/**
 * @brief Updates the best individuals found so far.
 *
 * @param best_individuals The best individuals found so far.
 * @param new_individuals The new individuals found.
 * @param senses The optimisation senses.
 * @param max_num_solutions The maximum number of solutions.
 * @return true if the best individual are modified; false otherwise.
 */
bool Solver::update_best_individuals(
    std::vector<std::pair<std::vector<double>, std::vector<double>>>&
        best_individuals,
    const std::vector<std::pair<std::vector<double>, std::vector<double>>>&
        new_individuals,
    const std::vector<NSBRKGA::Sense>& senses, unsigned max_num_solutions) {
  bool result = Solver::update_best_individuals(best_individuals,
                                                new_individuals, senses);

  if (best_individuals.size() > max_num_solutions) {
    NSBRKGA::Population::crowdingSort<std::vector<double>>(best_individuals);
    best_individuals.resize(max_num_solutions);
    result = true;
  }

  return result;
}

/**
 * @brief Updates the best individuals found so far.
 *
 * @param new_individuals The new individuals found.
 * @return true if the best individual are modified; false otherwise.
 */
bool Solver::update_best_individuals(
    const std::vector<std::pair<std::vector<double>, std::vector<double>>>&
        new_individuals) {
  bool result = Solver::update_best_individuals(
      this->best_individuals, new_individuals, this->instance.senses);

  if (this->best_individuals.size() > this->max_num_solutions) {
    NSBRKGA::Population::crowdingSort<std::vector<double>>(
        this->best_individuals);
    this->best_individuals.resize(this->max_num_solutions);
    result = true;
  }

  return result;
}

/**
 * @brief Updates the best individuals found so far.
 *
 * @param pop The new solutions.
 * @return true if the best individuals are modified; false otherwise.
 */
bool Solver::update_best_individuals(const pagmo::population& pop) {
  std::vector<std::pair<std::vector<double>, std::vector<double>>>
      new_individuals(pop.size());

  for (std::size_t i = 0; i < pop.size(); i++) {
    new_individuals[i] = std::make_pair(pop.get_f()[i], pop.get_x()[i]);
  }

  return this->update_best_individuals(new_individuals);
}

/**
 * @brief Captures a snapshot of the current population.
 *
 * @param pop The current population.
 */
void Solver::capture_snapshot(const pagmo::population& pop) {
  double time_snapshot = this->elapsed_time();

  this->best_solutions_snapshots.emplace_back(std::make_tuple(
      this->num_iterations, time_snapshot,
      std::vector<std::vector<double>>(this->best_individuals.size())));

  for (std::size_t i = 0; i < this->best_individuals.size(); i++) {
    std::get<2>(this->best_solutions_snapshots.back())[i] =
        this->best_individuals[i].first;
  }

  f = pop.get_f();
  this->current_individuals.resize(pop.size());

  for (std::size_t i = 0; i < pop.size(); i++) {
    this->current_individuals[i] = std::make_pair(f[i], pop.get_x()[i]);
  }

  this->fronts = NSBRKGA::Population::nonDominatedSort<std::vector<double>>(
      current_individuals, this->instance.senses);
  this->num_non_dominated_snapshots.push_back(
      std::make_tuple(this->num_iterations, time_snapshot,
                      std::vector<unsigned>(1, fronts.front().size())));
  this->num_fronts_snapshots.push_back(
      std::make_tuple(this->num_iterations, time_snapshot,
                      std::vector<unsigned>(1, fronts.size())));
  this->populations_snapshots.push_back(
      std::make_tuple(this->num_iterations, time_snapshot,
                      std::vector<std::vector<std::vector<double>>>(1, f)));
  this->time_last_snapshot = time_snapshot;
  this->iteration_last_snapshot = this->num_iterations;
  this->num_snapshots++;
}

/**
 * @brief Standard stream operator.
 *
 * @param os The standard output stream object.
 * @param solver The solver.
 * @return The stream object.
 */
std::ostream& operator<<(std::ostream& os, const Solver& solver) {
  os << "Number of assets: " << solver.instance.num_assets << std::endl
     << "Seed: " << solver.seed << std::endl
     << "Time limit: " << solver.time_limit << std::endl
     << "Iterations limit: " << solver.iterations_limit << std::endl
     << "Maximum number of solutions: " << solver.max_num_solutions << std::endl
     << "Maximum number of snapshots: " << solver.max_num_snapshots << std::endl
     << "Factor at which the time between snapshots are increased: "
     << solver.time_snapshot_factor << std::endl
     << "Factor at which the iterations between snapshots are increased: "
     << solver.iteration_snapshot_factor << std::endl
     << "Number of iterations: " << solver.num_iterations << std::endl
     << "Solutions obtained: " << solver.best_solutions.size() << std::endl
     << "Solving time: " << solver.solving_time << std::endl
     << "Number of snapshots: " << solver.num_snapshots << std::endl
     << "Time next snapshot: " << solver.time_next_snapshot << std::endl
     << "Time when the last snapshot was taken: " << solver.time_last_snapshot
     << std::endl
     << "Number of iteration of the next snapshot: "
     << solver.iteration_next_snapshot << std::endl
     << "Iteration when the last snapshot was taken: "
     << solver.iteration_last_snapshot << std::endl;
  return os;
}

}  // namespace mopop
