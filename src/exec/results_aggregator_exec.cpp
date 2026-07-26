#include <algorithm>
#include <cmath>
#include <fstream>
#include <numeric>

#include "utils/argument_parser.hpp"

int main(int argc, char* argv[]) {
  Argument_Parser arg_parser(argc, argv);
  std::vector<double> hvr_values, nigd_plus_values;

  unsigned num_hvrs, index_best, index_median, num_nigd_pluses;
  std::vector<std::pair<double, unsigned>> hvrs, nigd_pluses;

  for (num_hvrs = 0;
       arg_parser.option_exists("--hvr-" + std::to_string(num_hvrs));
       num_hvrs++) {
  }

  for (num_nigd_pluses = 0; arg_parser.option_exists(
           "--nigd-plus-" + std::to_string(num_nigd_pluses));
       num_nigd_pluses++) {
  }

  hvrs.resize(num_hvrs);
  nigd_pluses.resize(num_nigd_pluses);

  for (unsigned i = 0; i < num_hvrs; i++) {
    std::ifstream ifs;
    ifs.open(arg_parser.option_value("--hvr-" + std::to_string(i)));

    if (ifs.is_open()) {
      ifs >> hvrs[i].first;

      if (ifs.eof() || ifs.fail() || ifs.bad()) {
        throw std::runtime_error(
            "Error reading file " +
            arg_parser.option_value("--hvr-" + std::to_string(i)) + ".");
      }

      hvrs[i].second = i;
      ifs.close();
      hvr_values.push_back(hvrs[i].first);
    } else {
      throw std::runtime_error(
          "File " + arg_parser.option_value("--hvr-" + std::to_string(i)) +
          " not found.");
    }
  }

  for (unsigned i = 0; i < num_nigd_pluses; i++) {
    std::ifstream ifs;
    ifs.open(arg_parser.option_value("--nigd-plus-" + std::to_string(i)));

    if (ifs.is_open()) {
      ifs >> nigd_pluses[i].first;

      if (ifs.eof() || ifs.fail() || ifs.bad()) {
        throw std::runtime_error(
            "Error reading file " +
            arg_parser.option_value("--nigd-plus-" + std::to_string(i)) + ".");
      }

      nigd_pluses[i].second = i;
      ifs.close();
      nigd_plus_values.push_back(nigd_pluses[i].first);
    } else {
      throw std::runtime_error(
          "File " +
          arg_parser.option_value("--nigd-plus-" + std::to_string(i)) +
          " not found.");
    }
  }

  if (arg_parser.option_exists("--hvrs")) {
    std::ofstream ofs;
    ofs.open(arg_parser.option_value("--hvrs"));

    if (ofs.is_open()) {
      for (const std::pair<double, unsigned>& hv : hvrs) {
        ofs << hv.first << std::endl;

        if (ofs.eof() || ofs.fail() || ofs.bad()) {
          throw std::runtime_error("Error writing file " +
                                   arg_parser.option_value("--hvrs") + ".");
        }
      }

      ofs.close();
    } else {
      throw std::runtime_error("File " + arg_parser.option_value("--hvrs") +
                               " not created.");
    }
  }

  if (arg_parser.option_exists("--nigd-pluses")) {
    std::ofstream ofs;
    ofs.open(arg_parser.option_value("--nigd-pluses"));

    if (ofs.is_open()) {
      for (const std::pair<double, unsigned>& nigd_plus : nigd_pluses) {
        ofs << nigd_plus.first << std::endl;

        if (ofs.eof() || ofs.fail() || ofs.bad()) {
          throw std::runtime_error("Error writing file " +
                                   arg_parser.option_value("--nigd-pluses") +
                                   ".");
        }
      }

      ofs.close();
    } else {
      throw std::runtime_error(
          "File " + arg_parser.option_value("--nigd-pluses") + " not created.");
    }
  }

  if (arg_parser.option_exists("--hvr-statistics")) {
    std::ofstream ofs;
    ofs.open(arg_parser.option_value("--hvr-statistics"));

    if (ofs.is_open()) {
      double hvr_mean =
          std::accumulate(hvr_values.begin(), hvr_values.end(), 0.0) /
          hvr_values.size();
      double hvr_var =
          std::accumulate(hvr_values.begin(), hvr_values.end(), 0.0,
                          [hvr_mean](double acc, double val) {
                            return acc + (val - hvr_mean) * (val - hvr_mean);
                          }) /
          hvr_values.size();
      double hvr_std = std::sqrt(hvr_var);

      ofs << hvr_mean << ", " << hvr_std << std::endl;

      if (ofs.eof() || ofs.fail() || ofs.bad()) {
        throw std::runtime_error("Error writing file " +
                                 arg_parser.option_value("--hvr-statistics") +
                                 ".");
      }

      ofs.close();
    } else {
      throw std::runtime_error("File " +
                               arg_parser.option_value("--hvr-statistics") +
                               " not created.");
    }
  }

  if (arg_parser.option_exists("--nigd-plus-statistics")) {
    std::ofstream ofs;
    ofs.open(arg_parser.option_value("--nigd-plus-statistics"));

    if (ofs.is_open()) {
      double nigd_plus_mean = std::accumulate(nigd_plus_values.begin(),
                                              nigd_plus_values.end(), 0.0) /
                              nigd_plus_values.size();
      double nigd_plus_var =
          std::accumulate(nigd_plus_values.begin(), nigd_plus_values.end(), 0.0,
                          [nigd_plus_mean](double acc, double val) {
                            return acc + (val - nigd_plus_mean) *
                                             (val - nigd_plus_mean);
                          }) /
          nigd_plus_values.size();
      double nigd_plus_std = std::sqrt(nigd_plus_var);

      ofs << nigd_plus_mean << ", " << nigd_plus_std << std::endl;

      if (ofs.eof() || ofs.fail() || ofs.bad()) {
        throw std::runtime_error(
            "Error writing file " +
            arg_parser.option_value("--nigd-plus-statistics") + ".");
      }

      ofs.close();
    } else {
      throw std::runtime_error(
          "File " + arg_parser.option_value("--nigd-plus-statistics") +
          " not created.");
    }
  }

  std::sort(hvrs.begin(), hvrs.end());
  index_best = hvrs.back().second;
  index_median = hvrs[hvrs.size() / 2].second;

  if (arg_parser.option_exists("--statistics-best") &&
      arg_parser.option_exists("--statistics-" + std::to_string(index_best))) {
    std::ifstream ifs;
    ifs.open(
        arg_parser.option_value("--statistics-" + std::to_string(index_best)),
        std::ios::binary);

    if (ifs.is_open()) {
      std::ofstream ofs;
      ofs.open(arg_parser.option_value("--statistics-best"), std::ios::binary);

      if (ofs.is_open()) {
        ofs << ifs.rdbuf();

        if (ofs.eof() || ofs.fail() || ofs.bad()) {
          throw std::runtime_error(
              "Error writing file " +
              arg_parser.option_value("--statistics-best") + ".");
        }

        ofs.close();
      } else {
        throw std::runtime_error("File " +
                                 arg_parser.option_value("--statistics-best") +
                                 " not created.");
      }

      ifs.close();
    } else {
      throw std::runtime_error(
          "File " +
          arg_parser.option_value("--statistics-" +
                                  std::to_string(index_best)) +
          " not found.");
    }
  }

  if (arg_parser.option_exists("--statistics-median") &&
      arg_parser.option_exists("--statistics-" +
                               std::to_string(index_median))) {
    std::ifstream ifs;
    ifs.open(
        arg_parser.option_value("--statistics-" + std::to_string(index_median)),
        std::ios::binary);

    if (ifs.is_open()) {
      std::ofstream ofs;
      ofs.open(arg_parser.option_value("--statistics-median"),
               std::ios::binary);

      if (ofs.is_open()) {
        ofs << ifs.rdbuf();

        if (ofs.eof() || ofs.fail() || ofs.bad()) {
          throw std::runtime_error(
              "Error writing file " +
              arg_parser.option_value("--statistics-median") + ".");
        }

        ofs.close();
      } else {
        throw std::runtime_error(
            "File " + arg_parser.option_value("--statistics-median") +
            " not created.");
      }

      ifs.close();
    } else {
      throw std::runtime_error(
          "File " +
          arg_parser.option_value("--statistics-" +
                                  std::to_string(index_median)) +
          " not found.");
    }
  }

  if (arg_parser.option_exists("--pareto-best") &&
      arg_parser.option_exists("--pareto-" + std::to_string(index_best))) {
    std::ifstream ifs;
    ifs.open(arg_parser.option_value("--pareto-" + std::to_string(index_best)),
             std::ios::binary);

    if (ifs.is_open()) {
      std::ofstream ofs;
      ofs.open(arg_parser.option_value("--pareto-best"), std::ios::binary);

      if (ofs.is_open()) {
        ofs << ifs.rdbuf();

        if (ofs.eof() || ofs.fail() || ofs.bad()) {
          throw std::runtime_error("Error writing file " +
                                   arg_parser.option_value("--pareto-best") +
                                   ".");
        }

        ofs.close();
      } else {
        throw std::runtime_error("File " +
                                 arg_parser.option_value("--pareto-best") +
                                 " not created.");
      }

      ifs.close();
    } else {
      throw std::runtime_error(
          "File " +
          arg_parser.option_value("--pareto-" + std::to_string(index_best)) +
          " not found.");
    }
  }

  if (arg_parser.option_exists("--pareto-median") &&
      arg_parser.option_exists("--pareto-" + std::to_string(index_median))) {
    std::ifstream ifs;
    ifs.open(
        arg_parser.option_value("--pareto-" + std::to_string(index_median)),
        std::ios::binary);

    if (ifs.is_open()) {
      std::ofstream ofs;
      ofs.open(arg_parser.option_value("--pareto-median"), std::ios::binary);

      if (ofs.is_open()) {
        ofs << ifs.rdbuf();

        if (ofs.eof() || ofs.fail() || ofs.bad()) {
          throw std::runtime_error("Error writing file " +
                                   arg_parser.option_value("--pareto-median") +
                                   ".");
        }

        ofs.close();
      } else {
        throw std::runtime_error("File " +
                                 arg_parser.option_value("--pareto-median") +
                                 " not created.");
      }

      ifs.close();
    } else {
      throw std::runtime_error(
          "File " +
          arg_parser.option_value("--pareto-" + std::to_string(index_median)) +
          " not found.");
    }
  }

  if (arg_parser.option_exists("--hvr-snapshots-best") &&
      arg_parser.option_exists("--hvr-snapshots-" +
                               std::to_string(index_best))) {
    std::ifstream ifs;
    ifs.open(arg_parser.option_value("--hvr-snapshots-" +
                                     std::to_string(index_best)),
             std::ios::binary);

    if (ifs.is_open()) {
      std::ofstream ofs;
      ofs.open(arg_parser.option_value("--hvr-snapshots-best"),
               std::ios::binary);

      if (ofs.is_open()) {
        ofs << ifs.rdbuf();

        if (ofs.eof() || ofs.fail() || ofs.bad()) {
          throw std::runtime_error(
              "Error writing file " +
              arg_parser.option_value("--hvr-snapshots-best") + ".");
        }

        ofs.close();
      } else {
        throw std::runtime_error(
            "File " + arg_parser.option_value("--hvr-snapshots-best") +
            " not created.");
      }

      ifs.close();
    } else {
      throw std::runtime_error(
          "File " +
          arg_parser.option_value("--hvr-snapshots-" +
                                  std::to_string(index_best)) +
          " not found.");
    }
  }

  if (arg_parser.option_exists("--hvr-snapshots-median") &&
      arg_parser.option_exists("--hvr-snapshots-" +
                               std::to_string(index_median))) {
    std::ifstream ifs;
    ifs.open(arg_parser.option_value("--hvr-snapshots-" +
                                     std::to_string(index_median)),
             std::ios::binary);

    if (ifs.is_open()) {
      std::ofstream ofs;
      ofs.open(arg_parser.option_value("--hvr-snapshots-median"),
               std::ios::binary);

      if (ofs.is_open()) {
        ofs << ifs.rdbuf();

        if (ofs.eof() || ofs.fail() || ofs.bad()) {
          throw std::runtime_error(
              "Error writing file " +
              arg_parser.option_value("--hvr-snapshots-median") + ".");
        }

        ofs.close();
      } else {
        throw std::runtime_error(
            "File " + arg_parser.option_value("--hvr-snapshots-median") +
            " not created.");
      }

      ifs.close();
    } else {
      throw std::runtime_error(
          "File " +
          arg_parser.option_value("--hvr-snapshots-" +
                                  std::to_string(index_median)) +
          " not found.");
    }
  }

  if (arg_parser.option_exists("--best-solutions-snapshots-best") &&
      arg_parser.option_exists("--best-solutions-snapshots-" +
                               std::to_string(index_best))) {
    std::string best_solutions_snapshots_best_filename =
        arg_parser.option_value("--best-solutions-snapshots-best");
    std::string best_solutions_snapshots_index_best_filename =
        arg_parser.option_value("--best-solutions-snapshots-" +
                                std::to_string(index_best));

    for (unsigned i = 0;; i++) {
      std::ifstream ifs;
      ifs.open(best_solutions_snapshots_index_best_filename +
                   std::to_string(i) + ".txt",
               std::ios::binary);

      if (ifs.is_open()) {
        std::ofstream ofs;
        ofs.open(
            best_solutions_snapshots_best_filename + std::to_string(i) + ".txt",
            std::ios::binary);

        if (ofs.is_open()) {
          ofs << ifs.rdbuf();

          if (ofs.eof() || ofs.fail() || ofs.bad()) {
            throw std::runtime_error("Error writing file " +
                                     best_solutions_snapshots_best_filename +
                                     std::to_string(i) + ".txt.");
          }

          ofs.close();
        } else {
          throw std::runtime_error("File " +
                                   best_solutions_snapshots_best_filename +
                                   std::to_string(i) + ".txt not created.");
        }

        ifs.close();
      } else {
        break;
      }
    }
  }

  if (arg_parser.option_exists("--best-solutions-snapshots-median") &&
      arg_parser.option_exists("--best-solutions-snapshots-" +
                               std::to_string(index_median))) {
    std::string best_solutions_snapshots_median_filename =
        arg_parser.option_value("--best-solutions-snapshots-median");
    std::string best_solutions_snapshots_index_median_filename =
        arg_parser.option_value("--best-solutions-snapshots-" +
                                std::to_string(index_median));

    for (unsigned i = 0;; i++) {
      std::ifstream ifs;
      ifs.open(best_solutions_snapshots_index_median_filename +
                   std::to_string(i) + ".txt",
               std::ios::binary);

      if (ifs.is_open()) {
        std::ofstream ofs;
        ofs.open(best_solutions_snapshots_median_filename + std::to_string(i) +
                     ".txt",
                 std::ios::binary);

        if (ofs.is_open()) {
          ofs << ifs.rdbuf();

          if (ofs.eof() || ofs.fail() || ofs.bad()) {
            throw std::runtime_error("Error writing file " +
                                     best_solutions_snapshots_median_filename +
                                     std::to_string(i) + ".txt.");
          }

          ofs.close();
        } else {
          throw std::runtime_error("File " +
                                   best_solutions_snapshots_median_filename +
                                   std::to_string(i) + ".txt not created.");
        }

        ifs.close();
      } else {
        break;
      }
    }
  }

  if (arg_parser.option_exists("--num-non-dominated-snapshots-best") &&
      arg_parser.option_exists("--num-non-dominated-snapshots-" +
                               std::to_string(index_best))) {
    std::ifstream ifs;
    ifs.open(arg_parser.option_value("--num-non-dominated-snapshots-" +
                                     std::to_string(index_best)),
             std::ios::binary);

    if (ifs.is_open()) {
      std::ofstream ofs;
      ofs.open(arg_parser.option_value("--num-non-dominated-snapshots-best"),
               std::ios::binary);

      if (ofs.is_open()) {
        ofs << ifs.rdbuf();

        if (ofs.eof() || ofs.fail() || ofs.bad()) {
          throw std::runtime_error(
              "Error writing file " +
              arg_parser.option_value("--num-non-dominated-snapshots-best") +
              ".");
        }

        ofs.close();
      } else {
        throw std::runtime_error(
            "File " +
            arg_parser.option_value("--num-non-dominated-snapshots-best") +
            " not created.");
      }

      ifs.close();
    } else {
      throw std::runtime_error(
          "File " +
          arg_parser.option_value("--num-non-dominated-snapshots-" +
                                  std::to_string(index_best)) +
          " not found.");
    }
  }

  if (arg_parser.option_exists("--num-non-dominated-snapshots-median") &&
      arg_parser.option_exists("--num-non-dominated-snapshots-" +
                               std::to_string(index_median))) {
    std::ifstream ifs;
    ifs.open(arg_parser.option_value("--num-non-dominated-snapshots-" +
                                     std::to_string(index_median)),
             std::ios::binary);

    if (ifs.is_open()) {
      std::ofstream ofs;
      ofs.open(arg_parser.option_value("--num-non-dominated-snapshots-median"),
               std::ios::binary);

      if (ofs.is_open()) {
        ofs << ifs.rdbuf();

        if (ofs.eof() || ofs.fail() || ofs.bad()) {
          throw std::runtime_error(
              "Error writing file " +
              arg_parser.option_value("--num-non-dominated-snapshots-median") +
              ".");
        }

        ofs.close();
      } else {
        throw std::runtime_error(
            "File " +
            arg_parser.option_value("--num-non-dominated-snapshots-median") +
            " not created.");
      }

      ifs.close();
    } else {
      throw std::runtime_error(
          "File " +
          arg_parser.option_value("--num-non-dominated-snapshots-" +
                                  std::to_string(index_median)) +
          " not found.");
    }
  }

  if (arg_parser.option_exists("--num-fronts-snapshots-best") &&
      arg_parser.option_exists("--num-fronts-snapshots-" +
                               std::to_string(index_best))) {
    std::ifstream ifs;
    ifs.open(arg_parser.option_value("--num-fronts-snapshots-" +
                                     std::to_string(index_best)),
             std::ios::binary);

    if (ifs.is_open()) {
      std::ofstream ofs;
      ofs.open(arg_parser.option_value("--num-fronts-snapshots-best"),
               std::ios::binary);

      if (ofs.is_open()) {
        ofs << ifs.rdbuf();

        if (ofs.eof() || ofs.fail() || ofs.bad()) {
          throw std::runtime_error(
              "Error writing file " +
              arg_parser.option_value("--num-non-dominated-snapshots-best") +
              ".");
        }

        ofs.close();
      } else {
        throw std::runtime_error(
            "File " + arg_parser.option_value("--num-fronts-snapshots-best") +
            " not created.");
      }

      ifs.close();
    } else {
      throw std::runtime_error(
          "File " +
          arg_parser.option_value("--num-fronts-snapshots-" +
                                  std::to_string(index_best)) +
          " not found.");
    }
  }

  if (arg_parser.option_exists("--num-fronts-snapshots-median") &&
      arg_parser.option_exists("--num-fronts-snapshots-" +
                               std::to_string(index_median))) {
    std::ifstream ifs;
    ifs.open(arg_parser.option_value("--num-fronts-snapshots-" +
                                     std::to_string(index_median)),
             std::ios::binary);

    if (ifs.is_open()) {
      std::ofstream ofs;
      ofs.open(arg_parser.option_value("--num-fronts-snapshots-median"),
               std::ios::binary);

      if (ofs.is_open()) {
        ofs << ifs.rdbuf();

        if (ofs.eof() || ofs.fail() || ofs.bad()) {
          throw std::runtime_error(
              "Error writing file " +
              arg_parser.option_value("--num-fronts-snapshots-median") + ".");
        }

        ofs.close();
      } else {
        throw std::runtime_error(
            "File " + arg_parser.option_value("--num-fronts-snapshots-median") +
            " not created.");
      }

      ifs.close();
    } else {
      throw std::runtime_error(
          "File " +
          arg_parser.option_value("--num-fronts-snapshots-" +
                                  std::to_string(index_median)) +
          " not found.");
    }
  }

  if (arg_parser.option_exists("--populations-snapshots-best") &&
      arg_parser.option_exists("--populations-snapshots-" +
                               std::to_string(index_best))) {
    std::string populations_snapshots_best_filename =
        arg_parser.option_value("--populations-snapshots-best");
    std::string populations_snapshots_index_best_filename =
        arg_parser.option_value("--populations-snapshots-" +
                                std::to_string(index_best));

    for (unsigned i = 0;; i++) {
      std::ifstream ifs;
      ifs.open(populations_snapshots_index_best_filename + std::to_string(i) +
                   ".txt",
               std::ios::binary);

      if (ifs.is_open()) {
        std::ofstream ofs;
        ofs.open(
            populations_snapshots_best_filename + std::to_string(i) + ".txt",
            std::ios::binary);

        if (ofs.is_open()) {
          ofs << ifs.rdbuf();

          if (ofs.eof() || ofs.fail() || ofs.bad()) {
            throw std::runtime_error("Error writing file " +
                                     populations_snapshots_best_filename +
                                     std::to_string(i) + ".txt.");
          }

          ofs.close();
        } else {
          throw std::runtime_error("File " +
                                   populations_snapshots_best_filename +
                                   std::to_string(i) + ".txt not created.");
        }

        ifs.close();
      } else {
        break;
      }
    }
  }

  if (arg_parser.option_exists("--populations-snapshots-median") &&
      arg_parser.option_exists("--populations-snapshots-" +
                               std::to_string(index_median))) {
    std::string populations_snapshots_median_filename =
        arg_parser.option_value("--populations-snapshots-median");
    std::string populations_snapshots_index_median_filename =
        arg_parser.option_value("--populations-snapshots-" +
                                std::to_string(index_median));

    for (unsigned i = 0;; i++) {
      std::ifstream ifs;
      ifs.open(populations_snapshots_index_median_filename + std::to_string(i) +
                   ".txt",
               std::ios::binary);

      if (ifs.is_open()) {
        std::ofstream ofs;
        ofs.open(
            populations_snapshots_median_filename + std::to_string(i) + ".txt",
            std::ios::binary);

        if (ofs.is_open()) {
          ofs << ifs.rdbuf();

          if (ofs.eof() || ofs.fail() || ofs.bad()) {
            throw std::runtime_error("Error writing file " +
                                     populations_snapshots_median_filename +
                                     std::to_string(i) + ".txt.");
          }

          ofs.close();
        } else {
          throw std::runtime_error("File " +
                                   populations_snapshots_median_filename +
                                   std::to_string(i) + ".txt not created.");
        }

        ifs.close();
      } else {
        break;
      }
    }
  }

  if (arg_parser.option_exists("--num-elites-snapshots-best") &&
      arg_parser.option_exists("--num-elites-snapshots-" +
                               std::to_string(index_best))) {
    std::ifstream ifs;
    ifs.open(arg_parser.option_value("--num-elites-snapshots-" +
                                     std::to_string(index_best)),
             std::ios::binary);

    if (ifs.is_open()) {
      std::ofstream ofs;
      ofs.open(arg_parser.option_value("--num-elites-snapshots-best"),
               std::ios::binary);

      if (ofs.is_open()) {
        ofs << ifs.rdbuf();

        if (ofs.eof() || ofs.fail() || ofs.bad()) {
          throw std::runtime_error(
              "Error writing file " +
              arg_parser.option_value("--num-elites-snapshots-best") + ".");
        }

        ofs.close();
      } else {
        throw std::runtime_error(
            "File " + arg_parser.option_value("--num-elites-snapshots-best") +
            " not created.");
      }

      ifs.close();
    } else {
      throw std::runtime_error(
          "File " +
          arg_parser.option_value("--num-elites-snapshots-" +
                                  std::to_string(index_best)) +
          " not found.");
    }
  }

  if (arg_parser.option_exists("--num-elites-snapshots-median") &&
      arg_parser.option_exists("--num-elites-snapshots-" +
                               std::to_string(index_median))) {
    std::ifstream ifs;
    ifs.open(arg_parser.option_value("--num-elites-snapshots-" +
                                     std::to_string(index_median)),
             std::ios::binary);

    if (ifs.is_open()) {
      std::ofstream ofs;
      ofs.open(arg_parser.option_value("--num-elites-snapshots-median"),
               std::ios::binary);

      if (ofs.is_open()) {
        ofs << ifs.rdbuf();

        if (ofs.eof() || ofs.fail() || ofs.bad()) {
          throw std::runtime_error(
              "Error writing file " +
              arg_parser.option_value("--num-elites-snapshots-median") + ".");
        }

        ofs.close();
      } else {
        throw std::runtime_error(
            "File " + arg_parser.option_value("--num-elites-snapshots-median") +
            " not created.");
      }

      ifs.close();
    } else {
      throw std::runtime_error(
          "File " +
          arg_parser.option_value("--num-elites-snapshots-" +
                                  std::to_string(index_median)) +
          " not found.");
    }
  }

  if (arg_parser.option_exists("--num-mutants-snapshots-best") &&
      arg_parser.option_exists("--num-mutants-snapshots-" +
                               std::to_string(index_best))) {
    std::ifstream ifs;
    ifs.open(arg_parser.option_value("--num-mutants-snapshots-" +
                                     std::to_string(index_best)),
             std::ios::binary);

    if (ifs.is_open()) {
      std::ofstream ofs;
      ofs.open(arg_parser.option_value("--num-mutants-snapshots-best"),
               std::ios::binary);

      if (ofs.is_open()) {
        ofs << ifs.rdbuf();

        if (ofs.eof() || ofs.fail() || ofs.bad()) {
          throw std::runtime_error(
              "Error writing file " +
              arg_parser.option_value("--num-mutants-snapshots-best") + ".");
        }

        ofs.close();
      } else {
        throw std::runtime_error(
            "File " + arg_parser.option_value("--num-mutants-snapshots-best") +
            " not created.");
      }

      ifs.close();
    } else {
      throw std::runtime_error(
          "File " +
          arg_parser.option_value("--num-mutants-snapshots-" +
                                  std::to_string(index_best)) +
          " not found.");
    }
  }

  if (arg_parser.option_exists("--num-mutants-snapshots-median") &&
      arg_parser.option_exists("--num-mutants-snapshots-" +
                               std::to_string(index_median))) {
    std::ifstream ifs;
    ifs.open(arg_parser.option_value("--num-mutants-snapshots-" +
                                     std::to_string(index_median)),
             std::ios::binary);

    if (ifs.is_open()) {
      std::ofstream ofs;
      ofs.open(arg_parser.option_value("--num-mutants-snapshots-median"),
               std::ios::binary);

      if (ofs.is_open()) {
        ofs << ifs.rdbuf();
        ofs.close();
      } else {
        throw std::runtime_error(
            "File " +
            arg_parser.option_value("--num-mutants-snapshots-median") +
            " not created.");
      }

      ifs.close();
    } else {
      throw std::runtime_error(
          "File " +
          arg_parser.option_value("--num-mutants-snapshots-" +
                                  std::to_string(index_median)) +
          " not found.");
    }
  }

  return 0;
}
