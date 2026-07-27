#!/bin/bash

# Full MOPOP experiment pipeline over the ten rolling IBOVESPA windows.
#
# Scale is configured through the environment so that a reduced profile
# (see run_smoke.sh) drives the very same script. The plotters read the same
# MOPOP_* variables through plotter_definitions.py, so the plot stage always
# matches the run that produced the data.

set -euo pipefail

: "${MOPOP_INSTANCES:=ibov_2011 ibov_2012 ibov_2013 ibov_2014 ibov_2015 ibov_2016 ibov_2017 ibov_2018 ibov_2019 ibov_2020}"
: "${MOPOP_SEEDS:=305089489 511812191 608055156 467424509 944441939 414977408 819312498 562386085 287613914 755772793}"
: "${MOPOP_TIME_LIMIT:=3600}"
: "${MOPOP_MAX_NUM_SOLUTIONS:=500}"
: "${MOPOP_MAX_NUM_SNAPSHOTS:=30}"
: "${MOPOP_MAX_REF_SOLUTIONS:=800}"
: "${MOPOP_NUM_PROCESSES:=6}"

# Consumed by plotter_definitions.py.
export MOPOP_INSTANCES MOPOP_SEEDS MOPOP_MAX_NUM_SNAPSHOTS

read -r -a instances <<< "${MOPOP_INSTANCES}"
read -r -a seeds <<< "${MOPOP_SEEDS}"
solvers=(nsga2 nspso moead mhaco ihs nsbrkga)
versions=(best median)

path=$(dirname "$(realpath "$0")")

commands=()
unit=0

# Start a fresh round of worker chains.
reset_commands () {
  commands=()
  local i
  for ((i = 0; i < MOPOP_NUM_PROCESSES; i++)); do
    commands[i]="("
  done
  unit=0
}

# Append one work unit to a worker chain, round-robin over the flat unit index.
# $1 = the fully built command string.
dispatch () {
  if [ "${unit}" -lt "${MOPOP_NUM_PROCESSES}" ]; then
    commands[unit]+="$1"
  else
    commands[unit % MOPOP_NUM_PROCESSES]+=" && $1"
  fi
  unit=$((unit + 1))
}

# Run every non-empty worker chain in parallel and wait for all of them.
# $1 = stage name (for the warning), $2 = ">" to truncate or ">>" to append logs.
launch () {
  local final="" i rc=0 num_workers=${MOPOP_NUM_PROCESSES}

  if [ "${unit}" -lt "${num_workers}" ]; then
    num_workers=${unit}
  fi

  if [ "${num_workers}" -eq 0 ]; then
    return 0
  fi

  for ((i = 0; i < num_workers; i++)); do
    final+="${commands[i]}) &${2} ${path}/log_${i}.txt & "
  done

  eval "${final}"

  wait || rc=$?

  if [ "${rc}" -ne 0 ]; then
    echo "WARNING: stage '$1' had failing workers; see ${path}/log_*.txt" >&2
  fi
}

mkdir -p "${path}/statistics"
mkdir -p "${path}/pareto"
mkdir -p "${path}/best_solutions_snapshots"
mkdir -p "${path}/num_non_dominated_snapshots"
mkdir -p "${path}/num_fronts_snapshots"
mkdir -p "${path}/populations_snapshots"
mkdir -p "${path}/num_elites_snapshots"
mkdir -p "${path}/hvr"
mkdir -p "${path}/hvr_snapshots"
mkdir -p "${path}/nigd_plus"
mkdir -p "${path}/nigd_plus_snapshots"
mkdir -p "${path}/metrics"
mkdir -p "${path}/metrics_snapshots"

################################################################################
# Stage 1 - solver runs, one work unit per (instance, solver, seed).
################################################################################

reset_commands

