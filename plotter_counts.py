"""Shared plotting for the per-snapshot count series.

num_non_dominated_snapshots, num_fronts_snapshots and num_elites_snapshots all
hold the same shape of data - space separated "iteration time count" rows - and
get the same three figures, so the body lives here once and each plotter script
supplies only its directory and axis label.
"""

import matplotlib
matplotlib.use("agg")
import matplotlib.pyplot as plt
from plotter_definitions import *
from plotter_utils import load_snapshots, mean, path, quartiles, read_rows


def plot(directory, ylabel, basename, only_solvers=None):
  """Draw the per-instance curves and the pooled mean/quartile curves."""
  chosen = solvers if only_solvers is None else only_solvers

  # The best and median run of each instance, plotted as its own curve.
  for instance in instances:
    for version in versions:
      plt.figure()
      plt.xlabel("Time (s)", fontsize="x-large")
      plt.ylabel(ylabel, fontsize="x-large")
      plotted = False
      for i, solver in enumerate(solvers):
        if solver not in chosen:
          continue
        rows = read_rows(
            path(directory, instance + "_" + solver + "_" + version + ".txt"),
            delimiter=" ")
        rows = [row for row in rows if len(row) > 2]
        if not rows:
          continue
        plotted = True
        plt.plot([float(row[1]) for row in rows],
                 [float(row[2]) for row in rows],
                 label=solver_labels[solver], color=colors[i],
                 marker=(i + 3, 2, 0), alpha=0.80)
      if plotted:
        plt.xscale("log")
        plt.legend(loc="best", fontsize="large")
        plt.savefig(path(directory, instance + "_" + version + ".png"),
                    format="png")
      plt.close()

  time_per_solver, value_per_solver, _ = load_snapshots(
      directory, delimiter=" ", only_solvers=chosen)

  # Mean over every instance and seed.
  plt.figure()
  plt.title("MOPOP", fontsize="xx-large")
  plt.xlabel("Time (s)", fontsize="x-large")
  plt.ylabel(ylabel, fontsize="x-large")
  for i, solver in enumerate(solvers):
    if solver not in chosen:
      continue
    x = [mean(time_per_solver[solver][j]) for j in range(num_snapshots)]
    y = [mean(value_per_solver[solver][j]) for j in range(num_snapshots)]
    plt.plot(x, y, label=solver_labels[solver], marker=(i + 3, 2, 0),
             color=colors[i], alpha=0.80)
  plt.xscale("log")
  plt.legend(loc="best", fontsize="large")
  plt.savefig(path(directory, basename + "_mean_snapshots.png"), format="png")
  plt.close()

  # Median with an interquartile band.
  plt.figure()
  plt.title("MOPOP", fontsize="xx-large")
  plt.xlabel("Time (s)", fontsize="x-large")
  plt.ylabel(ylabel, fontsize="x-large")
  for i, solver in enumerate(solvers):
    if solver not in chosen:
      continue
    x = [mean(time_per_solver[solver][j]) for j in range(num_snapshots)]
    q = [quartiles(value_per_solver[solver][j]) for j in range(num_snapshots)]
    plt.fill_between(x, [value[0] for value in q], [value[2] for value in q],
                     color=colors[i], alpha=0.25)
  for i, solver in enumerate(solvers):
    if solver not in chosen:
      continue
    x = [mean(time_per_solver[solver][j]) for j in range(num_snapshots)]
    y = [quartiles(value_per_solver[solver][j])[1]
         for j in range(num_snapshots)]
    plt.plot(x, y, label=solver_labels[solver], marker=(i + 3, 2, 0),
             color=colors[i], alpha=0.75)
  plt.xscale("log")
  plt.legend(loc="best", fontsize="large")
  plt.savefig(path(directory, basename + "_quartiles_snapshots.png"),
              format="png")
  plt.close()
