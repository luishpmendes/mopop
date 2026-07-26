#include <cassert>
#include <cmath>
#include <fstream>
#include <limits>

#include "instance/instance.hpp"
#include "utils/argument_parser.hpp"

/**
 * @brief Computes the modified distance from a reference point to a point.
 *
 * An objective only contributes to the distance when the point is worse than
 * the reference point on that objective, which is what distinguishes IGD+ from
 * the plain inverted generational distance.
 *
 * @param senses A vector indicating whether each objective is minimized or
 * maximized.
 * @param reference_point The reference point.
 * @param point The point.
 * @return The modified distance between the two points.
 */
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

/**
 * @brief Computes the modified inverted generational distance of a front.
 *
 * @param senses A vector indicating whether each objective is minimized or
 * maximized.
 * @param reference_front The reference front.
 * @param front The front.
 * @return The mean modified distance from each reference front point to its
 * closest point in the front, or infinity if the front is empty.
 */
static inline double modified_inverted_generational_distance(
    const std::vector<NSBRKGA::Sense>& senses,
    const std::vector<std::vector<double>>& reference_front,
    const std::vector<std::vector<double>>& front) {
  double igd_plus = 0.0, min_distance, distance;

  if (front.empty()) {
    return std::numeric_limits<double>::infinity();
  }

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

/**
 * @brief Computes the normalized modified inverted generational distance of a
 * front.
 *
 * @param reference_igd_plus The modified inverted generational distance of the
 * front made of the reference point alone, which is the largest value any front
 * can attain.
 * @param senses A vector indicating whether each objective is minimized or
 * maximized.
 * @param reference_front The reference front.
 * @param front The front.
 * @return The modified inverted generational distance of the front divided by
 * the reference one, or one if the front is empty.
 */
static inline double normalized_modified_inverted_generational_distance(
    const double& reference_igd_plus, const std::vector<NSBRKGA::Sense>& senses,
    const std::vector<std::vector<double>>& reference_front,
    const std::vector<std::vector<double>>& front) {
  if (front.empty()) {
    return 1.0;
  }

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
    unsigned num_objectives = instance.senses.size();
    std::vector<double> reference_point(num_objectives, 0.0);
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

        for (unsigned j = 0; j < num_objectives; j++) {
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
        std::vector<double> value(num_objectives, 0.0);

        for (unsigned j = 0; j < num_objectives; j++) {
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
         arg_parser.option_exists("--nigd-plus-" +
                                  std::to_string(num_solvers)) ||
         arg_parser.option_exists("--nigd-plus-snapshots-" +
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
            std::vector<double> value(num_objectives, 0.0);

            for (unsigned j = 0; j < num_objectives; j++) {
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
              std::vector<double> value(num_objectives, 0.0);

              for (unsigned j = 0; j < num_objectives; j++) {
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
      if (arg_parser.option_exists("--nigd-plus-" + std::to_string(i))) {
        std::ofstream ofs;
        ofs.open(arg_parser.option_value("--nigd-plus-" + std::to_string(i)));

        if (ofs.is_open()) {
          double normalized_igd_plus =
              normalized_modified_inverted_generational_distance(
                  reference_igd_plus, instance.senses, reference_pareto,
                  paretos[i]);

          assert(normalized_igd_plus >= 0.0);
          assert(normalized_igd_plus <= 1.0 + 1e-9);

          ofs << normalized_igd_plus << std::endl;

          if (ofs.eof() || ofs.fail() || ofs.bad()) {
            throw std::runtime_error(
                "Error writing file " +
                arg_parser.option_value("--nigd-plus-" + std::to_string(i)) +
                ".");
          }

          ofs.close();
        } else {
          throw std::runtime_error(
              "File " +
              arg_parser.option_value("--nigd-plus-" + std::to_string(i)) +
              " not created.");
        }
      }
    }

    for (unsigned i = 0; i < num_solvers; i++) {
      if (arg_parser.option_exists("--nigd-plus-snapshots-" +
                                   std::to_string(i))) {
        std::ofstream ofs;

        ofs.open(arg_parser.option_value("--nigd-plus-snapshots-" +
                                         std::to_string(i)));

        if (ofs.is_open()) {
          for (unsigned j = 0; j < best_solutions_snapshots[i].size(); j++) {
            double normalized_igd_plus =
                normalized_modified_inverted_generational_distance(
                    reference_igd_plus, instance.senses, reference_pareto,
                    best_solutions_snapshots[i][j]);

            assert(normalized_igd_plus >= 0.0);
            assert(normalized_igd_plus <= 1.0 + 1e-9);

            ofs << iteration_snapshots[i][j] << "," << time_snapshots[i][j]
                << "," << normalized_igd_plus << std::endl;

            if (ofs.eof() || ofs.fail() || ofs.bad()) {
              throw std::runtime_error(
                  "Error writing file " +
                  arg_parser.option_value("--nigd-plus-snapshots-" +
                                          std::to_string(i)) +
                  ".");
            }
          }

          ofs.close();
        } else {
          throw std::runtime_error(
              "File " +
              arg_parser.option_value("--nigd-plus-snapshots-" +
                                      std::to_string(i)) +
              " not created.");
        }
      }
    }
  } else {
    std::cerr
        << "./normalized_modified_generational_distance_calculator_exec "
        << "--expected-returns-filename <expected_returns_filename> "
        << "--covariance-filename <covariance_filename> "
        << "--reference-pareto <reference_pareto_filename> "
        << "--reference-point <reference_point_filename> "
        << "--pareto-i <pareto_filename> "
        << "--best-solutions-snapshots-i <best_solutions_snapshots_filename> "
        << "--nigd-plus-i <nigd_plus_filename> "
        << "--nigd-plus-snapshots-i <nigd_plus_snapshots_filename> "
        << std::endl;
  }

  return 0;
}