for instance in "${instances[@]}"; do
  expected_returns="${path}/instances/${instance}/train/expected_returns.csv"
  covariance="${path}/instances/${instance}/train/covariance_matrix.csv"
  for solver in "${solvers[@]}"; do
    for seed in "${seeds[@]}"; do
      command="${path}/bin/exec/${solver}_solver_exec "
      command+="--expected-returns-filename ${expected_returns} "
      command+="--covariance-filename ${covariance} "
      command+="--seed ${seed} "
      command+="--time-limit ${MOPOP_TIME_LIMIT} "
      command+="--max-num-solutions ${MOPOP_MAX_NUM_SOLUTIONS} "
      command+="--max-num-snapshots ${MOPOP_MAX_NUM_SNAPSHOTS} "
      command+="--statistics ${path}/statistics/${instance}_${solver}_${seed}.txt "
      command+="--pareto ${path}/pareto/${instance}_${solver}_${seed}.txt "
      command+="--best-solutions-snapshots ${path}/best_solutions_snapshots/${instance}_${solver}_${seed}_ "
      command+="--num-non-dominated-snapshots ${path}/num_non_dominated_snapshots/${instance}_${solver}_${seed}.txt "
      command+="--num-fronts-snapshots ${path}/num_fronts_snapshots/${instance}_${solver}_${seed}.txt "
      command+="--populations-snapshots ${path}/populations_snapshots/${instance}_${solver}_${seed}_ "
      if [ "${solver}" = "nspso" ]; then
        command+="--memory "
      fi
      if [ "${solver}" = "moead" ]; then
        command+="--preserve-diversity "
      fi
      if [ "${solver}" = "mhaco" ]; then
        command+="--memory "
      fi
      if [ "${solver}" = "nsbrkga" ]; then
        command+="--num-elites-snapshots ${path}/num_elites_snapshots/${instance}_${solver}_${seed}.txt "
      fi
      dispatch "${command}"
    done
  done
done

launch "solvers" ">"

################################################################################
# Stage 2 - reference Pareto front and reference point, one unit per instance.
################################################################################

reset_commands

for instance in "${instances[@]}"; do
  expected_returns="${path}/instances/${instance}/train/expected_returns.csv"
  covariance="${path}/instances/${instance}/train/covariance_matrix.csv"
  command="${path}/bin/exec/reference_pareto_front_and_point_calculator_exec "
  command+="--expected-returns-filename ${expected_returns} "
  command+="--covariance-filename ${covariance} "
  command+="--max-num-solutions ${MOPOP_MAX_REF_SOLUTIONS} "
  command+="--reference-pareto ${path}/pareto/${instance}.txt "
  command+="--reference-point ${path}/pareto/${instance}_point.txt "
  j=0
  for solver in "${solvers[@]}"; do
    for seed in "${seeds[@]}"; do
      command+="--pareto-${j} ${path}/pareto/${instance}_${solver}_${seed}.txt "
      command+="--best-solutions-snapshots-${j} ${path}/best_solutions_snapshots/${instance}_${solver}_${seed}_ "
      j=$((j + 1))
    done
  done
  dispatch "${command}"
done

launch "reference fronts" ">>"

################################################################################
# Stage 3 - hypervolume ratio, one unit per instance.
################################################################################

reset_commands

for instance in "${instances[@]}"; do
  expected_returns="${path}/instances/${instance}/train/expected_returns.csv"
  covariance="${path}/instances/${instance}/train/covariance_matrix.csv"
  command="${path}/bin/exec/hypervolume_ratio_calculator_exec "
  command+="--expected-returns-filename ${expected_returns} "
  command+="--covariance-filename ${covariance} "
  command+="--reference-pareto ${path}/pareto/${instance}.txt "
  command+="--reference-point ${path}/pareto/${instance}_point.txt "
  j=0
  for solver in "${solvers[@]}"; do
    for seed in "${seeds[@]}"; do
      command+="--pareto-${j} ${path}/pareto/${instance}_${solver}_${seed}.txt "
      command+="--best-solutions-snapshots-${j} ${path}/best_solutions_snapshots/${instance}_${solver}_${seed}_ "
      command+="--hvr-${j} ${path}/hvr/${instance}_${solver}_${seed}.txt "
      command+="--hvr-snapshots-${j} ${path}/hvr_snapshots/${instance}_${solver}_${seed}.txt "
      j=$((j + 1))
    done
  done
  dispatch "${command}"
