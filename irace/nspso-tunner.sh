#!/bin/bash
# This machine has LC_NUMERIC=pt_BR.UTF-8, under which awk parses "0.001511" as
# 0. The elapsed time and the hypervolume negation both go through awk, so the
# whole locale is forced to C.
export LC_ALL=C
export LC_NUMERIC=C
###############################################################################
# NSPSO Target Runner for iRace
#
# Usage: ./nspso-tunner.sh <config_id> <instance_id> <seed> <instance> [params...]
# Output: cost time
#
# The instance argument is a directory (e.g. ../instances/ibov_2015) holding
# train/expected_returns.csv, train/covariance_matrix.csv and the frozen
# reference_point.txt written by scripts/build_pilot_reference_points.sh. The
# reference point must never be recomputed here: costs across candidate
# configurations are only comparable while it stays fixed.
#
# The cost is the negated raw hypervolume, since iRace minimises.
###############################################################################

CONFIG_ID="$1"
INSTANCE_ID="$2"
SEED="$3"
INSTANCE="$4"
shift 4
PARAMS=("$@")

# Project directory (one level up from irace/).
PROJECT_DIR="$(cd "$(dirname "$0")/.." && pwd)"

# Executables.
SOLVER="${PROJECT_DIR}/bin/exec/nspso_solver_exec"
HV_CALC="${PROJECT_DIR}/bin/exec/hypervolume_calculator_exec"

# Time limit per run (seconds). Overridable so that a budget smoke test can run
# many evaluations quickly; the tuning runs use the default.
TIME_LIMIT="${MOPOP_IRACE_TIME_LIMIT:-60}"

# Archive size, matching the MOPOP_MAX_NUM_SOLUTIONS default of run.sh.
MAX_NUM_SOLUTIONS="${MOPOP_IRACE_MAX_NUM_SOLUTIONS:-500}"

# Resolve the instance directory to an absolute path: everything else here is
# absolute, and iRace invokes this script from execDir rather than from the
# directory the paths in the scenario file are relative to.
if [ ! -d "$INSTANCE" ]; then
    echo "Inf 0"
    exit 0
fi

INSTANCE="$(cd "$INSTANCE" && pwd)"
EXPECTED_RETURNS="${INSTANCE}/train/expected_returns.csv"
COVARIANCE="${INSTANCE}/train/covariance_matrix.csv"
REFERENCE_POINT="${INSTANCE}/reference_point.txt"

# Create temporary directory for this run.
TMPDIR=$(mktemp -d)
trap 'rm -rf "$TMPDIR"' EXIT

# Output files.
PARETO_FILE="${TMPDIR}/pareto.txt"
HV_FILE="${TMPDIR}/hv.txt"

# Transform parameters (population_size_factor -> population_size). Unlike
# NSGA-II, pagmo's NSPSO imposes no multiple-of-four requirement; the factor is
# kept so the tuned range stays 100..500, matching the other pagmo solvers.
TRANSFORMED_PARAMS=()
i=0
while [ $i -lt ${#PARAMS[@]} ]; do
    if [ "${PARAMS[$i]}" = "--population-size-factor" ]; then
        FACTOR="${PARAMS[$((i + 1))]}"
        POPULATION_SIZE=$((FACTOR * 4))
        TRANSFORMED_PARAMS+=("--population-size" "$POPULATION_SIZE")
        i=$((i + 2))
    elif [ "${PARAMS[$i]}" = "--diversity-mechanism" ]; then
        # iRace emits categorical values unquoted, so pagmo's spaced strings
        # would word-split into two argv entries. nspso-parameters.txt uses
        # single tokens instead; map them back to what pagmo accepts. This
        # cannot call fail(): that helper is defined further down, after
        # $ELAPSED exists, so here it would print a single field.
        case "${PARAMS[$((i + 1))]}" in
            crowding_distance) MECHANISM="crowding distance" ;;
            niche_count) MECHANISM="niche count" ;;
            max_min) MECHANISM="max min" ;;
            *) echo "Inf 0"; exit 0 ;;
        esac
        TRANSFORMED_PARAMS+=("--diversity-mechanism" "$MECHANISM")
        i=$((i + 2))
    else
        TRANSFORMED_PARAMS+=("${PARAMS[$i]}")
        i=$((i + 1))
    fi
done

# Run solver.
#
# The brace group with its own stderr redirect is load-bearing. An uncaught C++
# exception kills the exec with SIGABRT, and bash then announces "Aborted (core
# dumped)" on the stderr of the shell that ran it -- the redirect above only
# covers the child, not that announcement. iRace merges our stdout and stderr
# and halts the whole race with "output of targetRunner should not be more than
# two numbers", so the announcement has to be swallowed here. A subshell does
# not work: bash exec-optimises it, leaving the main shell to report the death.
# The brace group preserves $? (134), so the exit-status check below still sees
# the failure.
START_TIME=$(date +%s.%N)

{ "$SOLVER" \
    --expected-returns-filename "$EXPECTED_RETURNS" \
    --covariance-filename "$COVARIANCE" \
    --seed "$SEED" \
    --time-limit "$TIME_LIMIT" \
    --max-num-solutions "$MAX_NUM_SOLUTIONS" \
    --memory \
    --pareto "$PARETO_FILE" \
    "${TRANSFORMED_PARAMS[@]}" > /dev/null 2>&1; } 2>/dev/null

SOLVER_EXIT=$?

END_TIME=$(date +%s.%N)
ELAPSED=$(awk -v a="$START_TIME" -v b="$END_TIME" 'BEGIN { printf "%.0f", b - a }')

# Report a failed evaluation. iRace treats Inf as the worst possible cost.
fail () {
    echo "Inf $ELAPSED"
    exit 0
}

# An exec handed a bad flag prints its usage string and exits 0, and the solver
# writes an empty --pareto file when its archive is empty, so the exit status
# alone is not enough.
[ $SOLVER_EXIT -eq 0 ] || fail
[ -s "$PARETO_FILE" ] || fail
[ -r "$REFERENCE_POINT" ] || fail

# Calculate hypervolume. Braced for the same reason as the solver call above --
# this is the one that actually fires in practice, since pagmo aborts whenever a
# front point fails to dominate the frozen reference point.
{ "$HV_CALC" \
    --expected-returns-filename "$EXPECTED_RETURNS" \
    --covariance-filename "$COVARIANCE" \
    --reference-point "$REFERENCE_POINT" \
    --pareto-0 "$PARETO_FILE" \
    --hypervolume-0 "$HV_FILE" > /dev/null 2>&1; } 2>/dev/null

HV_EXIT=$?

# hypervolume_calculator_exec opens the output file before computing, and pagmo
# throws when a front point fails to dominate the reference point. A failure
# therefore leaves the file present but empty, so existence is not enough.
[ $HV_EXIT -eq 0 ] || fail
[ -s "$HV_FILE" ] || fail

# Read hypervolume and negate (iRace minimises, we want to maximise HV).
HV=$(tr -d '[:space:]' < "$HV_FILE")
COST=$(awk -v hv="$HV" 'BEGIN {
    if (hv !~ /^-?[0-9]+([.][0-9]+)?([eE][-+]?[0-9]+)?$/) { exit 1 }
    printf "%.17g", -hv
}') || fail

echo "$COST $ELAPSED"
