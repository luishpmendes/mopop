#include <cassert>
#include <fstream>

#include "instance/instance.hpp"
#include "utils/argument_parser.hpp"

static inline double modified_distance(
    const std::vector<NSBRKGA::Sense>& senses,
    const std::vector<double>& reference_point,
    const std::vector<double>& point) {
  double distance = 0.0, delta;

  for (unsigned i = 0; i < senses.size(); i++) {
    delta = 0;

    if (senses[i] == NSBRKGA::Sense::MINIMIZE) {
      if (point[i] > reference_point[i]) {
        delta = point[i] - reference_point[i];
      }
    } else {  // senses[i] == NSBRKGA::Sense::MAXIMIZE
      if (reference_point[i] > point[i]) {
        delta = reference_point[i] - point[i];
      }
    }

    distance += delta * delta;
  }

  return sqrt(distance);
}

static inline double modified_inverted_generational_distance(
    const std::vector<NSBRKGA::Sense>& senses,
    const std::vector<std::vector<double>>& reference_front,
    const std::vector<std::vector<double>>& front) {
  double igd_plus = 0.0, min_distance, distance;

  for (unsigned i = 0; i < reference_front.size(); i++) {
    min_distance = modified_distance(senses, reference_front[i], front.front());

    for (unsigned j = 1; j < front.size(); j++) {
      distance = modified_distance(senses, reference_front[i], front[j]);

      if (distance < min_distance) {
        min_distance = distance;
      }
    }

    igd_plus += min_distance;
  }

  return igd_plus / reference_front.size();
}

static inline double normalized_modified_inverted_generational_distance(
    const double& reference_igd_plus, const std::vector<NSBRKGA::Sense>& senses,
    const std::vector<std::vector<double>>& reference_front,
    const std::vector<std::vector<double>>& front) {
  double igd_plus =
      modified_inverted_generational_distance(senses, reference_front, front);
  return igd_plus / reference_igd_plus;
}

