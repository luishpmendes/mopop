#include "solver/nsbrkga/decoder.hpp"

#include <algorithm>

namespace mopop {

Decoder::Decoder(const Instance& instance, unsigned num_threads)
    : instance(instance),
      weights(num_threads, std::vector<double>(instance.num_assets, 0.0)),
      total_weights(num_threads, 0.0),
      values(num_threads, std::vector<double>(4, 0.0)) {}

std::vector<double> Decoder::decode(NSBRKGA::Chromosome& chromosome,
                                    bool rewrite) {
#ifdef _OPENMP
  std::vector<double>& weight = this->weights[omp_get_thread_num()];
  double& total_weight = this->total_weights[omp_get_thread_num()];
  std::vector<double>& value = this->values[omp_get_thread_num()];
#else
  std::vector<double>& weight = this->weights.front();
  double& total_weight = this->total_weights.front();
  std::vector<double>& value = this->values.front();
#endif
  total_weight = 0.0;

  for (unsigned i = 0; i < this->instance.num_assets; i++) {
    weight[i] = chromosome[i];
    total_weight += weight[i];
  }

  if (total_weight > 0.0) {
    for (unsigned i = 0; i < this->instance.num_assets; i++) {
      weight[i] /= total_weight;
    }
  }

  value[0] = 0.0;
  value[1] = 0.0;
  value[3] = 0.0;

  for (unsigned i = 0; i < this->instance.num_assets; i++) {
    value[0] += weight[i] * this->instance.expected_returns[i];

    for (unsigned j = 0; j < this->instance.num_assets; j++) {
      value[1] += weight[i] * weight[j] * instance.covariance_matrix[i][j];
    }

    if (weight[i] > 0.0) {
      value[3] -= weight[i] * std::log2(weight[i]);
    }
  }

  value[2] = value[0] / std::sqrt(value[1]);

  return value;
}

}  // namespace mopop