done

launch "hypervolume ratio" ">>"

################################################################################
# Stage 4 - normalized modified inverted generational distance, per instance.
################################################################################

reset_commands

for instance in "${instances[@]}"; do
  expected_returns="${path}/instances/${instance}/train/expected_returns.csv"
  covariance="${path}/instances/${instance}/train/covariance_matrix.csv"
  command="${path}/bin/exec/normalized_modified_generational_distance_calculator_exec "
  command+="--expected-returns-filename ${expected_returns} "
  command+="--covariance-filename ${covariance} "
  command+="--reference-pareto ${path}/pareto/${instance}.txt "
  command+="--reference-point ${path}/pareto/${instance}_point.txt "
  j=0
  for solver in "${solvers[@]}"; do
    for seed in "${seeds[@]}"; do
      command+="--pareto-${j} ${path}/pareto/${instance}_${solver}_${seed}.txt "
      command+="--best-solutions-snapshots-${j} ${path}/best_solutions_snapshots/${instance}_${solver}_${seed}_ "
      command+="--nigd-plus-${j} ${path}/nigd_plus/${instance}_${solver}_${seed}.txt "
      command+="--nigd-plus-snapshots-${j} ${path}/nigd_plus_snapshots/${instance}_${solver}_${seed}.txt "
      j=$((j + 1))
    done
  done
  dispatch "${command}"
done

launch "nigd plus" ">>"

################################################################################
# Stage 5 - per (instance, solver) aggregation into best and median runs.
################################################################################

reset_commands

for instance in "${instances[@]}"; do
  for solver in "${solvers[@]}"; do
    command="${path}/bin/exec/results_aggregator_exec "
    command+="--hvrs ${path}/hvr/${instance}_${solver}.txt "
    command+="--hvr-statistics ${path}/hvr/${instance}_${solver}_stats.txt "
    command+="--nigd-pluses ${path}/nigd_plus/${instance}_${solver}.txt "
    command+="--nigd-plus-statistics ${path}/nigd_plus/${instance}_${solver}_stats.txt "
    command+="--statistics-best ${path}/statistics/${instance}_${solver}_best.txt "
    command+="--statistics-median ${path}/statistics/${instance}_${solver}_median.txt "
    command+="--pareto-best ${path}/pareto/${instance}_${solver}_best.txt "
    command+="--pareto-median ${path}/pareto/${instance}_${solver}_median.txt "
    command+="--hvr-snapshots-best ${path}/hvr_snapshots/${instance}_${solver}_best.txt "
    command+="--hvr-snapshots-median ${path}/hvr_snapshots/${instance}_${solver}_median.txt "
    command+="--best-solutions-snapshots-best ${path}/best_solutions_snapshots/${instance}_${solver}_best_ "
    command+="--best-solutions-snapshots-median ${path}/best_solutions_snapshots/${instance}_${solver}_median_ "
    command+="--num-non-dominated-snapshots-best ${path}/num_non_dominated_snapshots/${instance}_${solver}_best.txt "
    command+="--num-non-dominated-snapshots-median ${path}/num_non_dominated_snapshots/${instance}_${solver}_median.txt "
    command+="--populations-snapshots-best ${path}/populations_snapshots/${instance}_${solver}_best_ "
    command+="--populations-snapshots-median ${path}/populations_snapshots/${instance}_${solver}_median_ "
    command+="--num-fronts-snapshots-best ${path}/num_fronts_snapshots/${instance}_${solver}_best.txt "
    command+="--num-fronts-snapshots-median ${path}/num_fronts_snapshots/${instance}_${solver}_median.txt "
    if [ "${solver}" = "nsbrkga" ]; then
      command+="--num-elites-snapshots-best ${path}/num_elites_snapshots/${instance}_${solver}_best.txt "
      command+="--num-elites-snapshots-median ${path}/num_elites_snapshots/${instance}_${solver}_median.txt "
    fi
    j=0
    for seed in "${seeds[@]}"; do
      command+="--statistics-${j} ${path}/statistics/${instance}_${solver}_${seed}.txt "
      command+="--pareto-${j} ${path}/pareto/${instance}_${solver}_${seed}.txt "
      command+="--hvr-${j} ${path}/hvr/${instance}_${solver}_${seed}.txt "
      command+="--hvr-snapshots-${j} ${path}/hvr_snapshots/${instance}_${solver}_${seed}.txt "
      command+="--nigd-plus-${j} ${path}/nigd_plus/${instance}_${solver}_${seed}.txt "
      command+="--nigd-plus-snapshots-${j} ${path}/nigd_plus_snapshots/${instance}_${solver}_${seed}.txt "
      command+="--best-solutions-snapshots-${j} ${path}/best_solutions_snapshots/${instance}_${solver}_${seed}_ "
      command+="--num-non-dominated-snapshots-${j} ${path}/num_non_dominated_snapshots/${instance}_${solver}_${seed}.txt "
      command+="--populations-snapshots-${j} ${path}/populations_snapshots/${instance}_${solver}_${seed}_ "
      command+="--num-fronts-snapshots-${j} ${path}/num_fronts_snapshots/${instance}_${solver}_${seed}.txt "
      if [ "${solver}" = "nsbrkga" ]; then
        command+="--num-elites-snapshots-${j} ${path}/num_elites_snapshots/${instance}_${solver}_${seed}.txt "
      fi
      j=$((j + 1))
    done
    dispatch "${command}"
  done
