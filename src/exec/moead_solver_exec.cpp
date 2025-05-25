#include "solver/moead/moead_solver.hpp"
#include "utils/argument_parser.hpp"

int main(int argc, char* argv[]) {
  Argument_Parser arg_parser(argc, argv);

  if (arg_parser.option_exists("--expected-returns-filename") &&
      arg_parser.option_exists("--covariance-filename")) {
    mopop::Instance instance(
        arg_parser.option_value("--expected-returns-filename"),
        arg_parser.option_value("--covariance-filename"));
    mopop::MOEAD_Solver solver(instance);

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

    if (arg_parser.option_exists("--weight-generation")) {
      solver.weight_generation = arg_parser.option_value("--weight-generation");
    }

    if (arg_parser.option_exists("--decomposition")) {
      solver.decomposition = arg_parser.option_value("--decomposition");
    }

    if (arg_parser.option_exists("--neighbours")) {
      solver.neighbours = std::stoul(arg_parser.option_value("--neighbours"));
    }

    if (arg_parser.option_exists("--cr")) {
      solver.cr = std::stod(arg_parser.option_value("--cr"));
    }

    if (arg_parser.option_exists("--f")) {
      solver.f = std::stod(arg_parser.option_value("--f"));
    }

    if (arg_parser.option_exists("--eta-m")) {
      solver.eta_m = std::stod(arg_parser.option_value("--eta-m"));
    }

    if (arg_parser.option_exists("--realb")) {
      solver.realb = std::stod(arg_parser.option_value("--realb"));
    }

    if (arg_parser.option_exists("--limit")) {
      solver.limit = std::stoul(arg_parser.option_value("--limit"));
    }

    solver.preserve_diversity =
        arg_parser.option_exists("--preserve-diversity");

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

          for (unsigned j = 0; j < population.front().size(); j++) {
            for (unsigned k = 0; k < population.front()[j].size() - 1; k++) {
              ofs << population.front()[j][k] << " ";
            }

            ofs << population.front()[j].back() << std::endl;
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
  } else {
    std::cerr
        << "./moead_solver_exec "
        << "--expected-returns-filename <expected_returns_filename> "
        << "--covariance-filename <covariance_filename> "
        << "--seed <seed> "
        << "--time-limit <time_limit> "
        << "--iterations-limit <iterations_limit> "
        << "--max-num-solutions <max_num_solutions> "
        << "--max-num-snapshots <max_num_snapshots> "
        << "--population-size <population_size> "
        << "--weight-generation <weight_generation> "
        << "--decomposition <decomposition> "
        << "--neighbours <neighbours> "
        << "--cr <cr> "
        << "--f <f> "
        << "--eta-m <eta_m> "
        << "--realb <realb> "
        << "--limit <limit> "
        << "--preserve-diversity "
        << "--statistics <statistics_filename> "
        << "--solutions <solutions_filename> "
        << "--pareto <pareto_filename> "
        << "--best-solutions-snapshots <best_solutions_snapshots_filename> "
        << "--num-non-dominated-snapshots "
           "<num_non_dominated_snapshots_filename> "
        << "--num-fronts-snapshots <num_fronts_snapshots_filename> "
        << "--populations-snapshots <populations_snapshots_filename> "
        << std::endl;
  }

  return 0;
}
