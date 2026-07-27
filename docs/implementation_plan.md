# Implementation Plan: Multi-Instance IBOVESPA Experiments for `mopop`

## Goal

Extend `mopop` from its current single-instance setup into a reproducible
10-instance experimentation pipeline (`ibov_2011`–`ibov_2020`), with correct
solvers, `irace` tuning, and multi-instance result analysis. Mirror the proven
patterns from `motsp_irace`.

---

## Decisions

| Decision | Value |
|---|---|
| Algorithms | NSGA-II, NSPSO, MOEA/D-DE, MHACO, IHS, NS-BRKGA |
| Seeds | 10: 305089489, 511812191, 608055156, 467424509, 944441939, 414977408, 819312498, 562386085, 287613914, 755772793 |
| Tuning time limit | 60 s per candidate |
| Final-run time limit | 3600 s |
| Instances | `ibov_2011`–`ibov_2020` (10 rolling windows) |
| Training window | 5 calendar years |
| OOS window | Following calendar year |
| Ticker source | `ibovespa_tickers_2011_2025/tickers_{year}.csv` |
| Statistics | Daily (no annualization) |
| irace train split | `ibov_2011`–`ibov_2017` (7 instances) |
| irace holdout split | `ibov_2018`–`ibov_2020` (3 instances) |
| Population size factor | `population_size = factor × 4` (same as `motsp_irace`) |

---

## Completed Work

### Phase 2 — Instance Generation ✅

`scripts/generate_instances.py` and `scripts/validate_instances.py` exist. All
ten instances (`instances/ibov_2011/` through `instances/ibov_2020/`) build from
committed cache and validate. Each instance has `train/` and `oos/`
subdirectories with `expected_returns.csv` and `covariance_matrix.csv`, plus
`metadata.json` and `tickers.csv`. Instances are 28–47 assets; 65 of 165 unique
tickers are unavailable at Yahoo Finance. A rebuild from cache inside an
isolated network namespace (`unshare -rn`) is byte-identical.

### Phase 3 — Metric Bug Fixes and Renaming ✅

Four bugs fixed, executables renamed:

| Bug | Fix |
|---|---|
| BUG 1: `hv.compute(reference_point)` | → `hv.compute(reference_point_prime)` |
| BUG 2: Reference point used `max` for all objectives | Tracks worst per sense |
| BUG 3: 5% front perturbation sign-unsafe | Removed; pads reference *point* 5% of attained range |
| BUG 4: Docstrings say "annualized" | Resolved by deleting `downloader.py` |

Three metric executables renamed: `hypervolume_ratio_calculator_exec` (HVR),
`normalized_modified_generational_distance_calculator_exec` (NIGD+),
`hypervolume_calculator_exec` (raw HV for irace). CLI flags renamed to
`--hvr-*` and `--nigd-plus-*`. Hardcoded `4` eliminated from metric execs.
`metrics_test.cpp` pins formulas to analytic values. `run.sh` flags updated.

### IBOVESPA Ticker Data ✅

`ibovespa_tickers_2011_2025/` contains `tickers_{2011..2025}.csv` plus
`manifest.json`. `cache/prices/` is committed (165 ticker CSVs). This data is
the sole input to `generate_instances.py build` and must not be re-fetched.

---

## BUG 5 — Population Initialization (⚠️ OPEN, BLOCKS ALL EXPERIMENTS)

Present in all six `*_solver.cpp` files (commit `6b6aa16`). Two compounding
defects:

1. `std::vector<unsigned> positive_expected_returns_indexes(num_assets)` allocates
   `num_assets` zeros, then `push_back` appends — phantom entries all point at
   asset 0.
2. Raw expected returns used as chromosome weights; negative sums yield entries
   outside `[0, 1]`.

**Effect:** Negative Shannon entropy, portfolio variance above single-asset
maximum. Reproduced on `ibov_2020` (66/100 NS-BRKGA solutions have negative
entropy). `ibov_2011` is unaffected only because its first asset has positive
mean.

**Any experiment run before this is fixed is compromised.**

---

## Phase 4A — Fix BUG 5

**Objective:** Fix the population initialization across all six solvers so that
initial chromosomes are well-formed `[0, 1]` vectors and the resulting
portfolios have non-negative weights summing to 1.

