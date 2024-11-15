#pragma once

#define NSBRKGA_MULTIPLE_INCLUSIONS

#include <istream>
#include <ostream>
#include <vector>

#include "nsbrkga.hpp"

namespace mopop {
class Instance {
   public:
    unsigned num_assets;

    std::vector<std::string> tickers;

    std::vector<double> expected_returns;

    std::vector<std::vector<double>> covariance_matrix;

    std::vector<NSBRKGA::Sense> senses;

   private:
    void load_instance(const std::string& expected_returns_filename, const std::string& covariance_filename);

   public:
    Instance(const std::vector<std::string>& tickers, const std::vector<double>& expected_returns,
             const std::vector<std::vector<double>>& covariance_matrix);

    Instance(const std::string& expected_returns_filename, const std::string& covariance_file);

    Instance(const Instance& instance);

    Instance();

    friend std::ostream& operator<<(std::ostream& os, const Instance& instance);
};

}  // namespace mopop
