#include "solver/ihs/ihs_solver.hpp"

#include <pagmo/algorithms/ihs.hpp>

#include "solver/ihs/problem.hpp"

namespace mopop {
/**
 * @brief Constructs a new solver.
 *
 * @param instance The instance to be solved.
 */
IHS_Solver::IHS_Solver(const Instance &instance) : Solver::Solver(instance) {}

/**
 * @brief Constructs a new empty solver.
 */
IHS_Solver::IHS_Solver() = default;

/**
 * @brief Solves the instance.
 */
void IHS_Solver::solve() {
  this->start_time = std::chrono::steady_clock::now();

  pagmo::problem prob{Problem(this->instance)};
  pagmo::algorithm algo{pagmo::ihs(1, this->phmcr, this->ppar_min,
                                   this->ppar_max, this->bw_min, this->bw_max,
                                   this->seed)};
  pagmo::population pop{
      prob, this->population_size - (2 * this->instance.num_assets + 1),
      this->seed};

  for (unsigned i = 0; i < this->instance.num_assets; i++) {
    std::vector<double> x(this->instance.num_assets, 0.0);
    x[i] = ((double)this->instance.num_assets) /
           ((double)this->instance.num_assets + 1.0);
    pop.push_back(x);
  }

  for (unsigned i = 0; i < this->instance.num_assets; i++) {
    std::vector<double> x(this->instance.num_assets,
                          1.0 / ((double)this->instance.num_assets + 1.0));
    x[i] = 0.0;
    pop.push_back(x);
  }

  std::vector<double> x(this->instance.num_assets,
                        1.0 / ((double)this->instance.num_assets));
  pop.push_back(x);

  this->update_best_individuals(pop);

  if (this->max_num_snapshots > this->num_snapshots + 1) {
    this->capture_snapshot(pop);

    if (this->time_limit < std::numeric_limits<double>::max()) {
      this->time_snapshot_factor =
          std::pow(this->time_limit / this->time_last_snapshot,
                   1.0 / (this->max_num_snapshots - this->num_snapshots));
      this->time_next_snapshot =
          this->time_last_snapshot * this->time_snapshot_factor;
    } else {
      this->time_next_snapshot = std::numeric_limits<double>::max();
      this->time_snapshot_factor = 1.0;
    }

    if (this->iterations_limit < std::numeric_limits<unsigned>::max()) {
      this->iteration_snapshot_factor = std::pow(
          this->iterations_limit / (this->iteration_last_snapshot + 1.0),
          1.0 / (this->max_num_snapshots - this->num_snapshots));
      this->iteration_next_snapshot =
          unsigned(std::round(double(this->iteration_last_snapshot) *
                              this->iteration_snapshot_factor));
    } else {
      this->iteration_next_snapshot = std::numeric_limits<unsigned>::max();
      this->iteration_snapshot_factor = 1.0;
    }
  } else {
    this->time_next_snapshot = 0.0;
    this->iteration_next_snapshot = 0;
    this->time_snapshot_factor = 1.0;
    this->iteration_snapshot_factor = 1.0;
  }

  while (!this->are_termination_criteria_met()) {
    this->num_iterations++;
    pop = algo.evolve(pop);
    this->update_best_individuals(pop);

    if (this->max_num_snapshots > this->num_snapshots + 1) {
      if (this->num_iterations >= this->iteration_next_snapshot) {
        this->capture_snapshot(pop);

        if (this->time_limit < std::numeric_limits<double>::max()) {
          this->time_next_snapshot =
              this->time_last_snapshot * this->time_snapshot_factor;
          this->time_snapshot_factor =
              std::pow(this->time_limit / this->time_last_snapshot,
                       1.0 / (this->max_num_snapshots - this->num_snapshots));
        }

        if (this->iterations_limit < std::numeric_limits<unsigned>::max()) {
          this->iteration_next_snapshot =
              unsigned(std::round(double(this->iteration_last_snapshot) *
                                  this->iteration_snapshot_factor));
          this->iteration_snapshot_factor =
              std::pow(this->iterations_limit / this->iteration_last_snapshot,
                       1.0 / (this->max_num_snapshots - this->num_snapshots));
        }
      } else if (this->elapsed_time() >= this->time_next_snapshot) {
        this->capture_snapshot(pop);

        if (this->time_limit < std::numeric_limits<double>::max()) {
          this->time_next_snapshot =
              this->time_last_snapshot * this->time_snapshot_factor;
          this->time_snapshot_factor =
              std::pow(this->time_limit / this->time_last_snapshot,
                       1.0 / (this->max_num_snapshots - this->num_snapshots));
        }

        if (this->iterations_limit < std::numeric_limits<unsigned>::max()) {
          this->iteration_next_snapshot =
              unsigned(std::round(double(this->iteration_last_snapshot) *
                                  this->iteration_snapshot_factor));
          this->iteration_snapshot_factor =
              std::pow(this->iterations_limit / this->iteration_last_snapshot,
                       1.0 / (this->max_num_snapshots - this->num_snapshots));
        }
      }
    }
  }

  if (this->max_num_snapshots > 0) {
    this->capture_snapshot(pop);
  }

  this->best_solutions.clear();

  for (const auto &best_individual : this->best_individuals) {
    this->best_solutions.push_back(
        Solution(this->instance, best_individual.second));
  }

  this->solving_time = this->elapsed_time();
}

/**
 * @brief Standard stream operator.
 *
 * @param os The standard output stream object.
 * @param solver The solver.
 * @return The stream object.
 */
std::ostream &operator<<(std::ostream &os, const IHS_Solver &solver) {
  os << static_cast<const Solver &>(solver)
     << "Population size: " << solver.population_size << std::endl
     << "Probability of choosing from memory: " << solver.phmcr << std::endl
     << "Minimum pitch adjustment rate: " << solver.ppar_min << std::endl
     << "Maximum pitch adjustment rate: " << solver.ppar_max << std::endl
     << "Minimum distance bandwidth: " << solver.bw_min << std::endl
     << "Maximum distance bandwidth: " << solver.bw_max << std::endl;
  return os;
}

}  // namespace mopop
