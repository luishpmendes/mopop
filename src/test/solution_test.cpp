#include "solution/solution.hpp"

#include <cassert>
#include <fstream>
#include <iostream>

int main() {
  mopop::Instance instance;
  mopop::Solution solution;

  const std::string expected_returns_filename =
                        "input/expected_returns_test.csv",
                    covariance_filename = "input/covariance_matrix_test.csv";

  instance = mopop::Instance(expected_returns_filename, covariance_filename);

  {
    std::vector<double> key = {0.5, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
    mopop::Solution solution(instance, key);

    assert(solution.is_feasible());
    assert(solution.weight.size() == 7);
    assert(fabs(solution.weight[0] - 1.0) <
           std::numeric_limits<double>::epsilon());
    assert(solution.value.size() == 4);
    assert(fabs(solution.value[0] - 0.0012912465706528247) <
           std::numeric_limits<double>::epsilon());
    assert(fabs(solution.value[1] - 0.00018574179740743447) <
           std::numeric_limits<double>::epsilon());
    assert(fabs(solution.value[2] - 0.094744576568404567) <
           std::numeric_limits<double>::epsilon());
    assert(fabs(solution.value[3] - 0.0) <
           std::numeric_limits<double>::epsilon());

    std::cout << solution << std::endl;
  }

  {
    std::vector<double> key = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.5};
    mopop::Solution solution(instance, key);

    assert(solution.is_feasible());
    assert(solution.weight.size() == 7);
    assert(fabs(solution.weight[6] - 1.0) <
           std::numeric_limits<double>::epsilon());
    assert(solution.value.size() == 4);
    assert(fabs(solution.value[0] - 0.005107159883158241) <
           std::numeric_limits<double>::epsilon());
    assert(fabs(solution.value[1] - 0.001061156598370683) <
           std::numeric_limits<double>::epsilon());
    assert(fabs(solution.value[2] - 0.1567796586383924) <
           std::numeric_limits<double>::epsilon());
    assert(fabs(solution.value[3] - 0.0) <
           std::numeric_limits<double>::epsilon());

    std::cout << solution << std::endl;
  }

  {
    std::vector<double> key = {0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5};
    mopop::Solution solution(instance, key);

    assert(solution.is_feasible());
    assert(solution.weight.size() == 7);
    assert(fabs(solution.weight[0] - 1.0 / 7.0) <
           std::numeric_limits<double>::epsilon());
    assert(fabs(solution.weight[6] - 1.0 / 7.0) <
           std::numeric_limits<double>::epsilon());
    assert(solution.value.size() == 4);
    assert(fabs(solution.value[0] - 0.00232273) < 0.00000001);
    assert(fabs(solution.value[1] - 0.000202819) < 0.000000001);
    assert(fabs(solution.value[2] - 0.163096) < 0.000001);
    assert(
        fabs(
            solution.value[3] -
            2.8073549220576041074419693172318308086410266259661407836772917240) <
        std::numeric_limits<double>::epsilon());

    std::cout << solution << std::endl;
  }

  std::cout << std::endl << "Solution Test PASSED" << std::endl;

  return 0;
}
