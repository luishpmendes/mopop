import csv
import matplotlib.pyplot as plt
import seaborn as sns
import ptitprince as pt
import os
from plotter_definitions import *

dirname = os.path.dirname(__file__)

min_hypervolume = 1.0
max_hypervolume = 0.0
for solver in solvers:
  filename = os.path.join(dirname, "hypervolume/" + solver + ".txt")
  with open(filename) as csv_file:
    data = csv.reader(csv_file)
    for row in data:
      min_hypervolume = min(min_hypervolume, float(row[0]))
      max_hypervolume = max(max_hypervolume, float(row[0]))
delta_hypervolume = max_hypervolume - min_hypervolume
min_hypervolume = max(min_hypervolume - round(0.025 * delta_hypervolume), 0.00)
max_hypervolume = min(max_hypervolume + round(0.025 * delta_hypervolume), 1.00)

plt.figure(figsize=(11, 11))
plt.xlabel("Hypervolume Ratio", fontsize="x-large")
xs = []
for solver in solvers:
  filename = os.path.join(dirname, "hypervolume/" + solver + ".txt")
  x = []
  with open(filename) as csv_file:
    data = csv.reader(csv_file)
    for row in data:
      x.append(float(row[0]))
  xs.append(x)
pt.half_violinplot(data=xs, palette=colors, orient="h",
                   width=0.6, cut=0.0, inner=None)
sns.stripplot(data=xs, palette=colors, orient="h", size=2, zorder=0)
sns.boxplot(data=xs, orient="h", width=0.20, color="black", zorder=10, showcaps=True, boxprops={
            'facecolor': 'none', "zorder": 10}, showfliers=True, whiskerprops={'linewidth': 2, "zorder": 10}, flierprops={'markersize': 2})
plt.yticks(ticks=list(range(len(solvers))), labels=[
    solver_labels[solver] for solver in solvers], fontsize="large")
filename = os.path.join(dirname, "hypervolume/hypervolume.png")
plt.savefig(filename, format="png")
plt.close()

hypervolume = []

for solver in solvers:
  hypervolume.append([])

for i in range(len(solvers)):
  for seed in seeds:
    filename = os.path.join(dirname, "hypervolume/" +
                            solvers[i] + "_" + str(seed) + ".txt")
    if os.path.exists(filename):
      with open(filename) as csv_file:
        data = csv.reader(csv_file, delimiter=",")
        for row in data:
          hypervolume[i].append(float(row[0]))
        csv_file.close()

plt.figure()
plt.xlabel(fontsize="large", xlabel="Hypervolume Ratio")
plt.tick_params(axis="x", which="both", labelsize="large")
plt.grid(alpha=0.5, color="gray", linestyle="dashed",
         linewidth=0.5, which="both")
pt.half_violinplot(data=hypervolume, palette=colors,
                   orient="h", width=0.6, cut=0.0, inner=None)
sns.stripplot(data=hypervolume, palette=colors, orient="h", size=2, zorder=0)
sns.boxplot(data=hypervolume, orient="h", width=0.20, color="black", zorder=10, showcaps=True, boxprops={
            'facecolor': 'none', "zorder": 10}, showfliers=True, whiskerprops={'linewidth': 2, "zorder": 10}, flierprops={'markersize': 2})
plt.yticks(fontsize="large", ticks=list(range(len(solvers))),
           labels=[solver_labels[solver] for solver in solvers])
plt.tight_layout()
filename = os.path.join(dirname, "hypervolume/hypervolume.png")
plt.savefig(bbox_inches='tight', fname=filename, format="png")
filename = os.path.join(dirname, "hypervolume/hypervolume.pdf")
plt.savefig(bbox_inches='tight', fname=filename, format="pdf")
plt.close()
