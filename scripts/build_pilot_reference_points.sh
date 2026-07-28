#!/bin/bash

# Builds the frozen reference point of every irace training instance.
#
# irace scores a candidate configuration by the raw hypervolume of its front
# against a reference point. That point has to be computed once, from a pool
# that no single candidate can influence, and then never touched again: a point
# recomputed per candidate makes costs incomparable across the race.
#
# The pool is every solver at two seeds, so that the point bounds what any of
# the six can attain and the five solvers tuned after NSGA-II score against the
# same geometry. reference_pareto_front_and_point_calculator_exec pads each
# objective outward by 5% of its attained range, which absorbs a later run
# slightly exceeding the pool.
#
# Scale is configured through the environment, following run.sh.

set -euo pipefail

: "${MOPOP_PILOT_INSTANCES:=ibov_2011 ibov_2012 ibov_2013 ibov_2014 ibov_2015 ibov_2016 ibov_2017}"
: "${MOPOP_PILOT_SEEDS:=305089489 511812191}"
: "${MOPOP_PILOT_TIME_LIMIT:=60}"
: "${MOPOP_PILOT_MAX_NUM_SOLUTIONS:=500}"
: "${MOPOP_PILOT_MAX_REF_SOLUTIONS:=800}"
: "${MOPOP_PILOT_JOBS:=8}"

read -r -a instances <<< "${MOPOP_PILOT_INSTANCES}"
read -r -a seeds <<< "${MOPOP_PILOT_SEEDS}"
solvers=(nsga2 nspso moead mhaco ihs nsbrkga)

path=$(dirname "$(dirname "$(realpath "$0")")")

force=0

usage () {
  cat >&2 <<EOF
./build_pilot_reference_points.sh [--force]

Writes instances/<instance>/reference_point.txt for every instance in
MOPOP_PILOT_INSTANCES, from a pool of every solver at every seed in
MOPOP_PILOT_SEEDS.

  --force   recompute reference points that already exist

Environment:
  MOPOP_PILOT_INSTANCES         default: ibov_2011 .. ibov_2017
  MOPOP_PILOT_SEEDS             default: 305089489 511812191
  MOPOP_PILOT_TIME_LIMIT        default: 60
  MOPOP_PILOT_MAX_NUM_SOLUTIONS default: 500
  MOPOP_PILOT_MAX_REF_SOLUTIONS default: 800
  MOPOP_PILOT_JOBS              default: 8
EOF
  exit 1
}

while [ $# -gt 0 ]; do
  case "$1" in
    --force) force=1; shift ;;
    *) usage ;;
  esac
done

work_dir=$(mktemp -d)
trap 'rm -rf "${work_dir}"' EXIT

################################################################################
# Refuse to clobber a frozen point unless asked. Overwriting one silently
# invalidates every tuning result already scored against it.
################################################################################

pending=()

for instance in "${instances[@]}"; do
  if [ ! -d "${path}/instances/${instance}/train" ]; then
    echo "ERROR: ${path}/instances/${instance}/train not found." >&2
    echo "Build the instances first: python3 scripts/generate_instances.py build" >&2
    exit 1
  fi

  if [ -s "${path}/instances/${instance}/reference_point.txt" ] && [ "${force}" -eq 0 ]; then
    echo "SKIP ${instance}: reference_point.txt already frozen (--force to recompute)."
  else
    pending+=("${instance}")
  fi
done

if [ "${#pending[@]}" -eq 0 ]; then
  echo "Nothing to do."
  exit 0
fi

################################################################################
# Stage 1 - the pilot pool: every solver, every seed, every pending instance.
################################################################################

echo "Pilot pool: ${#pending[@]} instance(s) x ${#solvers[@]} solvers x ${#seeds[@]} seeds" \
     "at ${MOPOP_PILOT_TIME_LIMIT}s, ${MOPOP_PILOT_JOBS} at a time."

job_list="${work_dir}/jobs.txt"
: > "${job_list}"

for instance in "${pending[@]}"; do
  expected_returns="${path}/instances/${instance}/train/expected_returns.csv"
  covariance="${path}/instances/${instance}/train/covariance_matrix.csv"

  for solver in "${solvers[@]}"; do
    for seed in "${seeds[@]}"; do
      printf '%s\n' "${path}/bin/exec/${solver}_solver_exec \
--expected-returns-filename ${expected_returns} \
--covariance-filename ${covariance} \
--seed ${seed} \
--time-limit ${MOPOP_PILOT_TIME_LIMIT} \
--max-num-solutions ${MOPOP_PILOT_MAX_NUM_SOLUTIONS} \
--pareto ${work_dir}/${instance}_${solver}_${seed}.txt" >> "${job_list}"
    done
  done
done

# A single failed run must not abort the pool: the reference point only needs
# the pooled extremes, and a missing front is reported rather than fatal.
set +e
xargs -a "${job_list}" -P "${MOPOP_PILOT_JOBS}" -I {} sh -c '{} > /dev/null 2>&1'
set -e

################################################################################
# Stage 2 - pool each instance's fronts into a reference front and point.
################################################################################

for instance in "${pending[@]}"; do
  expected_returns="${path}/instances/${instance}/train/expected_returns.csv"
  covariance="${path}/instances/${instance}/train/covariance_matrix.csv"
  command=("${path}/bin/exec/reference_pareto_front_and_point_calculator_exec"
           --expected-returns-filename "${expected_returns}"
           --covariance-filename "${covariance}"
           --max-num-solutions "${MOPOP_PILOT_MAX_REF_SOLUTIONS}"
           --reference-pareto "${work_dir}/${instance}_front.txt"
           --reference-point "${path}/instances/${instance}/reference_point.txt")

  j=0
  missing=0

  for solver in "${solvers[@]}"; do
    for seed in "${seeds[@]}"; do
      front="${work_dir}/${instance}_${solver}_${seed}.txt"

      if [ -s "${front}" ]; then
        command+=("--pareto-${j}" "${front}")
        j=$((j + 1))
      else
        echo "WARNING: ${instance} ${solver} ${seed} produced no front; excluded from the pool." >&2
        missing=$((missing + 1))
      fi
    done
  done

  if [ "${j}" -eq 0 ]; then
    echo "ERROR: ${instance} has no usable fronts; no reference point written." >&2
    continue
  fi

  "${command[@]}"

  num_values=$(wc -w < "${path}/instances/${instance}/reference_point.txt")
  echo "${instance}: reference point over ${j} front(s) (${missing} missing), ${num_values} objectives."
done

exit 0
