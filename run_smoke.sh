#!/bin/bash

# Reduced profile of the full pipeline: one instance, two seeds, five seconds
# per run. Validates that every stage of run.sh wires together end to end
# without waiting for the ~100 hour production run.
#
# Two seeds is the floor - the plotters call statistics.stdev and
# statistics.quantiles, which need at least two samples.

set -euo pipefail

export MOPOP_INSTANCES="ibov_2015"
export MOPOP_SEEDS="305089489 511812191"
export MOPOP_TIME_LIMIT=5
export MOPOP_MAX_NUM_SOLUTIONS=100
export MOPOP_MAX_NUM_SNAPSHOTS=5
export MOPOP_MAX_REF_SOLUTIONS=200

exec "$(dirname "$(realpath "$0")")/run.sh"
