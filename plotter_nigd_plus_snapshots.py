import matplotlib
matplotlib.use("agg")
import matplotlib.pyplot as plt
from matplotlib.ticker import FormatStrFormatter
import seaborn as sns
import ptitprince as pt
from functools import partial
import numpy as np
import pandas as pd
from plotter_definitions import *
from plotter_utils import (load_snapshots, mean, path, quartiles,
                           report_missing, stdev)

LABEL = "Normalized Modified Inverted Generational Distance"

time_per_solver, nigd_per_solver, nigd_per_snapshot = load_snapshots(
    "nigd_plus_snapshots")

# Mean over time, one curve per solver.
plt.figure()
plt.xlabel(fontsize="large", xlabel="Time (s)")
plt.ylabel(fontsize="large", ylabel=LABEL)
plt.tick_params(axis="both", which="both", labelsize="large")
plt.grid(alpha=0.5, color='gray', linestyle='dashed',
         linewidth=0.5, which='both')
for i, solver in enumerate(solvers):
  x = [mean(time_per_solver[solver][j]) for j in range(num_snapshots)]
  y = [mean(nigd_per_solver[solver][j]) for j in range(num_snapshots)]
  plt.plot(x, y, label=solver_labels[solver], marker=(i + 3, 2, 0),
           color=colors[i], alpha=0.80)
plt.xscale("log")
plt.yscale("function", functions=(partial(np.power, 10.0), np.log10))
plt.legend(fontsize="large", loc="best")
plt.gca().xaxis.set_major_formatter(FormatStrFormatter('%.1f'))
plt.gca().yaxis.set_major_formatter(FormatStrFormatter('%.2f'))
plt.gca().yaxis.set_minor_formatter(FormatStrFormatter('%.2f'))
plt.tight_layout()
for extension in ("png", "pdf"):
  plt.savefig(bbox_inches='tight', format=extension,
              fname=path("nigd_plus_snapshots",
                         "nigd_plus_mean_snapshots." + extension))
plt.close()

# The same series as a table, written once rather than once per solver.
data = {'Solver': [], 'Time': [], 'NIGD+': [], 'Standard Deviation': []}
for solver in solvers:
  for j in range(num_snapshots):
    data['Solver'].append(solver)
    data['Time'].append(mean(time_per_solver[solver][j]))
    data['NIGD+'].append(mean(nigd_per_solver[solver][j]))
    data['Standard Deviation'].append(stdev(nigd_per_solver[solver][j]))
pd.DataFrame(data).to_csv(path("nigd_plus_snapshots", "nigd_plus_data.csv"),
                          index=False)

# Median with an interquartile band, one curve per solver.
plt.figure()
plt.xlabel(fontsize="large", xlabel="Time (s)")
plt.ylabel(fontsize="large", ylabel=LABEL)
plt.tick_params(axis="both", which="both", labelsize="large")
plt.grid(alpha=0.5, color='gray', linestyle='dashed',
         linewidth=0.5, which='both')
for i, solver in enumerate(solvers):
  x = [mean(time_per_solver[solver][j]) for j in range(num_snapshots)]
  q = [quartiles(nigd_per_solver[solver][j]) for j in range(num_snapshots)]
  plt.fill_between(x, [value[0] for value in q], [value[2] for value in q],
                   color=colors[i], alpha=0.25)
for i, solver in enumerate(solvers):
  x = [mean(time_per_solver[solver][j]) for j in range(num_snapshots)]
  y = [quartiles(nigd_per_solver[solver][j])[1] for j in range(num_snapshots)]
  plt.plot(x, y, label=solver_labels[solver], marker=(i + 3, 2, 0),
           color=colors[i], alpha=0.75)
plt.xscale("log")
plt.yscale("function", functions=(partial(np.power, 10.0), np.log10))
plt.legend(fontsize="large", loc="best")
plt.gca().xaxis.set_major_formatter(FormatStrFormatter('%.1f'))
plt.gca().yaxis.set_major_formatter(FormatStrFormatter('%.2f'))
plt.gca().yaxis.set_minor_formatter(FormatStrFormatter('%.2f'))
plt.tight_layout()
for extension in ("png", "pdf"):
  plt.savefig(bbox_inches='tight', format=extension,
              fname=path("nigd_plus_snapshots",
                         "nigd_plus_quartiles_snapshots." + extension))
plt.close()

# One raincloud frame per snapshot; run.sh stitches these into nigd_plus.mp4.
for snapshot in range(num_snapshots):
  if not any(nigd_per_snapshot[snapshot]):
    continue
  plt.figure(figsize=(11, 11))
  plt.title("Multi-Objective Portfolio Optimization Problem",
            fontsize="xx-large")
  plt.xlabel(LABEL, fontsize="x-large")
  pt.half_violinplot(data=nigd_per_snapshot[snapshot], palette=colors,
                     orient="h", width=0.6, cut=0.0, inner=None)
  sns.stripplot(data=nigd_per_snapshot[snapshot], palette=colors, orient="h",
                size=2, zorder=0)
  sns.boxplot(data=nigd_per_snapshot[snapshot], orient="h", width=0.20,
              color="black", zorder=10, showcaps=True,
              boxprops={'facecolor': 'none', "zorder": 10}, showfliers=True,
              whiskerprops={'linewidth': 2, "zorder": 10},
              flierprops={'markersize': 2})
  plt.xlim(left=0.0, right=1.0)
  plt.xticks(ticks=[0.0, 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0],
             fontsize="large")
  plt.yticks(ticks=list(range(len(solvers))),
             labels=[solver_labels[solver] for solver in solvers],
             fontsize="large")
  plt.savefig(path("nigd_plus_snapshots",
                   "snapshot_" + str(snapshot) + ".png"), format="png")
  plt.close()

report_missing("plotter_nigd_plus_snapshots.py")
