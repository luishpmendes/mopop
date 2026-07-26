#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>
#include <pagmo/utils/hypervolume.hpp>
#include <vector>

#include "instance/instance.hpp"

/*
 * The quality indicator formulas live in the metric executables, inside their
 * main() translation units, so they cannot be linked against. This test mirrors
 * them over an analytic mixed-sense fixture: it is the formulas themselves that
 * are under test, since that is where the reference point construction, the
 * negation transform and the modified distance can go wrong silently.
 *
 * Keep these copies in sync with:
 *   src/exec/reference_pareto_front_and_point_calculator_exec.cpp
 *   src/exec/hypervolume_calculator_exec.cpp
 *   src/exec/hypervolume_ratio_calculator_exec.cpp
 *   src/exec/normalized_modified_generational_distance_calculator_exec.cpp
 */

/**
 * @brief Mirrors update_bounds from the reference point calculator.
 */
static void update_bounds(const std::vector<NSBRKGA::Sense>& senses,
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
    } else {
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
 * @brief Mirrors compute_reference_point from the reference point calculator.
 */
static std::vector<double> compute_reference_point(
    const std::vector<NSBRKGA::Sense>& senses,
    const std::vector<double>& worst_values,
    const std::vector<double>& best_values) {
  std::vector<double> reference_point(senses.size(), 0.0);

  for (unsigned i = 0; i < senses.size(); i++) {
    double padding = 0.05 * std::fabs(best_values[i] - worst_values[i]);

    if (padding < std::numeric_limits<double>::epsilon()) {
      padding = 0.05 * std::fabs(worst_values[i]);
    }

    if (padding < std::numeric_limits<double>::epsilon()) {
      padding = 0.05;
    }

    if (senses[i] == NSBRKGA::Sense::MINIMIZE) {
      reference_point[i] = worst_values[i] + padding;
    } else {
      reference_point[i] = worst_values[i] - padding;
    }
  }

  return reference_point;
}

/**
 * @brief Runs the whole reference point construction over a set of fronts.
 */
static std::vector<double> reference_point_of(
    const std::vector<NSBRKGA::Sense>& senses,
    const std::vector<std::vector<double>>& points) {
  std::vector<double> worst_values(senses.size(), 0.0);
  std::vector<double> best_values(senses.size(), 0.0);

  for (unsigned i = 0; i < senses.size(); i++) {
    if (senses[i] == NSBRKGA::Sense::MINIMIZE) {
      worst_values[i] = std::numeric_limits<double>::lowest();
      best_values[i] = std::numeric_limits<double>::max();
    } else {
      worst_values[i] = std::numeric_limits<double>::max();
      best_values[i] = std::numeric_limits<double>::lowest();
    }
  }

  for (const std::vector<double>& point : points) {
    update_bounds(senses, point, worst_values, best_values);
  }

  return compute_reference_point(senses, worst_values, best_values);
}

/**
 * @brief Mirrors compute_hypervolume from the hypervolume calculators.
 */
static double compute_hypervolume(
    const std::vector<NSBRKGA::Sense>& senses,
    const std::vector<double>& reference_point,
    const std::vector<std::vector<double>>& front) {
  if (front.empty()) {
    return 0.0;
  }

  std::vector<double> reference_point_prime(reference_point.size());
  std::vector<std::vector<double>> front_prime(front.size());

  for (unsigned i = 0; i < reference_point.size(); i++) {
    if (senses[i] == NSBRKGA::Sense::MINIMIZE) {
      reference_point_prime[i] = reference_point[i];
    } else {
      reference_point_prime[i] = -reference_point[i];
    }
  }

  for (unsigned i = 0; i < front.size(); i++) {
    front_prime[i] = std::vector<double>(front[i].size());
    for (unsigned j = 0; j < front[i].size(); j++) {
      if (senses[j] == NSBRKGA::Sense::MINIMIZE) {
        front_prime[i][j] = front[i][j];
      } else {
        front_prime[i][j] = -front[i][j];
      }
    }
  }

  pagmo::hypervolume hv(front_prime);
  return hv.compute(reference_point_prime);
}

/**
 * @brief Mirrors modified_distance from the NIGD+ calculator.
 */
static double modified_distance(const std::vector<NSBRKGA::Sense>& senses,
                                const std::vector<double>& reference_point,
                                const std::vector<double>& point) {
  double distance = 0.0, delta;

  for (unsigned i = 0; i < senses.size(); i++) {
    delta = 0;

    if (senses[i] == NSBRKGA::Sense::MINIMIZE) {
      if (point[i] > reference_point[i]) {
        delta = point[i] - reference_point[i];
      }
    } else {
      if (reference_point[i] > point[i]) {
        delta = reference_point[i] - point[i];
      }
    }

    distance += delta * delta;
  }

  return sqrt(distance);
}

/**
 * @brief Mirrors modified_inverted_generational_distance from the NIGD+
 * calculator.
 */
static double modified_inverted_generational_distance(
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
 * @brief Compares two doubles up to a tolerance.
 */
static bool almost_equal(const double& a, const double& b,
                         const double& tolerance = 1e-9) {
  return std::fabs(a - b) <= tolerance;
}

int main() {
  // The senses of the problem, as fixed by the Instance.
  mopop::Instance instance("input/expected_returns_test.csv",
                           "input/covariance_matrix_test.csv");

  assert(instance.is_valid());
  assert(instance.senses.size() == 4);
  assert(instance.senses[0] == NSBRKGA::Sense::MAXIMIZE);
  assert(instance.senses[1] == NSBRKGA::Sense::MINIMIZE);
  assert(instance.senses[2] == NSBRKGA::Sense::MAXIMIZE);
  assert(instance.senses[3] == NSBRKGA::Sense::MINIMIZE);

  const std::vector<NSBRKGA::Sense>& senses = instance.senses;

  // Two mutually non dominated points: A leads on the maximization objectives,
  // B leads on the minimization ones.
  std::vector<double> a = {3.0, 6.0, 3.0, 6.0};
  std::vector<double> b = {1.0, 2.0, 1.0, 2.0};
  std::vector<std::vector<double>> reference_front = {a, b};

  // The reference point takes the worst attained value of every objective,
  // pushed outward by 5% of that objective's attained range.
  std::vector<double> reference_point =
      reference_point_of(senses, reference_front);

  assert(almost_equal(reference_point[0], 0.9));
  assert(almost_equal(reference_point[1], 6.2));
  assert(almost_equal(reference_point[2], 0.9));
  assert(almost_equal(reference_point[3], 6.2));

  // Every attained point is strictly better than the reference point on every
  // objective, so no point sits on the boundary and contributes nothing.
  for (const std::vector<double>& point : reference_front) {
    for (unsigned i = 0; i < senses.size(); i++) {
      if (senses[i] == NSBRKGA::Sense::MINIMIZE) {
        assert(point[i] < reference_point[i]);
      } else {
        assert(point[i] > reference_point[i]);
      }
    }
  }

  // Analytic hypervolume: the union of two boxes of volume 2.1 * 0.2 * 2.1 *
  // 0.2 = 0.1764 overlapping in a box of volume 0.1 * 0.2 * 0.1 * 0.2 = 0.0004.
  double reference_hypervolume =
      compute_hypervolume(senses, reference_point, reference_front);

  assert(almost_equal(reference_hypervolume, 0.3524));

  // A front holding a single point covers just its own box.
  double hypervolume_a = compute_hypervolume(senses, reference_point, {a});

  assert(almost_equal(hypervolume_a, 0.1764));

  // A candidate front equal to the reference front scores exactly one. This is
  // the case the removed 5% front perturbation used to hide.
  double hypervolume_ratio =
      compute_hypervolume(senses, reference_point, reference_front) /
      reference_hypervolume;

  assert(almost_equal(hypervolume_ratio, 1.0, 1e-12));
  assert(hypervolume_ratio <= 1.0 + 1e-9);

  // A strictly smaller candidate front scores strictly between zero and one.
  double partial_ratio = hypervolume_a / reference_hypervolume;

  assert(partial_ratio > 0.0);
  assert(partial_ratio < 1.0);
  assert(almost_equal(partial_ratio, 0.1764 / 0.3524));

  // An empty front scores zero instead of reaching pagmo.
  assert(almost_equal(compute_hypervolume(senses, reference_point, {}), 0.0));

  // The modified distance only charges the objectives on which the point is
  // worse than the reference point, whatever the sense of each objective.
  std::vector<double> centre = {2.0, 2.0, 2.0, 2.0};
  std::vector<double> worse_everywhere = {1.0, 3.0, 1.0, 3.0};
  std::vector<double> better_everywhere = {3.0, 1.0, 3.0, 1.0};

  assert(almost_equal(modified_distance(senses, centre, worse_everywhere), 2.0));
  assert(
      almost_equal(modified_distance(senses, centre, better_everywhere), 0.0));

  // The normalizer is the distance from the reference front to the reference
  // point, which is the largest value any front can reach.
  double reference_igd_plus = modified_inverted_generational_distance(
      senses, reference_front, {reference_point});

  assert(reference_igd_plus > 0.0);
  assert(almost_equal(reference_igd_plus,
                      (std::sqrt(8.9) + std::sqrt(35.3)) / 2.0));

  // A candidate front equal to the reference front scores exactly zero.
  double normalized_igd_plus =
      modified_inverted_generational_distance(senses, reference_front,
                                              reference_front) /
      reference_igd_plus;

  assert(almost_equal(normalized_igd_plus, 0.0));

  // A strictly smaller candidate front scores strictly between zero and one.
  double partial_igd_plus =
      modified_inverted_generational_distance(senses, reference_front, {a}) /
      reference_igd_plus;

  assert(partial_igd_plus > 0.0);
  assert(partial_igd_plus < 1.0);
  assert(almost_equal(partial_igd_plus,
                      (std::sqrt(32.0) / 2.0) / reference_igd_plus));

  // Objectives 0 and 2 take negative values on daily returns. The padding is
  // additive on the attained range, so it still moves the reference point away
  // from the front, which a multiplicative perturbation would not.
  std::vector<double> negative_a = {-0.002, 1.0, -0.5, 1.0};
  std::vector<double> negative_b = {0.001, 3.0, 0.25, 3.0};
  std::vector<std::vector<double>> negative_front = {negative_a, negative_b};
  std::vector<double> negative_reference_point =
      reference_point_of(senses, negative_front);

  assert(almost_equal(negative_reference_point[0], -0.00215));
  assert(almost_equal(negative_reference_point[1], 3.1));
  assert(almost_equal(negative_reference_point[2], -0.5375));
  assert(almost_equal(negative_reference_point[3], 3.1));

  for (const std::vector<double>& point : negative_front) {
    for (unsigned i = 0; i < senses.size(); i++) {
      if (senses[i] == NSBRKGA::Sense::MINIMIZE) {
        assert(point[i] < negative_reference_point[i]);
      } else {
        assert(point[i] > negative_reference_point[i]);
      }
    }
  }

  // pagmo accepts the negated front against the negated reference point, which
  // is what breaks when the untransformed reference point is passed through.
  assert(compute_hypervolume(senses, negative_reference_point, negative_front) >
         0.0);

  // An objective that took a single value across every front still gets a
  // padding, and so does one that took the single value zero.
  std::vector<double> constant = {0.0, 5.0, 0.0, 5.0};
  std::vector<double> constant_reference_point =
      reference_point_of(senses, {constant, constant});

  assert(almost_equal(constant_reference_point[0], -0.05));
  assert(almost_equal(constant_reference_point[1], 5.25));
  assert(almost_equal(constant_reference_point[2], -0.05));
  assert(almost_equal(constant_reference_point[3], 5.25));

  std::cout << "Metrics test passed." << std::endl;

  return 0;
}
