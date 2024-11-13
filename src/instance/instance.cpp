#include "instance.hpp"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <iostream>

namespace mopop {

Instance::Instance() : num_assets(0), tickers(), expected_returns(), covariance_matrix(), senses() {}

void Instance::load_instance(const std::string & returns_file, const std::string & covariance_file) {
    this->parse_csv(returns_file, expected_returns);
    this->parse_csv(covariance_file, covariance_matrix);

    if (!covariance_matrix.empty()) {
        num_assets = covariance_matrix.size();
    } else {
        throw std::runtime_error("Covariance matrix is empty.");
    }

    senses = {NSBRKGA::Sense::MAXIMIZE, NSBRKGA::Sense::MINIMIZE}; // Maximizing returns, minimizing risk
}

void Instance::parse_csv(const std::string & filename, std::vector<std::string> & tickers, std::vector<double> & expected_returns) {
    std::ifstream file(filename);

    if (!file.is_open()) {
        throw std::runtime_error("Could not open file " + filename);
    }

    std::string line;

    while (std::getline(file, line)) {
        std::stringstream ss(line);
        std::string ticker;
        double value;

        std::getline(ss, ticker, ',');
        tickers.push_back(ticker);

        ss >> value;
        expected_returns.push_back(value);
    }
}

void Instance::parse_csv(const std::string & filename, std::vector<std::vector<double>> & matrix) {
    std::ifstream file(filename);

    if (!file.is_open()) {
        throw std::runtime_error("Could not open file " + filename);
    }

    std::string line;

    while (std::getline(file, line)) {
        std::stringstream ss(line);
        std::vector<double> row;
        double value;

        while (ss >> value) {
            row.push_back(value);
            if (ss.peek() == ',') ss.ignore();
        }

        matrix.push_back(row);
    }
}

std::ostream & operator <<(std::ostream & os, const Instance & instance) {
    os << "Number of assets: " << instance.num_assets << std::endl;

    os << "Tickers and Expected returns: " << std::endl;

    for (size_t i = 0; i < instance.tickers.size(); ++i) {
        os << instance.tickers[i] << ": " << instance.expected_returns[i] << std::endl;
    }

    os << std::endl;

    os << "Covariance matrix:" << std::endl;

    for (const auto & row : instance.covariance_matrix) {
        for (const auto &value : row) {
            os << value << " ";
        }

        os << std::endl;
    }

    return os;
}

}
