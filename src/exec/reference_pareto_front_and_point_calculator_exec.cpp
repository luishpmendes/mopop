#include <cmath>
#include <fstream>
#include <limits>
#include <sstream>

#include "instance/instance.hpp"
#include "solver/solver.hpp"
#include "utils/argument_parser.hpp"

/**
 * @brief Updates the per objective worst and best attained bounds with a point.
 *
 * The worst bound of a maximization objective is its minimum attained value and
 * the worst bound of a minimization objective is its maximum attained value.
 *
 * @param senses A vector indicating whether each objective is minimized or
 * maximized.
 * @param value The objective values of the point.
 * @param worst_values The worst attained value of each objective, updated in
 * place.
 * @param best_values The best attained value of each objective, updated in
 * place.
 */
static inline void update_bounds(const std::vector<NSBRKGA::Sense>& senses,
                                 const std::vector<double>& value,
                                 std::vector<double>& worst_values,
                                 std::vector<double>& best_values) {
  for (unsigned i = 0; i < senses.size(); i++) {
    if (senses[i] == NSBRKGA::Sense::MINIMIZE) {
      if (worst_values[i] < value[i]) {
        worst_values[i] = value[i];
      }

      if (best_values[i] > value[i]) {
        best_values[i] = value[i];
      }
    } else {  // senses[i] == NSBRKGA::Sense::MAXIMIZE
      if (worst_values[i] > value[i]) {
        worst_values[i] = value[i];
      }

      if (best_values[i] < value[i]) {
        best_values[i] = value[i];
      }
    }
  }
}

/**
 * @brief Builds the reference point from the attained bounds.
 *
 * Each coordinate is the worst attained value of that objective pushed outward
 * by 5% of the objective's attained range. The padding keeps every attained
 * point strictly better than the reference point, so that the extreme points of
 * a front still contribute a positive hypervolume and both the hypervolume
 * ratio and the normalized IGD+ stay within [0, 1]. It is additive on the
 * range, so it remains well defined when an objective takes negative values.
 *
 * @param senses A vector indicating whether each objective is minimized or
 * maximized.
 * @param worst_values The worst attained value of each objective.
 * @param best_values The best attained value of each objective.
 * @return The reference point.
 */
static inline std::vector<double> compute_reference_point(
    const std::vector<NSBRKGA::Sense>& senses,
    const std::vector<double>& worst_values,
    const std::vector<double>& best_values) {
  std::vector<double> reference_point(senses.size(), 0.0);

  for (unsigned i = 0; i < senses.size(); i++) {
    double padding = 0.05 * std::fabs(best_values[i] - worst_values[i]);

    // The objective took a single value across every front.
    if (padding < std::numeric_limits<double>::epsilon()) {
      padding = 0.05 * std::fabs(worst_values[i]);
    }

    // That single value was zero.
    if (padding < std::numeric_limits<double>::epsilon()) {
      padding = 0.05;
    }

    if (senses[i] == NSBRKGA::Sense::MINIMIZE) {
      reference_point[i] = worst_values[i] + padding;
    } else {  // senses[i] == NSBRKGA::Sense::MAXIMIZE
      reference_point[i] = worst_values[i] - padding;
    }
  }

  return reference_point;
}

