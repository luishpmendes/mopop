#include "instance/instance.hpp"

#include <cassert>
#include <fstream>
#include <iostream>

int main() {
  mopop::Instance instance;

  const std::string expected_returns_filename =
                        "input/expected_returns_test.csv",
                    covariance_filename = "input/covariance_matrix_test.csv";

  instance = mopop::Instance(expected_returns_filename, covariance_filename);

  assert(instance.num_assets == 7);
  assert(instance.tickers.size() == 7);
  assert(instance.tickers.front() == "AAPL34.SA");
  assert(instance.tickers.back() == "NVDC34.SA");
  assert(instance.expected_returns.size() == 7);
  assert(instance.expected_returns.front() == 0.0012912465706528247);
  assert(instance.expected_returns.back() == 0.005107159883158241);
  assert(instance.covariance_matrix.size() == 7);
  assert(instance.covariance_matrix.front().size() == 7);
  assert(instance.covariance_matrix.front().front() == 0.00018574179740743447);
  assert(instance.covariance_matrix.back().size() == 7);
  assert(instance.covariance_matrix.back().front() == 0.00020082888001011015);
  assert(instance.covariance_matrix.back().back() == 0.001061156598370683);

  std::cout << instance << std::endl;

  std::cout << std::endl << "Instance Test PASSED" << std::endl;

  return 0;
}