int main(int argc, char* argv[]) {
  Argument_Parser arg_parser(argc, argv);

  if (arg_parser.option_exists("--expected-returns-filename") &&
      arg_parser.option_exists("--covariance-filename") &&
      arg_parser.option_exists("--reference-pareto") &&
      arg_parser.option_exists("--reference-point")) {
    mopop::Instance instance(
        arg_parser.option_value("--expected-returns-filename"),
        arg_parser.option_value("--covariance-filename"));
    std::ifstream ifs;
    std::vector<double> reference_point(4, 0.0);
    std::vector<std::vector<double>> reference_pareto;
    double reference_igd_plus;
    std::vector<std::vector<std::vector<double>>> paretos;
    std::vector<std::vector<unsigned>> iteration_snapshots;
    std::vector<std::vector<double>> time_snapshots;
    std::vector<std::vector<std::vector<std::vector<double>>>>
        best_solutions_snapshots;
    unsigned num_solvers;

    ifs.open(arg_parser.option_value("--reference-point"));

    if (ifs.is_open()) {
      for (std::string line; std::getline(ifs, line);) {
        std::istringstream iss(line);

        for (unsigned j = 0; j < 4; j++) {
          iss >> reference_point[j];
        }
      }

      ifs.close();
    } else {
      throw std::runtime_error("File " +
                               arg_parser.option_value("--reference-point") +
                               " not found.");
    }

    ifs.open(arg_parser.option_value("--reference-pareto"));

    if (ifs.is_open()) {
      for (std::string line; std::getline(ifs, line);) {
        std::istringstream iss(line);
        std::vector<double> value(4, 0.0);

        for (unsigned j = 0; j < 4; j++) {
          iss >> value[j];
        }

        reference_pareto.push_back(value);
      }

      ifs.close();
    } else {
      throw std::runtime_error("File " +
                               arg_parser.option_value("--reference-pareto") +
                               " not found.");
    }

    reference_igd_plus = modified_inverted_generational_distance(
        instance.senses, reference_pareto, {reference_point});

    assert(reference_igd_plus > 0.0);

    for (num_solvers = 0;
         arg_parser.option_exists("--pareto-" + std::to_string(num_solvers)) ||
         arg_parser.option_exists("--best-solutions-snapshots-" +
                                  std::to_string(num_solvers)) ||
         arg_parser.option_exists("--igd-plus-" +
                                  std::to_string(num_solvers)) ||
         arg_parser.option_exists("--igd-plus-snapshots-" +
                                  std::to_string(num_solvers));
         num_solvers++) {
    }

    paretos.resize(num_solvers);
    iteration_snapshots.resize(num_solvers);
    time_snapshots.resize(num_solvers);
    best_solutions_snapshots.resize(num_solvers);

    for (unsigned i = 0; i < num_solvers; i++) {
      if (arg_parser.option_exists("--pareto-" + std::to_string(i))) {
        ifs.open(arg_parser.option_value("--pareto-" + std::to_string(i)));

        if (ifs.is_open()) {
          for (std::string line; std::getline(ifs, line);) {
            std::istringstream iss(line);
            std::vector<double> value(4, 0.0);

            for (unsigned j = 0; j < 4; j++) {
              iss >> value[j];
            }

            paretos[i].push_back(value);
          }

          ifs.close();
        } else {
          throw std::runtime_error(
              "File " +
              arg_parser.option_value("--pareto-" + std::to_string(i)) +
              " not found.");
        }
      }
    }

    for (unsigned i = 0; i < num_solvers; i++) {
      if (arg_parser.option_exists("--best-solutions-snapshots-" +
                                   std::to_string(i))) {
        std::string best_solutions_snapshots_filename = arg_parser.option_value(
            "--best-solutions-snapshots-" + std::to_string(i));

        for (unsigned j = 0;; j++) {
          ifs.open(best_solutions_snapshots_filename + std::to_string(j) +
                   ".txt");

          if (ifs.is_open()) {
            unsigned iteration;
            double time;

            ifs >> iteration >> time;

            iteration_snapshots[i].push_back(iteration);
            time_snapshots[i].push_back(time);
            best_solutions_snapshots[i].emplace_back();

            ifs.ignore();

            for (std::string line; std::getline(ifs, line);) {
              std::istringstream iss(line);
              std::vector<double> value(4, 0.0);

              for (unsigned j = 0; j < 4; j++) {
                iss >> value[j];
              }

              best_solutions_snapshots[i].back().push_back(value);
            }

            ifs.close();
          } else {
            break;
          }
        }
      }
    }

    for (unsigned i = 0; i < num_solvers; i++) {
      std::ofstream ofs;
      ofs.open(arg_parser.option_value("--igd-plus-" + std::to_string(i)));

      if (ofs.is_open()) {
        double normalized_igd_plus =
            normalized_modified_inverted_generational_distance(
                reference_igd_plus, instance.senses, reference_pareto,
                paretos[i]);

        assert(normalized_igd_plus >= 0.0);
        assert(normalized_igd_plus <= 1.0);

        ofs << normalized_igd_plus << std::endl;

        if (ofs.eof() || ofs.fail() || ofs.bad()) {
          throw std::runtime_error(
              "Error writing file " +
              arg_parser.option_value("--igd-plus-" + std::to_string(i)) + ".");
        }

        ofs.close();
      } else {
        throw std::runtime_error(
            "File " +
            arg_parser.option_value("--igd-plus-" + std::to_string(i)) +
            " not created.");
      }
    }

    for (unsigned i = 0; i < num_solvers; i++) {
      std::ofstream ofs;

      ofs.open(
          arg_parser.option_value("--igd-plus-snapshots-" + std::to_string(i)));

      if (ofs.is_open()) {
        for (unsigned j = 0; j < best_solutions_snapshots[i].size(); j++) {
          double normalized_igd_plus =
              normalized_modified_inverted_generational_distance(
                  reference_igd_plus, instance.senses, reference_pareto,
                  best_solutions_snapshots[i][j]);

          assert(normalized_igd_plus >= 0.0);
          assert(normalized_igd_plus <= 1.0);

          ofs << iteration_snapshots[i][j] << "," << time_snapshots[i][j] << ","
              << normalized_igd_plus << std::endl;

          if (ofs.eof() || ofs.fail() || ofs.bad()) {
            throw std::runtime_error(
                "Error writing file " +
                arg_parser.option_value("--igd-plus-snapshots-" +
                                        std::to_string(i)) +
                ".");
          }
        }

        ofs.close();
      } else {
        throw std::runtime_error(
            "File " +
            arg_parser.option_value("--igd-plus-snapshots-" +
                                    std::to_string(i)) +
            " not created.");
      }
    }
  } else {
    std::cerr
        << "./modified_inverted_generational_distance_exec "
        << "--instance <instance_filename> "
        << "--reference-pareto <reference_pareto_filename> "
        << "--reference-point <reference_point_filename> "
        << "--pareto-i <pareto_filename> "
        << "--best-solutions-snapshots-i <best_solutions_snapshots_filename> "
        << "--igd-plus-i <modified_inverted_generational_distance_filename> "
        << "--igd-plus-snapshots-i "
           "<modified_inverted_generational_distance_snapshots_filename> "
        << std::endl;
  }

  return 0;
}