int main(int argc, char* argv[]) {
  Argument_Parser arg_parser(argc, argv);

  if (arg_parser.option_exists("--expected-returns-filename") &&
      arg_parser.option_exists("--covariance-filename")) {
    mopop::Instance instance(
        arg_parser.option_value("--expected-returns-filename"),
        arg_parser.option_value("--covariance-filename"));
    std::ifstream ifs;
    std::vector<std::pair<std::vector<double>, std::vector<double>>>
        reference_pareto, pareto, best_solutions_snapshot;
    unsigned num_objectives = instance.senses.size();
    std::vector<double> worst_values(num_objectives, 0.0);
    std::vector<double> best_values(num_objectives, 0.0);
    std::vector<double> reference_point;
    unsigned num_solvers, max_num_solutions = 800;

    for (unsigned i = 0; i < num_objectives; i++) {
      if (instance.senses[i] == NSBRKGA::Sense::MINIMIZE) {
        worst_values[i] = std::numeric_limits<double>::lowest();
        best_values[i] = std::numeric_limits<double>::max();
      } else {  // instance.senses[i] == NSBRKGA::Sense::MAXIMIZE
        worst_values[i] = std::numeric_limits<double>::max();
        best_values[i] = std::numeric_limits<double>::lowest();
      }
    }

    if (arg_parser.option_exists("--max-num-solutions")) {
      max_num_solutions =
          std::stoul(arg_parser.option_value("--max-num-solutions"));
    }

    for (num_solvers = 0;
         arg_parser.option_exists("--pareto-" + std::to_string(num_solvers)) ||
         arg_parser.option_exists("--best-solutions-snapshots-" +
                                  std::to_string(num_solvers));
         num_solvers++) {
    }

    for (unsigned i = 0; i < num_solvers; i++) {
      if (arg_parser.option_exists("--pareto-" + std::to_string(i))) {
        ifs.open(arg_parser.option_value("--pareto-" + std::to_string(i)));

        if (ifs.is_open()) {
          pareto.clear();

          for (std::string line; std::getline(ifs, line);) {
            std::istringstream iss(line);
            std::vector<double> value(num_objectives, 0.0);

            for (unsigned j = 0; j < num_objectives; j++) {
              iss >> value[j];
            }

            update_bounds(instance.senses, value, worst_values, best_values);

            pareto.push_back(std::make_pair(value, std::vector<double>()));
          }

          mopop::Solver::update_best_individuals(
              reference_pareto, pareto, instance.senses, max_num_solutions);

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

            best_solutions_snapshot.clear();

            ifs.ignore();

            for (std::string line; std::getline(ifs, line);) {
              std::istringstream iss(line);
              std::vector<double> value(num_objectives, 0.0);

              for (unsigned j = 0; j < num_objectives; j++) {
                iss >> value[j];
              }

              update_bounds(instance.senses, value, worst_values, best_values);

              best_solutions_snapshot.push_back(
                  std::make_pair(value, std::vector<double>()));
            }

            mopop::Solver::update_best_individuals(
                reference_pareto, best_solutions_snapshot, instance.senses,
                max_num_solutions);

            ifs.close();
          } else {
            break;
          }
        }
      }
    }

    reference_point =
        compute_reference_point(instance.senses, worst_values, best_values);

    if (arg_parser.option_exists("--reference-pareto")) {
      std::ofstream ofs;
      ofs.open(arg_parser.option_value("--reference-pareto"));

      if (ofs.is_open()) {
        for (const std::pair<std::vector<double>, std::vector<double>>&
                 solution : reference_pareto) {
          for (unsigned i = 0; i < solution.first.size() - 1; i++) {
            ofs << solution.first[i] << " ";
          }

          ofs << solution.first.back() << std::endl;

          if (ofs.eof() || ofs.fail() || ofs.bad()) {
            throw std::runtime_error(
                "Error writing file " +
                arg_parser.option_value("--reference-pareto") + ".");
          }
        }

        ofs.close();
      } else {
        throw std::runtime_error("File " +
                                 arg_parser.option_value("--reference-pareto") +
                                 " not created.");
      }
    }

    if (arg_parser.option_exists("--reference-point")) {
      std::ofstream ofs;
      ofs.open(arg_parser.option_value("--reference-point"));

      if (ofs.is_open()) {
        for (unsigned i = 0; i < reference_point.size() - 1; i++) {
          ofs << reference_point[i] << " ";
        }

        ofs << reference_point.back() << std::endl;

        if (ofs.eof() || ofs.fail() || ofs.bad()) {
          throw std::runtime_error(
              "Error writing file " +
              arg_parser.option_value("--reference-point") + ".");
        }

        ofs.close();
      } else {
        throw std::runtime_error("File " +
                                 arg_parser.option_value("--reference-point") +
                                 " not created.");
      }
    }
  } else {
    std::cerr
        << "./reference_pareto_front_and_point_calculator_exec "
        << "--expected-returns-filename <expected_returns_filename> "
        << "--covariance-filename <covariance_filename> "
        << "--max-num-solutions <max_num_solutions> "
        << "--pareto-i <pareto_filename> "
        << "--best-solutions-snapshots-i <best_solutions_snapshots_filename> "
        << "--reference-pareto <reference_pareto_filename> "
        << "--reference-point <reference_point_filename> " << std::endl;
  }

  return 0;
}
