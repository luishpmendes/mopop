#include "instance.hpp"

#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>

namespace mopop {

void Instance::load_instance(const std::string &expected_returns_filename, const std::string &covariance_filename) {
    std::ifstream expected_returns_file(expected_returns_filename), covariance_file(covariance_filename);
    std::string line;

    if (!expected_returns_file.is_open()) {
        throw std::runtime_error("Unable to open returns file");
    }

    std::getline(expected_returns_file, line);
    this->tickers.clear();

    while (std::getline(expected_returns_file, line)) {
        std::istringstream linestream(line);
        std::string ticker, expected_return_str;
        double expected_return;

        if (std::getline(linestream, ticker, ',') && std::getline(linestream, expected_return_str, ',')) {
            this->tickers.push_back(ticker);
            expected_return = std::stod(expected_return_str);
            this->expected_returns.push_back(expected_return);
        }
    }

    expected_returns_file.close();

    if (!covariance_file.is_open()) {
        throw std::runtime_error("Unable to open covariance file");
    }

    std::getline(covariance_file, line);
    this->covariance_matrix.clear();

    while (std::getline(covariance_file, line)) {
        std::istringstream linestream(line);
        std::string ticker;
        std::vector<double> row;
        std::string value_str;

        std::getline(linestream, ticker, ',');

        while (std::getline(linestream, value_str, ',')) {
            row.push_back(std::stod(value_str));
        }

        this->covariance_matrix.push_back(row);
    }

    covariance_file.close();
    this->num_assets = tickers.size();
    this->senses = {NSBRKGA::Sense::MAXIMIZE, NSBRKGA::Sense::MINIMIZE};
}

Instance::Instance() : num_assets(0), tickers(), expected_returns(), covariance_matrix(), senses() {}

Instance::Instance(const std::vector<std::string> &tickers, const std::vector<double> &expected_returns,
                   const std::vector<std::vector<double>> &covariance_matrix)
    : num_assets(covariance_matrix.size()),
      tickers(tickers),
      expected_returns(expected_returns),
      covariance_matrix(covariance_matrix),
      senses({NSBRKGA::Sense::MAXIMIZE, NSBRKGA::Sense::MINIMIZE}) {}

Instance::Instance(const std::string &returns_filename, const std::string &covariance_filename)
    : num_assets(0), tickers(), expected_returns(), covariance_matrix(), senses() {
    this->load_instance(returns_filename, covariance_filename);
}

Instance::Instance(const Instance &instance) = default;

std::ostream &operator<<(std::ostream &os, const Instance &instance) {
    os << "Number of assets: " << instance.num_assets << std::endl;

    os << "Tickers and Expected returns: " << std::endl;

    for (size_t i = 0; i < instance.tickers.size(); ++i) {
        os << instance.tickers[i] << ": " << instance.expected_returns[i] << std::endl;
    }

    os << std::endl;

    os << "Covariance matrix:" << std::endl;

    for (const auto &row : instance.covariance_matrix) {
        for (const auto &value : row) {
            os << value << " ";
        }

        os << std::endl;
    }

    return os;
}

}  // namespace mopop
