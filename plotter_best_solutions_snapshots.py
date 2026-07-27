import os
import matplotlib
matplotlib.use("agg")
import matplotlib.pyplot as plt
import seaborn as sns
from plotter_definitions import *
from plotter_utils import path, read_rows, report_missing


def snapshot_file(instance, solver, version, snapshot):
  return path("best_solutions_snapshots",
              instance + "_" + solver + "_" + version + "_" + str(snapshot)
              + ".txt")


def count_snapshots(instance, version):
  """Number of contiguous snapshot files, taken over the longest solver.

  ffmpeg needs frames numbered 0..n-1 without gaps, so the frame count is the
  longest run any solver produced rather than the last one probed.
  """
  count = 0
  for solver in solvers:
    n = 0
    while os.path.exists(snapshot_file(instance, solver, version, n)):
      n += 1
    count = max(count, n)
  return count


for instance in instances:
  for version in versions:
    num_frames = count_snapshots(instance, version)
    if num_frames == 0:
      continue

    # Common axis limits, so the frames of one video share a scale.
    min_ys = [None] * m
    max_ys = [None] * m
    for solver in solvers:
      for snapshot in range(num_frames):
        for row in read_rows(snapshot_file(instance, solver, version, snapshot),
                             delimiter=" ", skip_header=True):
          if len(row) < m:
            continue
          for i in range(m):
            value = float(row[i])
            if min_ys[i] is None or min_ys[i] > value:
              min_ys[i] = value
            if max_ys[i] is None or max_ys[i] < value:
              max_ys[i] = value
    if any(value is None for value in min_ys):
      continue
    for i in range(m):
      delta_y = max_ys[i] - min_ys[i]
      min_ys[i] = min_ys[i] - 0.025 * delta_y
      max_ys[i] = max_ys[i] + 0.025 * delta_y

    for snapshot in range(num_frames):
      fig, axs = plt.subplots(nrows=m, ncols=m, figsize=(5.0 * m, 5.0 * m),
                              squeeze=False, num=1, clear=True)
      fig.set_size_inches(5.0 * m, 5.0 * m)
      for i, solver in enumerate(solvers):
        rows = read_rows(snapshot_file(instance, solver, version, snapshot),
                         delimiter=" ", skip_header=True)
        rows = [row for row in rows if len(row) >= m]
        if not rows:
          continue
        ys = [[float(row[j]) for row in rows] for j in range(m)]
        for j in range(m):
          axs[j][j].set_xlim(left=min_ys[j], right=max_ys[j])
          axs[j][j].set_xlabel(
              xlabel="$f_{" + str(j + 1) + "}$", fontsize="x-large")
          axs[j][j].set_yticks([])
          axs[j][j].set_ylabel(ylabel="Density", fontsize="x-large")
          sns.kdeplot(data=ys[j], ax=axs[j][j], color=colors[i],
                      label=solver_labels[solver], marker=(i + 3, 2, 0),
                      alpha=0.80)
          axs[j][j].legend(loc="best", fontsize="large")
          for k in range(m):
            if j != k:
              axs[j][k].set_xlim(left=min_ys[k], right=max_ys[k])
              axs[j][k].set_ylim(bottom=min_ys[j], top=max_ys[j])
              axs[j][k].set_xlabel(
                  xlabel="$f_{" + str(k + 1) + "}$", fontsize="x-large")
              axs[j][k].set_ylabel(
                  ylabel="$f_{" + str(j + 1) + "}$", fontsize="x-large")
              axs[j][k].scatter(x=ys[k], y=ys[j], color=colors[i],
                                label=solver_labels[solver],
                                marker=(i + 3, 2, 0), alpha=0.80)
              axs[j][k].legend(loc="best", fontsize="large")
        del ys
      plt.subplots_adjust(wspace=0.15 + 0.05 * m, hspace=0.15 + 0.05 * m)
      plt.savefig(path("best_solutions_snapshots",
                       instance + "_" + version + "_" + str(snapshot)
                       + ".png"), format="png")

report_missing("plotter_best_solutions_snapshots.py")
