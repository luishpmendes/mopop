# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

`mopop` is a research codebase for the **Multi-Objective Portfolio Optimization Problem (MOPOP)**. It benchmarks six multi-objective metaheuristics on the same problem, computes quality indicators (hypervolume, IGD+), and produces plots/videos for a doctoral thesis. C++17 core + Python data acquisition and plotting.

The six solvers: `nsga2`, `nspso`, `moead` (MOEA/D-DE), `mhaco`, `ihs` (all via pagmo) and `nsbrkga` (via the external NS-BRKGA header library).

## Build

```bash
make all                 # builds+runs all tests, then builds all executables
make execs               # executables only
make tests               # all tests (building a test target also RUNS it)
make nsbrkga_solver_test # build+run one test
make clean               # removes bin/
```

Build gotchas — these bite often:

- **`make` with no target runs `clean`** (it is the first rule in the Makefile). Always name a target.
- **No header dependencies are tracked.** `$(BIN)/%.o: $(SRC)/%.cpp` ignores `.hpp` files, so editing a header will not trigger a rebuild. After header edits, `make clean` first.
- **Test targets are file targets.** Once `bin/test/foo_test` exists, re-running `make foo_test` is a no-op. Delete the binary (or `make clean`) to re-run, or invoke `bin/test/foo_test` directly.
- **Tests use relative input paths** (`input/expected_returns_test.csv`), so they must be run from the repository root.
- **The Makefile hardcodes absolute include paths** — NS-BRKGA at `/home/luishpmendes/UNICAMP/Doutorado/nsbrkga/nsbrkga`, Boost at `/opt/boost`, pagmo at `/opt/pagmo`. On another machine, edit `BRKGAINC`/`BOOSTINC`/`PAGMOINC`.
- `bin/`, `venv/`, `__pycache__/` are gitignored; there is no `requirements.txt` yet (the venv has yfinance, pandas, numpy, matplotlib, seaborn, scipy).

Tests are plain `assert()` programs with `main()` — no test framework. A failing assert aborts the make run.

## Problem model (four objectives, mixed senses)

An `Instance` is loaded from **two CSVs**: expected returns (`Ticker,<value>` rows) and a covariance matrix (`Ticker,<row values>`), with headers. `Instance::senses` is fixed at:

| index | objective | sense |
|---|---|---|
| 0 | portfolio expected return `wᵀμ` | MAXIMIZE |
| 1 | portfolio variance `wᵀΣw` | MINIMIZE |
| 2 | ratio `value[0] / sqrt(value[1])` | MAXIMIZE |
| 3 | Shannon entropy of the weights (base 2) | MINIMIZE |

A decision vector is a raw non-negative vector normalized to sum to 1 (`Solution` constructor and `Decoder::decode` both do this; keep them in sync — the objective formulas are duplicated in `src/solution/solution.cpp` and `src/solver/nsbrkga/decoder.cpp`).

**The objective count 4 is still hardcoded on the solver side** (`get_nobj()`, `is_valid()`, `Decoder`'s value buffers, `Solution`'s `value(4, 0.0)`). The metric executables were converted to `instance.senses.size()` in Phase 3; the solver side needs a coordinated pass over all six solvers and is not done.

## Architecture

```
Instance  →  Solution  →  Solver (abstract)  →  concrete solver  →  *_exec (CLI)
```

