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

`run.sh` is the whole pipeline, single instance, in order: run every (solver × seed) across 6 parallel process groups → build the reference front/point → hypervolume → IGD+ → aggregate per solver → run every `plotter_*.py` → stitch snapshot PNGs into MP4s with ffmpeg. Output directories (`statistics/`, `pareto/`, `hypervolume/`, `igd_plus/`, `*_snapshots/`, `metrics/`) are created by the script and are not committed.

Python plotters read `plotter_definitions.py` for the shared solver list, labels, seeds, colors, and `num_snapshots`/`m`. Any change to the solver set or seed list must be mirrored there.

Two known inconsistencies in the current scripts: `run.sh` ends by invoking `metrics_stats.py`, which does not exist in this repo (it exists in the `motsp_irace` sibling project); and `plotter_definitions.py` names `input/*.txt` while the actual inputs are `input/*.csv`.

`downloader.py` fetches adjusted closes from Yahoo Finance and writes `returns`, `expected_returns`, and `covariance_matrix` CSVs. Despite the docstrings and log messages saying "annualized", it computes **daily** mean and covariance — this is intentional; fix the wording, not the math.

## In-flight work

`docs/implementation_plan.md` is the active plan: migrate from the single hardcoded instance to ten rolling IBOVESPA windows (`ibov_2011`–`ibov_2020`, tickers already committed under `ibovespa_tickers_2011_2025/`), 10 seeds, irace tuning, and multi-instance aggregation. **Phase 3 is done**: the three metric-exec bugs are fixed (untransformed reference point passed to pagmo; `max` used for maximization objectives, which also truncated IGD+ to 2 of 4 objectives; the sign-unsafe 5% front perturbation, replaced by a 5%-of-range padding on the reference *point*), the execs and flags are renamed to `hvr`/`nigd_plus`, and the raw HV exec for irace exists. `src/test/metrics_test.cpp` pins the formulas to analytic values — it duplicates the helpers from the exec files, so keep the two in sync. Still open: the "annualized" docstrings in `downloader.py` (Phase 2) and the `hypervolume/`→`hvr/` directory renames (Phase 4). **Read the plan before touching the metric executables or `run.sh`.**

The sibling project `/home/luishpmendes/UNICAMP/Doutorado/motsp_irace` (an additional working directory) is the reference implementation for the target structure — instance directories, irace scenarios/target runners, plotter naming, and `results_aggregator.py`. Prefer mirroring its patterns over inventing new ones.

## Conventions

- C++ formatted with `.clang-format` (Google style, 2-space indent, 80 columns). Everything lives in `namespace mopop` except `Argument_Parser`.
- Members are accessed via explicit `this->` throughout; public data members are the norm (solver parameters are set directly by the exec layer, not via setters).
- Doxygen `@brief`/`@param`/`@return` comments on all declarations in headers, repeated above the definition in the `.cpp`.
- Python is 2-space indented (autopep8 `--indent-size=2`, per `.vscode/settings.json`).
- Adding a solver or exec means adding an explicit link rule plus a phony alias in the Makefile and appending it to `tests`/`execs`; there is no wildcard target.
