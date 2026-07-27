#include "solver/mhaco/mhaco_solver.hpp"

#include <cassert>
#include <fstream>
#include <iostream>
#include <random>

#include "test/solver_invariants.hpp"

int main() {
  const std::string expected_returns_filename =
                        "input/expected_returns_test.csv",
                    covariance_filename = "input/covariance_matrix_test.csv";
  mopop::Instance instance(expected_returns_filename, covariance_filename);
  mopop::MHACO_Solver solver(instance);

  solver.set_seed(2351389233);
  solver.time_limit = 5.0;
  solver.iterations_limit = 100;
  solver.max_num_solutions = 128;
  solver.population_size = 32;
  solver.max_num_snapshots = 16;
  solver.ker = 32;

  assert((solver.seed = 2351389233));
  assert(fabs(solver.time_limit - 5.0) <
         std::numeric_limits<double>::epsilon());
  assert(solver.iterations_limit == 100);
  assert(solver.max_num_solutions == 128);
  assert(solver.population_size == 32);
  assert(solver.max_num_snapshots == 16);
  assert(solver.ker == 32);
  assert(fabs(solver.q - 1.0) < std::numeric_limits<double>::epsilon());
  assert(solver.threshold == 1);
  assert(solver.n_gen_mark == 7);
  assert(solver.eval_stop == 0);
  assert(fabs(solver.focus - 0.0) < std::numeric_limits<double>::epsilon());
  assert(solver.memory);

  solver.solve();

  mopop::assert_solver_invariants(solver);

  assert(solver.solving_time > 0);

  assert(solver.num_iterations > 0);
  assert(solver.num_iterations <= solver.iterations_limit);

  assert(solver.best_solutions.size() > 0);
  assert(solver.best_solutions.size() <= solver.max_num_solutions);

  assert(solver.num_snapshots == solver.max_num_snapshots);

  assert(solver.best_solutions_snapshots.size() == solver.num_snapshots);
  assert(solver.num_non_dominated_snapshots.size() == solver.num_snapshots);
  assert(solver.num_fronts_snapshots.size() == solver.num_snapshots);
  assert(solver.populations_snapshots.size() == solver.num_snapshots);

  for (const auto& s1 : solver.best_solutions) {
    assert(s1.is_feasible());

    for (const auto& s2 : solver.best_solutions) {
      assert(!s1.dominates(s2));
      assert(!s2.dominates(s1));
    }
  }

  for (const auto& snapshot : solver.best_solutions_snapshots) {
    assert(std::get<0>(snapshot) >= 0);
    assert(std::get<0>(snapshot) <= solver.num_iterations);
    assert(std::get<1>(snapshot) >= 0.0);
    assert(std::get<1>(snapshot) <= solver.solving_time);
    assert(std::get<2>(snapshot).size() > 0);
    assert(std::get<2>(snapshot).size() <= solver.max_num_solutions);

    for (const auto& s : std::get<2>(snapshot)) {
      assert(s.size() == 4);
    }
  }

  for (const auto& snapshot : solver.num_non_dominated_snapshots) {
    assert(std::get<0>(snapshot) >= 0);
    assert(std::get<0>(snapshot) <= solver.num_iterations);
    assert(std::get<1>(snapshot) >= 0.0);
    assert(std::get<1>(snapshot) <= solver.solving_time);
    assert(std::get<2>(snapshot).size() > 0);
    assert(std::get<2>(snapshot).size() <= solver.max_num_solutions);

    for (const unsigned& num_non_dominated : std::get<2>(snapshot)) {
      assert(num_non_dominated > 0);
      assert(num_non_dominated <= solver.population_size);
    }
  }

  for (const auto& snapshot : solver.num_fronts_snapshots) {
    assert(std::get<0>(snapshot) >= 0);
    assert(std::get<0>(snapshot) <= solver.num_iterations);
    assert(std::get<1>(snapshot) >= 0.0);
    assert(std::get<1>(snapshot) <= solver.solving_time);
    assert(std::get<2>(snapshot).size() > 0);
    assert(std::get<2>(snapshot).size() <= solver.max_num_solutions);

    for (const unsigned& num_fronts : std::get<2>(snapshot)) {
      assert(num_fronts > 0);
      assert(num_fronts < solver.population_size);
    }
  }

  for (const auto& snapshot : solver.populations_snapshots) {
    assert(std::get<0>(snapshot) >= 0);
    assert(std::get<0>(snapshot) <= solver.num_iterations);
    assert(std::get<1>(snapshot) >= 0.0);
    assert(std::get<1>(snapshot) <= solver.solving_time);
    assert(std::get<2>(snapshot).size() > 0);
    assert(std::get<2>(snapshot).size() <= solver.max_num_solutions);

    for (const auto& population : std::get<2>(snapshot)) {
      assert(population.size() == solver.population_size);

      for (const auto& s : population) {
        assert(s.size() == 4);
      }
    }
  }

  std::cout << solver << std::endl;

  std::cout << "Num non dominated snapshots: ";
  for (unsigned i = 0; i < solver.num_non_dominated_snapshots.size() - 1; i++) {
    std::cout << "(" << std::get<0>(solver.num_non_dominated_snapshots[i])
              << ", " << std::get<1>(solver.num_non_dominated_snapshots[i])
              << ", "
              << std::accumulate(
                     std::get<2>(solver.num_non_dominated_snapshots[i]).begin(),
                     std::get<2>(solver.num_non_dominated_snapshots[i]).end(),
                     0) /
                     std::get<2>(solver.num_non_dominated_snapshots[i]).size()
              << "), ";
  }
  std::cout
      << "(" << std::get<0>(solver.num_non_dominated_snapshots.back()) << ", "
      << std::get<1>(solver.num_non_dominated_snapshots.back()) << ", "
      << std::accumulate(
             std::get<2>(solver.num_non_dominated_snapshots.back()).begin(),
             std::get<2>(solver.num_non_dominated_snapshots.back()).end(), 0) /
             std::get<2>(solver.num_non_dominated_snapshots.back()).size()
      << ")" << std::endl;

  std::cout << "Num fronts snapshots: ";
  for (unsigned i = 0; i < solver.num_fronts_snapshots.size() - 1; i++) {
    std::cout << "(" << std::get<0>(solver.num_fronts_snapshots[i]) << ", "
              << std::get<1>(solver.num_fronts_snapshots[i]) << ", "
              << std::accumulate(
                     std::get<2>(solver.num_fronts_snapshots[i]).begin(),
                     std::get<2>(solver.num_fronts_snapshots[i]).end(), 0) /
                     std::get<2>(solver.num_fronts_snapshots[i]).size()
              << "), ";
  }
  std::cout << "(" << std::get<0>(solver.num_fronts_snapshots.back()) << ", "
            << std::get<1>(solver.num_fronts_snapshots.back()) << ", "
            << std::accumulate(
                   std::get<2>(solver.num_fronts_snapshots.back()).begin(),
                   std::get<2>(solver.num_fronts_snapshots.back()).end(), 0) /
                   std::get<2>(solver.num_fronts_snapshots.back()).size()
            << ")" << std::endl;

  // BUG 5 regression: an instance whose first asset has a negative expected
  // return, which is what made the old population initialization emit
  // chromosome entries outside [0, 1].
  {
    mopop::Instance bug5_instance("input/expected_returns_bug5_test.csv",
                                  "input/covariance_matrix_bug5_test.csv");
    mopop::MHACO_Solver bug5_solver(bug5_instance);

    bug5_solver.set_seed(305089489);
    bug5_solver.time_limit = 2.0;
    bug5_solver.max_num_solutions = 128;
    bug5_solver.population_size = 32;
    bug5_solver.ker = 32;
    bug5_solver.max_num_snapshots = 16;

    bug5_solver.solve();

    assert(bug5_solver.best_solutions.size() > 0);
    mopop::assert_solver_invariants(bug5_solver);
  }

  // The instance that originally exposed BUG 5, when it has been built. Its
  // seed chromosomes outnumber the population, which exercises the cap in
  // Solver::build_initial_chromosomes.
  if (std::ifstream("instances/ibov_2020/train/expected_returns.csv").good()) {
    mopop::Instance ibov_instance(
        "instances/ibov_2020/train/expected_returns.csv",
        "instances/ibov_2020/train/covariance_matrix.csv");
    mopop::MHACO_Solver ibov_solver(ibov_instance);

    ibov_solver.set_seed(305089489);
    ibov_solver.time_limit = 2.0;
    ibov_solver.max_num_solutions = 128;
    ibov_solver.population_size = 32;
    ibov_solver.ker = 32;
    ibov_solver.max_num_snapshots = 16;

    ibov_solver.solve();

    assert(ibov_solver.best_solutions.size() > 0);
    mopop::assert_solver_invariants(ibov_solver);
  }

  std::cout << std::endl << "MHACO Solver Test PASSED" << std::endl;

  return 0;
}
