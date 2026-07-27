import gc
import os
import matplotlib
matplotlib.use("agg")
import matplotlib.pyplot as plt
import seaborn as sns
from math import ceil, floor, sqrt
from plotter_definitions import *
from plotter_utils import path, read_rows, report_missing

num_rows = floor(sqrt(len(solvers)))
num_cols = ceil(len(solvers) / num_rows)


def snapshot_file(directory, instance, solver, version, snapshot):
  return path(directory,
              instance + "_" + solver + "_" + version + "_" + str(snapshot)
              + ".txt")


def objectives(directory, instance, solver, version, snapshot):
  """Return m lists of objective values, or None when the file is absent."""
  rows = read_rows(snapshot_file(directory, instance, solver, version,
                                 snapshot), delimiter=" ", skip_header=True)
  rows = [row for row in rows if len(row) >= m]
  if not rows:
    return None
  return [[float(row[j]) for row in rows] for j in range(m)]


def draw(axs, ys, color, alpha, marker, min_ys, max_ys):
  """Draw one solver's m x m density/scatter matrix into axs."""
  for j in range(m):
    axs[j][j].set_xlim(left=min_ys[j], right=max_ys[j])
    axs[j][j].set_xlabel(xlabel="$f_{" + str(j + 1) + "}$", fontsize="large")
    axs[j][j].set_yticks([])
    axs[j][j].set_ylabel(ylabel="Density", fontsize="large")
    sns.kdeplot(data=ys[j], ax=axs[j][j], color=color, marker=marker,
                alpha=alpha)
    for k in range(m):
      if j != k:
        axs[j][k].set_xlim(left=min_ys[k], right=max_ys[k])
        axs[j][k].set_ylim(bottom=min_ys[j], top=max_ys[j])
        axs[j][k].set_xlabel(xlabel="$f_{" + str(k + 1) + "}$",
                             fontsize="large")
        axs[j][k].set_ylabel(ylabel="$f_{" + str(j + 1) + "}$",
                             fontsize="large")
        axs[j][k].scatter(x=ys[k], y=ys[j], color=color, marker=marker,
                          alpha=alpha)


for instance in instances:
  for version in versions:
    # ffmpeg needs frames numbered 0..n-1 without gaps, so take the longest
    # run any solver produced.
    num_frames = 0
    for solver in solvers:
      n = 0
      while os.path.exists(snapshot_file("populations_snapshots", instance,
                                         solver, version, n)):
        n += 1
      num_frames = max(num_frames, n)
    if num_frames == 0:
      continue

    # Common axis limits, so the frames of one video share a scale.
    min_ys = [None] * m
    max_ys = [None] * m
    for solver in solvers:
      for snapshot in range(num_frames):
        ys = objectives("populations_snapshots", instance, solver, version,
                        snapshot)
        if ys is None:
          continue
        for i in range(m):
          low, high = min(ys[i]), max(ys[i])
          if min_ys[i] is None or min_ys[i] > low:
            min_ys[i] = low
          if max_ys[i] is None or max_ys[i] < high:
            max_ys[i] = high
    if any(value is None for value in min_ys):
      continue
    for i in range(m):
      delta_y = max_ys[i] - min_ys[i]
      min_ys[i] -= 0.025 * delta_y
      max_ys[i] += 0.025 * delta_y

    for snapshot in range(num_frames):
      fig = plt.figure(figsize=(5.0 * num_cols * m, 5.0 * num_rows * m),
                       constrained_layout=True)
      fig.set_size_inches(5.0 * num_cols * m, 5.0 * num_rows * m)
      figs = fig.subfigures(nrows=num_rows, ncols=num_cols, wspace=0.05,
                            hspace=0.05)
      for i, solver in enumerate(solvers):
        subfig = figs[floor(i / num_cols)][i % num_cols]
        subfig.suptitle(solver_labels[solver], fontsize="x-large")
        axs = subfig.subplots(nrows=m, ncols=m)
        marker = (i + 3, 2, 0)
        ys = objectives("populations_snapshots", instance, solver, version,
                        snapshot)
        if ys is not None:
          draw(axs, ys, colors[i], 0.50, marker, min_ys, max_ys)
        ys = objectives("best_solutions_snapshots", instance, solver, version,
                        snapshot)
        if ys is not None:
          draw(axs, ys, colors2[i], 0.75, marker, min_ys, max_ys)
      plt.savefig(path("populations_snapshots",
                       instance + "_" + version + "_" + str(snapshot)
                       + ".png"), format="png")
      plt.close(fig)
      plt.cla()
      del fig
      gc.collect()

report_missing("plotter_populations_snapshots.py")
