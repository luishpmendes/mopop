import csv
import matplotlib.pyplot as plt
import os
import seaborn as sns
from plotter_definitions import *

dirname = os.path.dirname(__file__)

for version in versions:
  fig, axs = plt.subplots(nrows=m, ncols=m, figsize=(
      5.0 * m, 5.0 * m), squeeze=False, num=1, clear=True)
  fig.set_size_inches(5.0 * m, 5.0 * m)
  for i in range(len(solvers)):
    filename = os.path.join(
        dirname, "pareto/" + solvers[i] + "_" + version + ".txt")
    if os.path.exists(filename):
      ys = []
      for j in range(m):
        ys.append([])
      with open(filename) as csv_file:
        data = csv.reader(csv_file, delimiter=" ")
        for row in data:
          for j in range(m):
            ys[j].append(float(row[j]))
        csv_file.close()
      for j in range(m):
        axs[j][j].set_xlabel(
            xlabel="$f_{" + str(j + 1) + "}$", fontsize="x-large")
        axs[j][j].set_yticks([])
        axs[j][j].set_ylabel(ylabel="Density", fontsize="x-large")
        sns.kdeplot(data=ys[j], ax=axs[j][j], color=colors[i],
                    label=solver_labels[solvers[i]], marker=(i + 3, 2, 0), alpha=0.80, cut=0.0)
        axs[j][j].legend(loc="best", fontsize="large")
        for k in range(m):
          if j != k:
            axs[j][k].set_xlabel(
                xlabel="$f_{" + str(k + 1) + "}$", fontsize="x-large")
            axs[j][k].set_ylabel(
                ylabel="$f_{" + str(j + 1) + "}$", fontsize="x-large")
            axs[j][k].scatter(x=ys[k], y=ys[j], color=colors[i],
                              label=solver_labels[solvers[i]], marker=(i + 3, 2, 0), alpha=0.80)
            axs[j][k].legend(loc="best", fontsize="large")
      del ys
  plt.subplots_adjust(wspace=0.16 + 0.07 * m, hspace=0.16 + 0.07 * m)
  filename = os.path.join(dirname, "pareto/" + version + ".png")
  plt.savefig(filename, format="png")
