import matplotlib
matplotlib.use("agg")
import matplotlib.pyplot as plt
import seaborn as sns
from plotter_definitions import *
from plotter_utils import path, read_rows, report_missing

for instance in instances:
  for version in versions:
    fig, axs = plt.subplots(nrows=m, ncols=m, figsize=(5.0 * m, 5.0 * m),
                            squeeze=False, num=1, clear=True)
    fig.set_size_inches(5.0 * m, 5.0 * m)
    plotted = False
    for i, solver in enumerate(solvers):
      rows = read_rows(
          path("pareto", instance + "_" + solver + "_" + version + ".txt"),
          delimiter=" ")
      rows = [row for row in rows if len(row) >= m]
      if not rows:
        continue
      plotted = True
      ys = [[float(row[j]) for row in rows] for j in range(m)]
      for j in range(m):
        axs[j][j].set_xlabel(
            xlabel="$f_{" + str(j + 1) + "}$", fontsize="x-large")
        axs[j][j].set_yticks([])
        axs[j][j].set_ylabel(ylabel="Density", fontsize="x-large")
        sns.kdeplot(data=ys[j], ax=axs[j][j], color=colors[i],
                    label=solver_labels[solver], marker=(i + 3, 2, 0),
                    alpha=0.80, cut=0.0)
        axs[j][j].legend(loc="best", fontsize="large")
        for k in range(m):
          if j != k:
            axs[j][k].set_xlabel(
                xlabel="$f_{" + str(k + 1) + "}$", fontsize="x-large")
            axs[j][k].set_ylabel(
                ylabel="$f_{" + str(j + 1) + "}$", fontsize="x-large")
            axs[j][k].scatter(x=ys[k], y=ys[j], color=colors[i],
                              label=solver_labels[solver],
                              marker=(i + 3, 2, 0), alpha=0.80)
            axs[j][k].legend(loc="best", fontsize="large")
      del ys
    if plotted:
      plt.subplots_adjust(wspace=0.16 + 0.07 * m, hspace=0.16 + 0.07 * m)
      plt.savefig(path("pareto", instance + "_" + version + ".png"),
                  format="png")

report_missing("plotter_pareto.py")