- `src/instance/` — data loading, senses, validation.
- `src/solution/` — weight normalization, objective evaluation, dominance.
- `src/solver/solver.hpp` — the base class holding everything shared: seed/RNG, time and iteration limits, `max_num_solutions`, the archive of `best_individuals`/`best_solutions`, and the **snapshot machinery** (`best_solutions_snapshots`, `num_non_dominated_snapshots`, `num_fronts_snapshots`, `populations_snapshots`; NS-BRKGA adds `num_elites_snapshots`). Snapshots are taken on a geometrically increasing schedule driven by `time_snapshot_factor`/`iteration_snapshot_factor`. Subclasses implement `solve()`.
- **Two adapter styles.** The five pagmo solvers each have a `problem.cpp`/`problem.hpp` implementing `pagmo::problem` (`fitness`, `get_bounds`, `get_nobj`) plus a `*_solver.cpp` that drives the pagmo algorithm generation by generation, calling `update_best_individuals(pop)` and `capture_snapshot(pop)`. NS-BRKGA instead uses `decoder.cpp` implementing the BRKGA decoder interface.
- `src/exec/` — one `main()` per binary. All argument handling goes through the minimal `Argument_Parser` (`--flag value` lookups, no validation); when required flags are missing, each exec prints its own usage string to stderr and exits 0. Solver execs share a common flag set (`--expected-returns-filename`, `--covariance-filename`, `--seed`, `--time-limit`, `--iterations-limit`, `--max-num-solutions`, `--max-num-snapshots`, plus output flags `--statistics`, `--solutions`, `--pareto`, `--*-snapshots`) and add algorithm-specific tuning flags. **The usage string at the bottom of each `*_exec.cpp` is the authoritative flag list** for that solver.
- Non-solver execs: `reference_pareto_front_and_point_calculator_exec` (pools all runs' fronts into a reference front, plus a reference point set to each objective's worst attained value padded outward by 5% of its attained range), `hypervolume_ratio_calculator_exec` (`--hvr-*`), `normalized_modified_generational_distance_calculator_exec` (`--nigd-plus-*`), `hypervolume_calculator_exec` (raw hypervolume against a reference point alone, no reference front — this is the irace cost), `results_aggregator_exec` (selects the *best* and *median* run per solver by HVR rank and copies their artifacts to `<solver>_best.*` / `<solver>_median.*`).

Metric execs handle mixed senses by **negating maximization objectives** before handing the front to pagmo, which assumes minimization.

## Experiment pipeline

`run.sh` is the whole pipeline, in order: run every (instance × solver × seed) across 6 parallel process groups → build the reference front/point **per instance** → HVR → NIGD+ → aggregate per instance per solver → run every `plotter_*.py` → stitch snapshot PNGs into MP4s with ffmpeg. Output directories (`statistics/`, `pareto/`, `hvr/`, `hvr_snapshots/`, `nigd_plus/`, `nigd_plus_snapshots/`, `*_snapshots/`, `metrics/`, `metrics_snapshots/`) are created by the script and are gitignored. Every artifact is named `{instance}_{solver}_{seed}`; the reference front and point are `pareto/{instance}.txt` and `pareto/{instance}_point.txt`. Unlike `motsp_irace`, the HVR and NIGD+ execs need `--reference-point` in addition to `--reference-pareto`.

**Scale comes from the environment, not from editing the script.** `run.sh` defaults to 10 instances × 10 seeds × 3600 s and honours `MOPOP_INSTANCES`, `MOPOP_SEEDS`, `MOPOP_TIME_LIMIT`, `MOPOP_MAX_NUM_SOLUTIONS`, `MOPOP_MAX_NUM_SNAPSHOTS`, `MOPOP_MAX_REF_SOLUTIONS`, `MOPOP_NUM_PROCESSES`. `run_smoke.sh` is a thin wrapper that overrides them (1 instance, 2 seeds, 5 s) and `exec`s `run.sh` — use it to validate any pipeline change. The first three are exported so `plotter_definitions.py` reads the same values; that is what keeps the plot stage in step with a reduced run. Under `set -euo pipefail`, worker, plotter and ffmpeg failures are reported and stepped over rather than aborting the run — check `log_0.txt`…`log_5.txt` for `WARNING:` lines and for exec usage strings, since an exec given a bad flag prints usage and exits 0.

Python plotters read `plotter_definitions.py` for the shared instance/solver/seed lists, labels, colors, and `num_snapshots`/`m`, and `plotter_utils.py` for guarded file reads (`read_rows`, `read_column`, `load_snapshots`) plus total statistics helpers (`mean`, `stdev`, `quartiles`) that return NaN instead of raising on empty samples. **All reads must go through `plotter_utils`** — one missing file out of 600 must not abort the plot stage; misses are counted and printed by `report_missing`. `plotter_counts.py` holds the body shared by the three `num_*_snapshots` plotters. Delimiters differ by file family: `hvr/` and `nigd_plus/` are comma-separated, `pareto/` and the `*_snapshots/` families are space-separated, and `best_solutions_snapshots/`/`populations_snapshots/` carry a header line (`skip_header=True`).

## Instance generation

`scripts/generate_instances.py` builds the ten `ibov_*` instances. It replaced `downloader.py`, which was deleted. Two subcommands, and the split is load-bearing:

- `fetch` — the **only** code path that touches the network. Downloads every ticker in `ibovespa_tickers_2011_2025/` over one span (`2010-12-01`–`2026-01-01`) into `cache/prices/{TICKER}.csv` plus `cache/manifest.json`.
- `build` — reads the cache and nothing else; a missing ticker is an error, never an implicit download. `--allow-partial` is required in practice (see below).