done

launch "results aggregation" ">>"

################################################################################
# Stage 6 - plots.
################################################################################

plotters=(hvr hvr_snapshots nigd_plus nigd_plus_snapshots metrics
          metrics_snapshots num_non_dominated_snapshots num_fronts_snapshots
          num_elites_snapshots pareto best_solutions_snapshots
          populations_snapshots)

for plotter in "${plotters[@]}"; do
  { python3 "${path}/plotter_${plotter}.py" ||
      echo "WARNING: plotter_${plotter}.py failed" >&2; } &
done

wait

################################################################################
# Stage 7 - videos.
################################################################################

# Stitch the PNG frames named "<prefix>_<n>.png" into a video, then drop the
# frames. $1 = frame prefix, $2 = output .mp4 (defaults to "<prefix>.mp4").
# A missing frame series is reported, never fatal.
animate () {
  local prefix="$1" output="${2:-$1.mp4}"
  if [ ! -f "${prefix}_0.png" ]; then
    echo "WARNING: no frames for $(basename "${prefix}"); skipping video" >&2
    return 0
  fi
  if ffmpeg -y -r 5 -i "${prefix}_%d.png" -c:v libx264 -vf fps=60 \
      -pix_fmt yuv420p "${output}" </dev/null; then
    rm -f "${prefix}_"*.png
  else
    echo "WARNING: ffmpeg failed for $(basename "${prefix}"); frames kept" >&2
  fi
}

for instance in "${instances[@]}"; do
  for version in "${versions[@]}"; do
    animate "${path}/best_solutions_snapshots/${instance}_${version}" &
    animate "${path}/populations_snapshots/${instance}_${version}"

    wait || true
  done
done

animate "${path}/hvr_snapshots/snapshot" "${path}/hvr_snapshots/hvr.mp4" &
animate "${path}/nigd_plus_snapshots/snapshot" "${path}/nigd_plus_snapshots/nigd_plus.mp4" &
animate "${path}/metrics_snapshots/raincloud" &
animate "${path}/metrics_snapshots/scatter"

wait || true
