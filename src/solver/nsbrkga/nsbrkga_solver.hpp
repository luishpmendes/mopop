#pragma once

#include "solver/nsbrkga/decoder.hpp"
#include "solver/solver.hpp"

namespace mopop {
/**
 * @class NSBRKGA_Solver
 * @brief The NSBRKGA_Solver represents a solver for the Multi-Objective
 * Portfolio Optimization Problem using the Non-Dominated Sorting Biased
 * Random-Key Genetic Algorithm.
 */
class NSBRKGA_Solver : public Solver {
 public:
  /**
   * @brief The size of the population.
   */
  unsigned population_size = 300;

  /**
   * @brief The minimum percentage of individuals to become the elite set
   */
  double min_elites_percentage = 0.10;

  /**
   * @brief The maximum percentage of individuals to become the elite set
   */
  double max_elites_percentage = 0.30;

  /**
   * @brief The mutation probability.
   */
  double mutation_probability = 0.01;

  /**
   * @brief The distribution index for mutation.
   */
  double mutation_distribution = 50.0;

  /**
   * @brief The number of total parents for mating.
   */
  unsigned num_total_parents = 3;

  /**
   * @brief The number of elite parents for mating.
   */
  unsigned num_elite_parents = 2;

  /**
   * @brief The type of bias that will be used.
   */
  NSBRKGA::BiasFunctionType bias_type = NSBRKGA::BiasFunctionType::SQRT;

  /**
   * @brief The type of diversity that will be used.
   */
  NSBRKGA::DiversityFunctionType diversity_type =
      NSBRKGA::DiversityFunctionType::AVERAGE_DISTANCE_TO_CENTROID;

  /**
   * @brief The number of independent parallel populations.
   */
  unsigned num_populations = 3;

  /**
   * @brief The interval at which the elite solutions are exchanged between
   * populations (0 means no exchange).
   */
  unsigned exchange_interval = 200;

  /**
   * @brief The number of elite individuals to be exchanged between populations.
   */
  unsigned num_exchange_individuals = 30;

  /**
   * @brief The type of path relinking that will be used.
   */
  NSBRKGA::PathRelinking::Type pr_type =
      NSBRKGA::PathRelinking::Type::BINARY_SEARCH;

  /**
   * @brief The distance function that will be used in the path relinking.
   */
  std::shared_ptr<NSBRKGA::DistanceFunctionBase> pr_dist_func =
      std::shared_ptr<NSBRKGA::DistanceFunctionBase>(
          new NSBRKGA::EuclideanDistance());

  /**
   * @brief The percentage of the path to be computed.
   */
  double pr_percentage = 0.20;

  /**
   * @brief The interval at which the path relink is applied.
   * (0 means no path relinking).
   */
  unsigned pr_interval = 500;

  /**
   * @brief The interval at which the populations are shaken.
   * (0 means no shaking).
   */
  unsigned shake_interval = 200;

  /**
   * @brief The intensity of the shaking.
   */
  double shake_intensity = 0.33;

  /**
   * @brief The shaking distribution.
   */
  double shake_distribution = 20.0;

  /**
   * @brief The interval at which the populations are reset (0 means no reset).
   */
  unsigned reset_interval = 500;

  /**
   * @brief The intensity of the reset.
   */
  double reset_intensity = 0.20;

  /**
   * @brief The number of threads to be used during parallel decoding.
   */
  unsigned num_threads = 1;

  /**
   * @brief The maximum number of local search iterations allowed.
   */
  unsigned max_local_search_iterations = 0;

  /**
   * @brief The last update generation.
   */
  unsigned last_update_generation = 0;

  /**
   * @brief The last update time.
   */
  double last_update_time = 0.0;

  /**
   * @brief The largest number of generations between improvements.
   */
  unsigned large_offset = 0;

  /**
   * @brief The total path relink time.
   */
  double path_relink_time = 0.0;

  /**
   * @brief The total path relink calls.
   */
  unsigned num_path_relink_calls = 0;

  /**
   * @brief The number of improvements in the elite set.
   */
  unsigned num_elite_improvments = 0;

  /**
   * @brief The number of best individual improvements.
   */
  unsigned num_best_improvements = 0;

  /**
   * @brief The total shaking calls.
   */
  unsigned num_shakings = 0;

  /**
   * @brief The total reset calls.
   */
  unsigned num_resets = 0;

  /**
   * @brief The snapshots of the number of elite individuals,
   * containing the iteration, time and number of elite individuals.
   */
  std::vector<std::tuple<unsigned, double, std::vector<unsigned>>>
      num_elites_snapshots = {};

  /**
   * @brief The number of non-dominated individuals in each current population.
   */
  std::vector<unsigned> num_non_dominated = {};

  /**
   * @brief The number of non-dominated fronts in each current population.
   */
  std::vector<unsigned> num_fronts = {};

  /**
   * @brief The number of elite individuals in each current population.
   */
  std::vector<unsigned> num_elites = {};

  /**
   * @brief The fronts of each current population.
   */
  std::vector<std::vector<std::pair<std::vector<double>, unsigned>>> fronts =
      {};

  /**
   * @brief Constructs a new solver.
   *
   * @param instance The instance to be solved.
   */
  NSBRKGA_Solver(const Instance& instance);

  /**
   * @brief Constructs a new empty solver.
   */
  NSBRKGA_Solver();

  /**
   * @brief Captures a snapshot of the current population.
   *
   * @param algorithm The current state of the algorithm.
   */
  void capture_snapshot(const NSBRKGA::NSBRKGA<Decoder>& algorithm);

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
  friend std::ostream& operator<<(std::ostream& os,
                                  const NSBRKGA_Solver& solver);
};

}  // namespace mopop