`cache/prices/` **is committed and must stay that way.** Yahoo re-adjusts its `auto_adjust` close series retroactively on every new corporate action, so a re-fetch returns different numbers; the cache is the only thing pinning the instances. `instances/` is gitignored — regenerate it offline with `build`.

Statistics are **daily** (mean and covariance of daily returns), never annualized. This was BUG 4 in the plan.

Manifest entries carry a `status` of `ok`, `no_data`, or `error`. `build` refuses to run while anything is `error`, since treating an unknown history as an empty one silently shrinks an instance without any visible failure. `fetch` also refuses to overwrite a non-empty cached series with an empty response. Use `fetch --mark-no-data <TICKER>…` to settle long-dead symbols that fail deterministically inside yfinance instead of returning empty.

**Instances are 28–47 assets, not the 58–73 constituents.** Yahoo serves nothing for delisted, renamed, or merged symbols, so 65 of the 165 unique tickers are unavailable and each instance loses 20–32 names. Every asset present is therefore a survivor, and expected returns are biased upward; each `metadata.json` records this and the per-ticker drop causes. The ≥95%-plus-endpoints coverage rule accounts for at most 2 drops per instance — the rest is upstream availability.

## irace tuning

`irace/` holds one `{solver}-parameters.txt`, `{solver}-scenario.txt` and
`{solver}-tunner.sh` per solver (NS-BRKGA gets six staged sets). **NSGA-II and NSPSO
exist; moead, mhaco, ihs and the NS-BRKGA stages are open.** The race trains on
`train-instances.txt` (`ibov_2011`–`ibov_2017`) and touches
`test-instances.txt` (`ibov_2018`–`ibov_2020`) only in the post-race testing phase.
Cost is the *negated* raw hypervolume — `hypervolume_calculator_exec` against the
frozen `instances/{instance}/reference_point.txt`, which must never be recomputed:
costs are comparable across configurations only while it stays fixed.

Four things bite when writing a new runner, all learned the hard way:

- **Wrap every exec call in `{ ... > /dev/null 2>&1; } 2>/dev/null`.** An uncaught C++
  exception kills an exec with SIGABRT and bash announces `Aborted (core dumped)` on
  the *calling shell's* stderr, which the inner redirect does not cover. irace merges
  the runner's stdout and stderr, so that one line aborts the entire race with
  `output of targetRunner should not be more than two numbers`. This fires routinely,
  because pagmo throws whenever a front point fails to dominate the reference point.
  A subshell does **not** work — bash exec-optimises it and the main shell reports the
  death anyway. The brace group preserves `$?`, so exit-status checks still work.
- **Parameter bounds are dictated by pagmo's constructor validation**, which throws
  `std::invalid_argument` on violation; the execs do not catch it, so the run aborts
  and the evaluation is wasted. With `digits = 2` a bound of `0.00` is reachable, so
  any parameter pagmo requires to be `> 0` must start at `0.01`. Read the real
  constraints out of the library rather than guessing:
  `strings /opt/pagmo/lib/libpagmo.so.9 | grep -i <param>`.
- **Categorical values must be single tokens.** irace emits them unquoted, so pagmo's
  `"crowding distance"` would word-split into two `argv` entries. The parameter files
  use `crowding_distance`/`niche_count`/`max_min` and the runner maps them back.
- **Presence-only flags cannot be tuned as `c(0,1)`.** `--memory` (NSPSO, MHACO) and
  `--preserve-diversity` (MOEA/D-DE) are read with `arg_parser.option_exists(...)`,
  which is true for `--memory 0` as well, so such a parameter would silently always
  mean *on*. They are hardcoded in the runner's invocation to match `run.sh`.

Test artefacts with `[ -s ]`, never `[ -f ]`: an exec given a bad flag prints usage and
exits 0, and `hypervolume_calculator_exec` opens its output file before computing.
Validation, from `irace/` with
`IRACE=~/R/x86_64-pc-linux-gnu-library/4.1/irace/bin/irace`:

```bash
MOPOP_IRACE_TIME_LIMIT=5 ./nspso-tunner.sh 1 1 12345 ../instances/ibov_2011 ...  # want: "cost elapsed"
$IRACE --check --scenario nspso-scenario.txt
# Budget smoke. The minimum is (minSurvival+1) * nbIterations * (mu + eachTest +
# (nbIterations-1)*max(eachTest,Tnew)), with nbIterations = minSurvival =
# floor(2 + log2 nParams): 180 for 5 parameters, 300 for 8. --max-time 0 is
# required, since irace rejects two positive budgets.
MOPOP_IRACE_TIME_LIMIT=3 $IRACE --scenario nspso-scenario.txt \
  --max-time 0 --max-experiments 300 --log-file ./smoke-nspso.Rdata
```

## In-flight work

`docs/implementation_plan.md` is the active plan: migrate from the single hardcoded instance to ten rolling IBOVESPA windows (`ibov_2011`–`ibov_2020`, tickers already committed under `ibovespa_tickers_2011_2025/`), 10 seeds, irace tuning, and multi-instance aggregation. **Instance generation (old Phase 2), metric bug fixes (old Phase 3), and the BUG 5 initialization fix (Phase 4A) are done.** `scripts/generate_instances.py` and `scripts/validate_instances.py` exist, all ten instances build and validate. The three metric-exec bugs are fixed, execs/flags renamed to `hvr`/`nigd_plus`, raw HV exec for irace exists. `src/test/metrics_test.cpp` pins formulas to analytic values — it duplicates the helpers from the exec files, so keep the two in sync.

**Next steps (in order):** Phase 4B (multi-instance `run.sh`, directory renames) → Phase 5 (irace tuning) → Phase 6 (aggregation & plots) → Phase 7 (tests & documentation). **Read the plan before touching the metric executables, solvers, or `run.sh`.**

**Initial population seeding lives in one place.** `Solver::build_initial_chromosomes(max_num_chromosomes)` (`src/solver/solver.cpp`) builds the deterministic seed chromosomes — single-asset, leave-one-out, uniform, and expected-return-weighted prefixes — and **never returns more than `max_num_chromosomes`**. All six solvers call it with `population_size`. Do not reintroduce per-solver seeding: the pagmo solvers size their random population as `population_size - initial_chromosomes.size()` in unsigned arithmetic, and NS-BRKGA's `setInitialPopulations` throws outright when a population holds more chromosomes than `population_size` — the cap is what makes both safe when `population_size` is small (irace tunes it as `factor × 4`, well below the ~122 seeds a 47-asset instance yields).

**A chromosome whose entries sum to zero decodes to the uniform portfolio,** not to an all-zero weight vector — in both `Solution`'s key constructor and `Decoder::decode`. An empty portfolio has zero variance and zero entropy, the global minimum of both MINIMIZE objectives, so it can never be dominated and permanently occupies an archive slot. NS-BRKGA produced one on every instance before this fallback existed.

Regression coverage: `src/test/solver_invariants.hpp` holds the shared assertions (finite objectives, entropy ≥ 0, variance ≤ largest single-asset variance, weights summing to 1, `is_feasible()`); every solver test applies them to the legacy `input/` fixture, to `input/*_bug5_test.csv` (a 10-asset adversarial submatrix of `ibov_2020` whose first asset has the most negative expected return), and to `instances/ibov_2020/train/` when it has been built.

> [!IMPORTANT]
> This machine has `LC_NUMERIC=pt_BR.UTF-8`, under which `awk` and `sort -g` parse `0.001511` as `0`. **Force `LC_ALL=C` for any numeric checking of solver output**, or use Python, whose `float()` is locale-independent. An earlier verification pass produced silently vacuous results this way.

The sibling project `/home/luishpmendes/UNICAMP/Doutorado/motsp_irace` (an additional working directory) is the reference implementation for the target structure — instance directories, irace scenarios/target runners, plotter naming, and `results_aggregator.py`. Prefer mirroring its patterns over inventing new ones.

## Conventions

- C++ formatted with `.clang-format` (Google style, 2-space indent, 80 columns). Everything lives in `namespace mopop` except `Argument_Parser`.
- Members are accessed via explicit `this->` throughout; public data members are the norm (solver parameters are set directly by the exec layer, not via setters).
- Doxygen `@brief`/`@param`/`@return` comments on all declarations in headers, repeated above the definition in the `.cpp`.
- Python is 2-space indented (autopep8 `--indent-size=2`, per `.vscode/settings.json`).
- Adding a solver or exec means adding an explicit link rule plus a phony alias in the Makefile and appending it to `tests`/`execs`; there is no wildcard target.
