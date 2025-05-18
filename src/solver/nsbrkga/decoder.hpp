#pragma once

#include "chromosome.hpp"
#include "solution/solution.hpp"

namespace mopop {

class Decoder {
 public:
  const Instance& instance;

  std::vector<std::vector<double>> weights;

  std::vector<double> total_weights;

  std::vector<std::vector<double>> values;

  Decoder(const Instance& instance, unsigned num_threads);

  std::vector<double> decode(NSBRKGA::Chromosome& chromosome, bool rewrite);
};

}  // namespace mopop
