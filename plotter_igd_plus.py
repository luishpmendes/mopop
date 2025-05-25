import csv
import matplotlib.pyplot as plt
import seaborn as sns
import ptitprince as pt
import os
from plotter_definitions import *

dirname = os.path.dirname(__file__)

min_igd_plus = 1.0
max_igd_plus = 0.0
for solver in solvers:
  filename = os.path.join(dirname, "igd_plus/" + solver + ".txt")
  with open(filename) as csv_file:
    data = csv.reader(csv_file)
    for row in data:
      min_igd_plus = min(min_igd_plus, float(row[0]))
      max_igd_plus = max(max_igd_plus, float(row[0]))
delta_igd_plus = max_igd_plus - min_igd_plus
min_igd_plus = max(min_igd_plus - round(0.025 * delta_igd_plus), 0.00)
max_igd_plus = min(max_igd_plus + round(0.025 * delta_igd_plus), 1.00)

plt.figure(figsize=(11, 11))
plt.xlabel("Normalized Modified Inverted Generational Distance",
           fontsize="x-large")
xs = []
for solver in solvers:
  filename = os.path.join(dirname, "igd_plus/" + solver + ".txt")
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
filename = os.path.join(dirname, "igd_plus/igd_plus.png")
plt.savefig(filename, format="png")
plt.close()

igd_plus = []

for solver in solvers:
  igd_plus.append([])

for i in range(len(solvers)):
  for seed in seeds:
    filename = os.path.join(
        dirname, "igd_plus/" + solvers[i] + "_" + str(seed) + ".txt")
    if os.path.exists(filename):
      with open(filename) as csv_file:
        data = csv.reader(csv_file, delimiter=",")
        for row in data:
          igd_plus[i].append(float(row[0]))
        csv_file.close()

plt.figure()
plt.xlabel(fontsize="large",
           xlabel="Normalized Modified Inverted Generational Distance")
plt.tick_params(axis="x", which="both", labelsize="large")
plt.grid(alpha=0.5, color="gray", linestyle="dashed",
         linewidth=0.5, which="both")
pt.half_violinplot(data=igd_plus, palette=colors,
                   orient="h", width=0.6, cut=0.0, inner=None)
sns.stripplot(data=igd_plus, palette=colors, orient="h", size=2, zorder=0)
sns.boxplot(data=igd_plus, orient="h", width=0.2, color="black", zorder=10, showcaps=True, boxprops={
            'facecolor': 'none', "zorder": 10}, showfliers=True, whiskerprops={'linewidth': 2, "zorder": 10}, flierprops={'markersize': 2})
plt.yticks(fontsize="large", ticks=list(range(len(solvers))),
           labels=[solver_labels[solver] for solver in solvers])
plt.tight_layout()
filename = os.path.join(dirname, "igd_plus/igd_plus.png")
plt.savefig(bbox_inches='tight', fname=filename, format="png")
filename = os.path.join(dirname, "igd_plus/igd_plus.pdf")
plt.savefig(bbox_inches='tight', fname=filename, format="pdf")
plt.close()
