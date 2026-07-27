"""Shared reading and statistics helpers for the plotter scripts.

Every plotter reads hundreds of per-run files. A single absent run - a solver
that crashed, a metric that was never computed - must not abort the whole plot
stage, so reads are guarded and misses are reported once at the end instead of
raising. The statistics helpers are likewise total: they return NaN rather than
raising on an empty sample.
"""

import csv
import os
import statistics

from plotter_definitions import instances, num_snapshots, seeds, solvers

DIRNAME = os.path.dirname(os.path.abspath(__file__))

_missing = []


def path(*parts):
  """Return an absolute path inside the repository."""
  return os.path.join(DIRNAME, *parts)


def read_rows(filename, delimiter=",", skip_header=False):
  """Return the non-empty rows of filename, or [] when it does not exist.

  A missing file is recorded for report_missing() rather than raising.
  """
  if not os.path.exists(filename):
    _missing.append(filename)
    return []
  with open(filename) as csv_file:
    if skip_header:
      next(csv_file, None)
    return [row for row in csv.reader(csv_file, delimiter=delimiter) if row]


def read_column(filename, column=0, delimiter=",", skip_header=False):
  """Return one column of filename as floats, or [] when it does not exist."""
  return [float(row[column])
          for row in read_rows(filename, delimiter, skip_header)
          if len(row) > column]


def report_missing(script):
  """Print a one-line summary of the files that could not be read."""
  if _missing:
    print("%s: %d input file(s) missing, first is %s"
          % (script, len(_missing), os.path.relpath(_missing[0], DIRNAME)))


def load_snapshots(directory, column=2, delimiter=",", only_solvers=None):
  """Load a per-run snapshot series pooled over every instance and seed.

  Snapshot files hold one row per snapshot: iteration, elapsed time, value.
  Returns (time_per_solver, value_per_solver, value_per_snapshot), where
  time_per_solver[solver][k] and value_per_solver[solver][k] pool snapshot k
  over instances and seeds, and value_per_snapshot[k][i] holds the values of
  solvers[i] at snapshot k. Rows beyond num_snapshots are ignored, so a run
  that recorded more snapshots than configured cannot misalign the series.

  only_solvers restricts the read to a subset - num_elites_snapshots exists for
  NS-BRKGA alone, and reading the others would report every absent file as a
  missing input.
  """
  chosen = solvers if only_solvers is None else only_solvers
  time_per_solver = {solver: [[] for _ in range(num_snapshots)]
                     for solver in solvers}
  value_per_solver = {solver: [[] for _ in range(num_snapshots)]
                      for solver in solvers}
  value_per_snapshot = [[[] for _ in solvers] for _ in range(num_snapshots)]

  for i, solver in enumerate(solvers):
    if solver not in chosen:
      continue
    for instance in instances:
      for seed in seeds:
        filename = path(directory,
                        instance + "_" + solver + "_" + str(seed) + ".txt")
        for k, row in enumerate(read_rows(filename, delimiter)):
          if k >= num_snapshots or len(row) <= column:
            break
          time_per_solver[solver][k].append(float(row[1]))
          value_per_solver[solver][k].append(float(row[column]))
          value_per_snapshot[k][i].append(float(row[column]))

  return time_per_solver, value_per_solver, value_per_snapshot


def mean(values):
  """Arithmetic mean, or NaN for an empty sample."""
  return statistics.mean(values) if values else float("nan")


def stdev(values):
  """Sample standard deviation, or 0.0 when there are fewer than two values."""
  return statistics.stdev(values) if len(values) > 1 else 0.0


def quartiles(values):
  """The three quartiles, degrading gracefully on short samples."""
  if len(values) > 1:
    return tuple(statistics.quantiles(values))
  value = values[0] if values else float("nan")
  return (value, value, value)
