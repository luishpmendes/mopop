#include "solver/nsbrkga/nsbrkga_solver.hpp"
#include "utils/argument_parser.hpp"

int main(int argc, char* argv[]) {
  Argument_Parser arg_parser(argc, argv);

  if (arg_parser.option_exists("--expected-returns-filename") &&
      arg_parser.option_exists("--covariance-filename")) {
    mopop::Instance instance(
        arg_parser.option_value("--expected-returns-filename"),
        arg_parser.option_value("--covariance-filename"));
    mopop::NSBRKGA_Solver solver(instance);

    if (arg_parser.option_exists("--seed")) {
      solver.set_seed(std::stoul(arg_parser.option_value("--seed")));
    }

    if (arg_parser.option_exists("--time-limit")) {
      solver.time_limit = std::stod(arg_parser.option_value("--time-limit"));
    }

    if (arg_parser.option_exists("--iterations-limit")) {
      solver.iterations_limit =
          std::stoul(arg_parser.option_value("--iterations-limit"));
    }

    if (arg_parser.option_exists("--max-num-solutions")) {
      solver.max_num_solutions =
          std::stoul(arg_parser.option_value("--max-num-solutions"));
    }

    if (arg_parser.option_exists("--max-num-snapshots")) {
      solver.max_num_snapshots =
          std::stoul(arg_parser.option_value("--max-num-snapshots"));
    }

    if (arg_parser.option_exists("--population-size")) {
      solver.population_size =
          std::stoul(arg_parser.option_value("--population-size"));
    }

    if (arg_parser.option_exists("--min-elites-percentage")) {
      solver.min_elites_percentage =
          std::stod(arg_parser.option_value("--min-elites-percentage"));
    }

    if (arg_parser.option_exists("--max-elites-percentage")) {
      solver.max_elites_percentage =
          std::stod(arg_parser.option_value("--max-elites-percentage"));
    }

    if (arg_parser.option_exists("--mutation-probability")) {
      solver.mutation_probability =
          std::stod(arg_parser.option_value("--mutation-probability"));
    }

    if (arg_parser.option_exists("--mutation-distribution")) {
      solver.mutation_distribution =
          std::stod(arg_parser.option_value("--mutation-distribution"));
    }

    if (arg_parser.option_exists("--num-total-parents")) {
      solver.num_total_parents =
          std::stoul(arg_parser.option_value("--num-total-parents"));
    }

    if (arg_parser.option_exists("--num-elite-parents")) {
      solver.num_elite_parents =
          std::stoul(arg_parser.option_value("--num-elite-parents"));
    }

    if (arg_parser.option_exists("--bias-type")) {
      std::stringstream ss(arg_parser.option_value("--bias-type"));
      ss >> solver.bias_type;
    }

    if (arg_parser.option_exists("--diversity-type")) {
      std::stringstream ss(arg_parser.option_value("--diversity-type"));
      ss >> solver.diversity_type;
    }

    if (arg_parser.option_exists("--num-populations")) {
      solver.num_populations =
          std::stoul(arg_parser.option_value("--num-populations"));
    }

    if (arg_parser.option_exists("--exchange-interval")) {
      solver.exchange_interval =
          std::stoul(arg_parser.option_value("--exchange-interval"));
    }

    if (arg_parser.option_exists("--num-exchange-individuals")) {
      solver.num_exchange_individuals =
          std::stoul(arg_parser.option_value("--num-exchange-individuals"));
    }

    if (arg_parser.option_exists("--pr-type")) {
      std::stringstream ss(arg_parser.option_value("--pr-type"));
      ss >> solver.pr_type;
    }

    if (arg_parser.option_exists("--pr-dist-func")) {
      std::string s = arg_parser.option_value("--pr-dist-func");
      std::transform(s.begin(), s.end(), s.begin(), ::toupper);

      if (s.compare("HAMMING") == 0) {
        solver.pr_dist_func = std::shared_ptr<NSBRKGA::DistanceFunctionBase>(
            new NSBRKGA::KendallTauDistance());
      } else if (s.compare("KENDALL_TAU") == 0) {
        solver.pr_dist_func = std::shared_ptr<NSBRKGA::DistanceFunctionBase>(
            new NSBRKGA::KendallTauDistance());
      } else if (s.compare("EUCLIDEAN") == 0) {
        solver.pr_dist_func = std::shared_ptr<NSBRKGA::DistanceFunctionBase>(
            new NSBRKGA::EuclideanDistance());
      }
    }

    if (arg_parser.option_exists("--pr-percentage")) {
      solver.pr_percentage =
          std::stod(arg_parser.option_value("--pr-percentage"));
    }

    if (arg_parser.option_exists("--pr-interval")) {
      solver.pr_interval = std::stoul(arg_parser.option_value("--pr-interval"));
    }

    if (arg_parser.option_exists("--shake-interval")) {
      solver.shake_interval =
          std::stoul(arg_parser.option_value("--shake-interval"));
    }

    if (arg_parser.option_exists("--shake-intensity")) {
      solver.shake_intensity =
          std::stod(arg_parser.option_value("--shake-intensity"));
    }

    if (arg_parser.option_exists("--shake-distribution")) {
      solver.shake_distribution =
          std::stod(arg_parser.option_value("--shake-distribution"));
    }

    if (arg_parser.option_exists("--reset-interval")) {
      solver.reset_interval =
          std::stoul(arg_parser.option_value("--reset-interval"));
    }

    if (arg_parser.option_exists("--reset-intensity")) {
      solver.reset_intensity =
          std::stoul(arg_parser.option_value("--reset-intensity"));
    }

    if (arg_parser.option_exists("--num-threads")) {
      solver.num_threads = std::stoul(arg_parser.option_value("--num-threads"));
    }

    solver.solve();

    if (arg_parser.option_exists("--statistics")) {
      std::ofstream ofs;
      ofs.open(arg_parser.option_value("--statistics"));

      if (ofs.is_open()) {
        ofs << solver;

        if (ofs.eof() || ofs.fail() || ofs.bad()) {
          throw std::runtime_error("Error writing file " +
                                   arg_parser.option_value("--statistics") +
                                   ".");
        }

        ofs.close();
      } else {
        throw std::runtime_error("File " +
                                 arg_parser.option_value("--statistics") +
                                 " not created.");
      }
    }

    if (arg_parser.option_exists("--solutions")) {
      std::string solution_filename = arg_parser.option_value("--solutions");

      for (unsigned i = 0; i < solver.best_solutions.size(); i++) {
        std::ofstream ofs;
        ofs.open(solution_filename + std::to_string(i) + ".sol");

        if (ofs.is_open()) {
          ofs << solver.best_solutions[i];

          if (ofs.eof() || ofs.fail() || ofs.bad()) {
            throw std::runtime_error("Error writing file " + solution_filename +
                                     std::to_string(i) + ".sol.");
          }

          ofs.close();
        } else {
          throw std::runtime_error("File " + solution_filename +
                                   std::to_string(i) + ".sol not created.");
        }
      }
    }

    if (arg_parser.option_exists("--pareto")) {
      std::ofstream ofs;
      ofs.open(arg_parser.option_value("--pareto"));

      if (ofs.is_open()) {
        for (const auto& solution : solver.best_solutions) {
          for (unsigned i = 0; i < solution.value.size() - 1; i++) {
            ofs << solution.value[i] << " ";
          }

          ofs << solution.value.back() << std::endl;

          if (ofs.eof() || ofs.fail() || ofs.bad()) {
            throw std::runtime_error("Error writing file " +
                                     arg_parser.option_value("--pareto") + ".");
          }
        }

        ofs.close();
      } else {
        throw std::runtime_error("File " + arg_parser.option_value("--pareto") +
                                 " not created.");
      }
    }

    if (arg_parser.option_exists("--best-solutions-snapshots")) {
      std::string best_solutions_snapshots_filename =
          arg_parser.option_value("--best-solutions-snapshots");

      for (unsigned i = 0; i < solver.best_solutions_snapshots.size(); i++) {
        std::ofstream ofs;
        ofs.open(best_solutions_snapshots_filename + std::to_string(i) +
                 ".txt");

        if (ofs.is_open()) {
          unsigned iteration = std::get<0>(solver.best_solutions_snapshots[i]);
          double time = std::get<1>(solver.best_solutions_snapshots[i]);
          std::vector<std::vector<double>> best_solutions =
              std::get<2>(solver.best_solutions_snapshots[i]);

          ofs << iteration << " " << time << std::endl;

          for (unsigned j = 0; j < best_solutions.size(); j++) {
            for (unsigned k = 0; k < best_solutions[j].size() - 1; k++) {
              ofs << best_solutions[j][k] << " ";
            }

            ofs << best_solutions[j].back() << std::endl;
          }

          if (ofs.eof() || ofs.fail() || ofs.bad()) {
            throw std::runtime_error("Error writing file " +
                                     best_solutions_snapshots_filename +
                                     std::to_string(i) + "txt.");
          }

          ofs.close();
        } else {
          throw std::runtime_error("File " + best_solutions_snapshots_filename +
                                   std::to_string(i) + ".txt not created.");
        }
      }
    }

    if (arg_parser.option_exists("--num-non-dominated-snapshots")) {
      std::ofstream ofs;
      ofs.open(arg_parser.option_value("--num-non-dominated-snapshots"));

      if (ofs.is_open()) {
        for (unsigned i = 0; i < solver.num_non_dominated_snapshots.size();
             i++) {
          unsigned iteration =
              std::get<0>(solver.num_non_dominated_snapshots[i]);
          double time = std::get<1>(solver.num_non_dominated_snapshots[i]);
          std::vector<unsigned> num_non_dominated =
              std::get<2>(solver.num_non_dominated_snapshots[i]);

          ofs << iteration << " " << time << " ";

          for (unsigned j = 0; j < num_non_dominated.size() - 1; j++) {
            ofs << num_non_dominated[j] << " ";
          }

          ofs << num_non_dominated.back() << std::endl;

          if (ofs.eof() || ofs.fail() || ofs.bad()) {
            throw std::runtime_error(
                "Error writing file " +
                arg_parser.option_value("--num-non-dominated-snapshots") + ".");
          }
        }

        ofs.close();
      } else {
        throw std::runtime_error(
            "File " + arg_parser.option_value("--num-non-dominated-snapshots") +
            " not created.");
      }
    }

    if (arg_parser.option_exists("--num-fronts-snapshots")) {
      std::ofstream ofs;
      ofs.open(arg_parser.option_value("--num-fronts-snapshots"));

      if (ofs.is_open()) {
        for (unsigned i = 0; i < solver.num_fronts_snapshots.size(); i++) {
          unsigned iteration = std::get<0>(solver.num_fronts_snapshots[i]);
          double time = std::get<1>(solver.num_fronts_snapshots[i]);
          std::vector<unsigned> num_fronts =
              std::get<2>(solver.num_fronts_snapshots[i]);

          ofs << iteration << " " << time << " ";

          for (unsigned j = 0; j < num_fronts.size() - 1; j++) {
            ofs << num_fronts[j] << " ";
          }

          ofs << num_fronts.back() << std::endl;

          if (ofs.eof() || ofs.fail() || ofs.bad()) {
            throw std::runtime_error(
                "Error writing file " +
                arg_parser.option_value("--num-fronts-snapshots") + ".");
          }
        }

        ofs.close();
      } else {
        throw std::runtime_error(
            "File " + arg_parser.option_value("--num-fronts-snapshots") +
            " not created.");
      }
    }

    if (arg_parser.option_exists("--populations-snapshots")) {
      std::string populations_snapshots_filename =
          arg_parser.option_value("--populations-snapshots");

      for (unsigned i = 0; i < solver.populations_snapshots.size(); i++) {
        std::ofstream ofs;
        ofs.open(populations_snapshots_filename + std::to_string(i) + ".txt");

        if (ofs.is_open()) {
          unsigned iteration = std::get<0>(solver.populations_snapshots[i]);
          double time = std::get<1>(solver.populations_snapshots[i]);
          std::vector<std::vector<std::vector<double>>> population =
              std::get<2>(solver.populations_snapshots[i]);

          ofs << iteration << " " << time << std::endl;

          for (unsigned j = 0; j < population.size(); j++) {
            for (unsigned k = 0; k < population[j].size(); k++) {
              for (unsigned l = 0; l < population[j][k].size() - 1; l++) {
                ofs << population[j][k][l] << " ";
              }

              ofs << population[j][k].back() << std::endl;
            }
          }

          if (ofs.eof() || ofs.fail() || ofs.bad()) {
            throw std::runtime_error("Error writing file " +
                                     populations_snapshots_filename +
                                     std::to_string(i) + ".txt.");
          }

          ofs.close();
        } else {
          throw std::runtime_error("File " + populations_snapshots_filename +
                                   std::to_string(i) + ".txt not created.");
        }
      }
    }

    if (arg_parser.option_exists("--num-elites-snapshots")) {
      std::ofstream ofs;
      ofs.open(arg_parser.option_value("--num-elites-snapshots"));

      if (ofs.is_open()) {
        for (unsigned i = 0; i < solver.num_elites_snapshots.size(); i++) {
          unsigned iteration = std::get<0>(solver.num_elites_snapshots[i]);
          double time = std::get<1>(solver.num_elites_snapshots[i]);
          std::vector<unsigned> num_elites =
              std::get<2>(solver.num_elites_snapshots[i]);

          ofs << iteration << " " << time << " ";

          for (unsigned j = 0; j < num_elites.size() - 1; j++) {
            ofs << num_elites[j] << " ";
          }

          ofs << num_elites.back() << std::endl;

          if (ofs.eof() || ofs.fail() || ofs.bad()) {
            throw std::runtime_error(
                "Error writing file " +
                arg_parser.option_value("--num-elites-snapshots") + ".");
          }
        }

        ofs.close();
      } else {
        throw std::runtime_error(
            "File " + arg_parser.option_value("--num-elites-snapshots") +
            " not created.");
      }
    }
  } else {
    std::cerr
        << "./nsbrkga_solver_exec "
        << "--expected-returns-filename <expected_returns_filename> "
        << "--covariance-filename <covariance_filename> "
        << "--seed <seed> "
        << "--time-limit <time_limit> "
        << "--iterations-limit <iterations_limit> "
        << "--max-num-solutions <max_num_solutions> "
        << "--max-num-snapshots <max_num_snapshots> "
        << "--population-size <population_size> "
        << "--min-elites-percentage <min_elites_percentage> "
        << "--max-elites-percentage <max_elites_percentage> "
        << "--mutation-probability <mutation_probability> "
        << "--mutation-distribution <mutation_distribution> "
        << "--num-total-parents <num_total_parents> "
        << "--num-elite-parents <num_elite_parents> "
        << "--bias-type <bias_type> "
        << "--diversity-type <diversity_type> "
        << "--num-populations <num_populations> "
        << "--exchange-interval <exchange_interval> "
        << "--num-exchange-individuals <num_exchange_individuals> "
        << "--pr-type <pr_type> "
        << "--pr-dist-func <pr_dist_func> "
        << "--pr-percentage <pr_percentage> "
        << "--pr-interval <pr_interval> "
        << "--shake-interval <shake_interval> "
        << "--shake-intensity <shake_intensity> "
        << "--shake-distribution <shake_distribution> "
        << "--reset-interval <reset_interval> "
        << "--reset-intensity <reset_intensity> "
        << "--num-threads <num_threads> "
        << "--statistics <statistics_filename> "
        << "--solutions <solutions_filename> "
        << "--pareto <pareto_filename> "
        << "--best-solutions-snapshots <best_solutions_snapshots_filename> "
        << "--num-non-dominated-snapshots "
           "<num_non_dominated_snapshots_filename> "
        << "--num-fronts-snapshots <num_fronts_snapshots_filename> "
        << "--populations-snapshots <populations_snapshots_filename> "
        << "--num-elites-snapshots <num_elites_snapshots_filename> "
        << std::endl;
  }

  return 0;
}