### Affected files (6 solvers)

| File | Lines (approx) |
|---|---|
| [nsga2_solver.cpp](file:///home/luishpmendes/mopop/src/solver/nsga2/nsga2_solver.cpp) | 27–88 |
| [nspso_solver.cpp](file:///home/luishpmendes/mopop/src/solver/nspso/nspso_solver.cpp) | 27–89 |
| [moead_solver.cpp](file:///home/luishpmendes/mopop/src/solver/moead/moead_solver.cpp) | 27–89 |
| [mhaco_solver.cpp](file:///home/luishpmendes/mopop/src/solver/mhaco/mhaco_solver.cpp) | 27–88 |
| [ihs_solver.cpp](file:///home/luishpmendes/mopop/src/solver/ihs/ihs_solver.cpp) | 26–87 |
| [nsbrkga_solver.cpp](file:///home/luishpmendes/mopop/src/solver/nsbrkga/nsbrkga_solver.cpp) | 126–148 |

### Fix specification

1. **Construct index vector empty:** `reserve(num_assets)`, not `(num_assets)`.
2. **Filter strictly positive expected returns only.**
3. **Derive chromosome weights from a non-negative quantity** (e.g., the
   absolute value of expected returns, or uniform weights over the selected
   subset) so the normalizing sum is strictly positive.
4. **Guard `value[2]` (Sharpe ratio):** if `value[1] == 0.0`, set
   `value[2] = 0.0` instead of computing `0.0 / 0.0`.
5. **Guard population sizing:** if `positive_expected_returns_indexes.size() < 2`,
   skip the expected-returns-weighted seeding block entirely (fall back to
   random initialization for those slots).

### Tasks

1. Fix the initialization block in all six `*_solver.cpp` files.
2. Add the `value[2]` zero-variance guard in `solution.cpp` and
   `decoder.cpp` (keep them in sync — CLAUDE.md documents this duplication).
3. Update all six solver tests to run on `ibov_2020/train/` (the instance
   that triggers the bug) and assert:
   - All objective values are finite (`std::isfinite`).
   - Shannon entropy ≥ 0 for every archived solution.
   - Portfolio variance ≤ max single-asset variance.
4. Rebuild and run all tests: `make clean && make all`.

### Acceptance criteria

- `make clean && make all` passes (all 9 tests).
- NS-BRKGA on `ibov_2020/train/` with seed 305089489, 5 s: 0 solutions with
  negative entropy.
- No solution in any solver's archive has `NaN` or `Inf` in its objective
  values.
- Legacy `input/` fixture tests still pass.

### Dependencies

None (self-contained).

### Risks

- Changing initialization affects solver convergence — existing single-instance
  results are not reproducible after this change. This is expected and
  acceptable since those results were wrong.

---

## Phase 4B — Multi-Instance Run Script

**Objective:** Rewrite `run.sh` to loop over instances, using `motsp_irace`'s
pattern. Rename output directories to `hvr/`, `nigd_plus/`.

**Depends on:** Phase 4A (solvers must be correct).

### Files to modify/create

#### [MODIFY] [run.sh](file:///home/luishpmendes/mopop/run.sh)

Rewrite to follow [motsp_irace/run.sh](file:///home/luishpmendes/UNICAMP/Doutorado/motsp_irace/run.sh):

- Add `set -euo pipefail`.
- Add outer `for instance in ${instances[@]}` loop.
- Use 10 seeds.
- Use renamed directories: `hvr/`, `hvr_snapshots/`, `nigd_plus/`,
  `nigd_plus_snapshots/`.
- Use `{instance}_{solver}_{seed}` filename pattern.
- Point each instance to `instances/${instance}/train/expected_returns.csv` and
  `instances/${instance}/train/covariance_matrix.csv`.
- Reference front/point computed **per instance**: `pareto/${instance}.txt` and
  `pareto/${instance}_point.txt`.
- HVR and NIGD+ computed **per instance**, reading the instance's own reference
  front and reference point.
- Results aggregator invoked **per instance per solver** using
  `{instance}_{solver}` output prefix.
- Parameters: `time_limit=3600`, `max_num_solutions=500`,
  `max_num_snapshots=30`, `max_ref_solutions=800`.

> [!IMPORTANT]
> `mopop`'s `hypervolume_ratio_calculator_exec` and
> `normalized_modified_generational_distance_calculator_exec` take
> `--reference-point` in addition to `--reference-pareto`, unlike `motsp_irace`
> which derives the reference point from `instance.primal_bound`. The run script
> must pass `--reference-point ${path}/pareto/${instance}_point.txt` to both.

#### [NEW] `run_smoke.sh`

Same structure but: 1 instance (`ibov_2015`), 2 seeds, 5 s time limit.

#### [MODIFY] [plotter_definitions.py](file:///home/luishpmendes/mopop/plotter_definitions.py)

```python
instances = ["ibov_2011", "ibov_2012", ..., "ibov_2020"]
seeds = [305089489, 511812191, ..., 755772793]  # 10 seeds
num_snapshots = 30
# Remove: expected_returns, covariance (no longer single-instance)
```

#### [MODIFY] All `plotter_*.py` files

- Add instance loop.
- Update directory names (`hypervolume` → `hvr`, `igd_plus` → `nigd_plus`).
- Update filename pattern to `{instance}_{solver}_{seed}`.
- Input file extensions are `.txt`, not `.csv` (fix existing mismatch noted in
  `plotter_definitions.py`).

### Output structure

```
mopop/
├── statistics/       ibov_2011_nsga2_305089489.txt ...
├── pareto/           ibov_2011_nsga2_305089489.txt, ibov_2011.txt, ibov_2011_point.txt ...
├── hvr/              ibov_2011_nsga2_305089489.txt ...
├── hvr_snapshots/    ibov_2011_nsga2_305089489.txt ...
├── nigd_plus/        ibov_2011_nsga2_305089489.txt ...
├── nigd_plus_snapshots/ ...
├── best_solutions_snapshots/ ...
├── num_non_dominated_snapshots/ ...
├── num_fronts_snapshots/ ...
├── num_elites_snapshots/ ...
├── populations_snapshots/ ...
├── metrics/ ...
└── metrics_snapshots/ ...
```

### Tasks

1. Rewrite `run.sh` with instance loop, 10 seeds, renamed dirs.
2. Create `run_smoke.sh`.
3. Update `plotter_definitions.py`.
4. Update all 12 `plotter_*.py` files for multi-instance.
5. Delete `plotter_num_fronts_snapshots copy.py` (stale duplicate).

### Acceptance criteria

- `run_smoke.sh` completes end-to-end on 1 instance, 2 seeds, 5 s.
- Output files follow `{instance}_{solver}_{seed}` naming.
- Reference front computed per instance (not globally).
- All 6 solvers produce non-empty pareto files.
- No file references old directory names (`hypervolume/`, `igd_plus/`).

### Runtime estimate (full run)

```
10 instances × 6 solvers × 10 seeds × 3600 s = 600 runs @ 1 hr each
With 6 workers ≈ 100 hours wall-clock
```

---

## Phase 5 — `irace` Parameter Tuning

**Objective:** Set up `irace` workflows for all 6 solvers.

**Depends on:** Phase 4A (solvers correct), Phase 4B (instances wired up).

### Key difference from `motsp_irace`

`motsp_irace`'s target runners use `--instance <path>` (single file) and the
raw HV exec derives its reference point from `instance.primal_bound`. In
`mopop`, each solver takes two instance files (`--expected-returns-filename` and
`--covariance-filename`) and the raw HV exec reads its reference point from a
file (`--reference-point`). The target runners must:

1. Parse `$4` as an instance name (e.g., `ibov_2015`).
2. Map it to `instances/${INSTANCE}/train/expected_returns.csv` and
   `instances/${INSTANCE}/train/covariance_matrix.csv`.
3. Pass `--reference-point instances/${INSTANCE}/reference_point.txt` to the HV
   calculator.

### Prerequisite: frozen reference points for tuning instances

Before irace can run, each training instance needs a **frozen** reference point.
Compute it once from a pilot pool (all 6 solvers × 2 seeds × 60 s) and store it
as `instances/${instance}/reference_point.txt`. The point must not be recomputed
per candidate, or costs become incomparable. The 5%-of-range padding ensures
that a later run slightly exceeding the pilot pool still lands inside the
dominated region.

### Directory structure

```
irace/
├── train-instances.txt         (ibov_2011 .. ibov_2017, one per line)
├── test-instances.txt          (ibov_2018 .. ibov_2020, one per line)
├── nsga2-parameters.txt
├── nsga2-scenario.txt
├── nsga2-tunner.sh
├── nspso-parameters.txt
├── nspso-scenario.txt
├── nspso-tunner.sh
├── moead-parameters.txt
├── moead-scenario.txt
├── moead-tunner.sh
├── mhaco-parameters.txt
├── mhaco-scenario.txt
├── mhaco-tunner.sh
├── ihs-parameters.txt
├── ihs-scenario.txt
├── ihs-tunner.sh
├── nsbrkga-parameters-stage{1..6}.txt
├── nsbrkga-scenario-stage{1..6}.txt
├── nsbrkga-tunner-stage{1..6}.sh
└── irace_runner.sh
```

### Instance files (matching `motsp_irace` format)

**`train-instances.txt`:**
```
ibov_2011
ibov_2012
ibov_2013
ibov_2014
ibov_2015
ibov_2016
ibov_2017
```

**`test-instances.txt`:**
```
ibov_2018
ibov_2019
ibov_2020
```

### Scenario files (example: `nsga2-scenario.txt`)

```
execDir = "./"
trainInstancesDir = "../instances"
trainInstancesFile = "./train-instances.txt"
targetRunner = "./nsga2-tunner.sh"
maxTime = 2160000
parallel = 1
logFile = "./irace-nsga2.Rdata"
parameterFile = "./nsga2-parameters.txt"
testInstancesDir = "../instances"
testInstancesFile = "./test-instances.txt"
testNbElites = 5
testIterationElites = 0
```

> [!NOTE]
> `trainInstancesDir = "../instances"` means irace passes `../instances/ibov_2015`
> as `$4`. The target runner appends `/train/expected_returns.csv` and
> `/train/covariance_matrix.csv`.

### Target runner contract (all solvers)

Each `{solver}-tunner.sh` must:
1. Parse: `$1`=config_id, `$2`=instance_id, `$3`=seed, `$4`=instance_path.
2. Derive `ER=${4}/train/expected_returns.csv`,
   `COV=${4}/train/covariance_matrix.csv`,
   `REF=${4}/reference_point.txt`.
3. Run solver for 60 s with `--pareto` output to tmpdir.
4. Compute raw HV: `hypervolume_calculator_exec --expected-returns-filename $ER
   --covariance-filename $COV --reference-point $REF --pareto-0 $PARETO_FILE
   --hypervolume-0 $HV_FILE`.
5. Print `cost elapsed_time` where `cost = -HV` (negated, since irace
   minimizes) or `Inf` on failure.
6. Clean tmpdir via `trap`.

### Parameter files

Derive from actual solver CLI flags (verified from `*_solver_exec.cpp` usage
strings):

| Solver | Parameters |
|---|---|
| **NSGA-II** | `population_size_factor` i(25,125), `crossover_probability` r(0.01,0.99), `crossover_distribution` r(1,99), `mutation_probability` r(0.01,0.99), `mutation_distribution` r(1,99) |
| **NSPSO** | `population_size_factor` i(25,125), `omega` r(0,1), `v_coeff` r(0,1), `chi` r(0,1), `v_max` r(0,1), `u_min` r(0,1), `u_max` r(0,1), `memory` c(0,1) |
| **MOEA/D-DE** | `population_size_factor` i(25,125), `weight_generation` c(grid,low_discrepancy,random), `decomposition` c(tchebycheff,weighted,bi), `cr` r(0,1), `f` r(0,2), `neighbours` i(2,50), `preserve_diversity` c(0,1) |
| **MHACO** | `population_size_factor` i(25,125), `ker` i(2,100), `q` r(0.01,100), `threshold` i(1,100), `n_gen_mark` i(1,100), `focus` r(0,1), `memory` c(0,1) |
| **IHS** | `population_size_factor` i(25,125), `phmcr` r(0.01,0.99), `ppar_min` r(0.01,0.99), `ppar_max` r(0.01,0.99), `bw_min` r(1e-6,1), `bw_max` r(1e-6,1) |
| **NS-BRKGA** | 6-stage ablation (identical structure to `motsp_irace`) |

### Tasks

1. Create `irace/train-instances.txt` and `irace/test-instances.txt`.
2. Create `scripts/build_pilot_reference_points.sh`: runs all 6 solvers ×
   2 seeds × 60 s on each training instance, then computes the reference front
   and reference point, writing `instances/${instance}/reference_point.txt`.
3. Run the pilot to generate frozen reference points.
4. Create parameter files for each solver.
5. Create scenario files for each solver.
6. Create target runner (`*-tunner.sh`) for each solver.
7. Create NS-BRKGA staged parameter/scenario files (6 stages).
8. Create `irace/irace_runner.sh` orchestrator.

### Acceptance criteria

- Every target runner passes `irace --check`.
- 10-evaluation smoke test completes for each solver.
- stdout contains only `cost elapsed_time`.
- Holdout instances untouched during tuning.
- `instances/ibov_{2011..2017}/reference_point.txt` exist and contain exactly
  4 space-separated floats.

---

## Phase 6 — Result Aggregation, Statistics, and Plotting

**Objective:** Aggregate multi-instance results, compute statistics, generate
publication-quality plots.

**Depends on:** Phase 4B (run script complete), Phase 5 (tuned parameters used
in final runs).

### Files to create/modify

#### [NEW] `scripts/results_aggregator.py`

Mirror [motsp_irace/results_aggregator.py](file:///home/luishpmendes/UNICAMP/Doutorado/motsp_irace/results_aggregator.py):

- Read HVR from `hvr/{instance}_{solver}_{seed}.txt` (one float per file).
- Read NIGD+ from `nigd_plus/{instance}_{solver}_{seed}.txt`.
- Read snapshots from `hvr_snapshots/` and `nigd_plus_snapshots/`.
- Output `metrics.csv` (tidy, one row per `(instance, solver, seed, metric)`).
- Output `metrics_snapshots.csv`.
- Output per-metric stats: `hvr_stats.csv`, `nigd_plus_stats.csv` with
  mean, std, rank.

#### [NEW] `scripts/metrics_stats.py`

Mirror [motsp_irace/metrics_stats.py](file:///home/luishpmendes/UNICAMP/Doutorado/motsp_irace/metrics_stats.py):

- Per-instance and all-instance mean ± std for each solver × metric.
- Print to stdout (tee to `metrics_stats.txt` in `run.sh`).

#### [MODIFY] All plotter scripts

Already handled in Phase 4B for the instance loop; this phase adds:

- Cross-instance aggregate plots (algorithm-rank boxplots).
- Convergence curves (median ± IQR) per instance.
- Pareto front plots for best/median runs per instance.

### Required plots

1. Per-instance HVR and NIGD+ boxplots (6 solvers).
2. Aggregate rank plots across all 10 instances.
3. Convergence curves (median ± IQR) per instance.
4. Pareto front plots for best/median runs per instance.

### Tasks

1. Implement `scripts/results_aggregator.py`.
2. Implement `scripts/metrics_stats.py`.
3. Wire both into `run.sh` (after the results_aggregator_exec block).
4. Add cross-instance aggregate plots to existing plotters.

### Acceptance criteria

- `metrics.csv` has exactly `10 × 6 × 10 × 2 = 1200` rows (600 HVR + 600 NIGD+).
- Missing data detected and reported (non-silent).
- All plots generate headlessly (`matplotlib.use('Agg')`).
- `metrics_stats.txt` contains per-instance and aggregate tables.

---

## Phase 7 — Tests and Documentation

**Objective:** Harden tests, complete README, ensure clean-clone
reproducibility.

**Depends on:** Phases 4A, 4B, 5, 6.

### Files to modify/create

#### [MODIFY] [.gitignore](file:///home/luishpmendes/mopop/.gitignore)

Add:
```
instances/*/train/
instances/*/oos/
instances/*/reference_point.txt
statistics/
solutions/
pareto/
hvr/
hvr_snapshots/
nigd_plus/
nigd_plus_snapshots/
best_solutions_snapshots/
num_non_dominated_snapshots/
num_fronts_snapshots/
num_elites_snapshots/
populations_snapshots/
metrics/
metrics_snapshots/
irace/*.Rdata
irace/*-tuning.log
irace/*-testing.log
```

#### [MODIFY] [README.md](file:///home/luishpmendes/mopop/README.md)

Sections:
1. Problem formulation (4 objectives, mixed senses).
2. Dependencies and build (`make all` with configurable paths).
3. Python environment setup (`requirements.txt`).
4. Instance generation commands.
5. Smoke experiment command (`run_smoke.sh`).
6. Full experiment command + runtime estimate.
7. HVR and NIGD+ definitions (exact formulas).
8. `irace` tuning procedure.
9. Result aggregation and plotting commands.
10. Reproducibility notes (Yahoo Finance caveats, survivorship bias).

#### Stale file cleanup

- Delete `plotter_num_fronts_snapshots copy.py`.
- Verify `input/` is only used by tests; if so, document in README.

### Tasks

1. Update `.gitignore`.
2. Write complete README.
3. Delete stale files.
4. Verify clean-clone smoke execution (clone, build, `run_smoke.sh`).

### Acceptance criteria

- New user with documented dependencies can run smoke profile from clean clone.
- `plotter_num_fronts_snapshots copy.py` removed.
- `.gitignore` covers all generated artifacts.

---

## Dependency Graph

```mermaid
graph TD
    P4A["Phase 4A: Fix BUG 5"] --> P4B["Phase 4B: Multi-Instance Run Script"]
    P4A --> P5["Phase 5: irace Tuning"]
    P4B --> P5
    P4B --> P6["Phase 6: Aggregation & Plots"]
    P5 --> P6
    P5 --> P7["Phase 7: Tests & Documentation"]
    P6 --> P7
```

**Critical path:** 4A → 4B → 5 → 6 → 7.

Phase 5 can start its file-creation tasks (parameter files, scenario files,
target runners) as soon as 4A is done, but the actual tuning runs require 4B's
`run_smoke.sh` to have validated the pipeline.

---

## Reference Projects

| Project | Path | Used for |
|---|---|---|
| `motsp_irace` | `/home/luishpmendes/UNICAMP/Doutorado/motsp_irace` | `run.sh`, irace files, `plotter_definitions.py`, `results_aggregator.py`, `metrics_stats.py` |
| `mopci` | `/home/luishpmendes/UNICAMP/Doutorado/mopci` | Additional plotter patterns if needed |
| `motmdsp` | `/home/luishpmendes/UNICAMP/Doutorado/motmdsp` | Additional plotter patterns if needed |

---

## Risks

| Risk | Impact | Mitigation |
|---|---|---|
| BUG 5 fix changes solver convergence | Existing results not reproducible | Expected — old results were wrong |
| `population_size_factor × 4` too small for seeded individuals | Runtime crash (pagmo requires `pop_size > 2 × num_assets + seeded_count`) | Add guard: `max(factor × 4, 2 × num_assets + positive_count + 1)` |
| irace reference point drift | Costs incomparable across candidates | Freeze reference point from pilot pool |
| Yahoo re-adjusts prices | Cache diverges from fresh download | Cache is committed and pinned; never re-fetch |
| 100+ hours wall-clock for full run | Long feedback loop | `run_smoke.sh` for quick validation |
| `results_aggregator_exec` flag mismatch (`--nigd-pluses-statistics` vs `--nigd-plus-statistics`) | Silently drops NIGD+ stats file | Already fixed in Phase 3; verify in 4B |

---

## What Was Removed from Prior Plans

1. **YAML configuration system** — unnecessary; hardcode in shell scripts.
2. **OOS portfolio reevaluation pipeline** — deferred.
3. **Atomic rename / resume / force behavior** — over-engineering.
4. **Nested `results/<experiment_id>/` structure** — use flat directories.
5. **Separate `src/metrics/` library** — keep metric logic in exec files.
6. **Multiple shell scripts (`build_reference_fronts.sh`, `compute_metrics.sh`)** — consolidated into `run.sh`.
7. **Speculative features** (training-vs-OOS degradation plots, effect-size measures).
8. **Phase 1 (Build Portability)** — `requirements.txt` exists; `make check` deferred to Phase 7.
