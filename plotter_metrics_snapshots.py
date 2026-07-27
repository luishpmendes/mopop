import matplotlib
matplotlib.use("agg")
import matplotlib.pyplot as plt
import seaborn as sns
import ptitprince as pt
from plotter_definitions import *
from plotter_utils import load_snapshots, path, report_missing

metrics_labels = ["Hypervolume Ratio",
                  "Normalized Modified Inverted Generational Distance"]
TICKS = [0.0, 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0]

# metrics_per_snapshot[metric][snapshot][solver], pooled over instances/seeds.
metrics_per_snapshot = [load_snapshots("hvr_snapshots")[2],
                        load_snapshots("nigd_plus_snapshots")[2]]

for snapshot in range(num_snapshots):
  if not any(any(metric[snapshot]) for metric in metrics_per_snapshot):
    continue

  fig, axs = plt.subplots(1, ncols=len(metrics_per_snapshot),
                          figsize=(12.0 * len(metrics_per_snapshot), 12.0),
                          squeeze=False, num=1, clear=True)
  fig.suptitle("Multi-Objective Portfolio Optimization Problem", fontsize=42)
  for j in range(len(metrics_per_snapshot)):
    axs[0][j].set_xlabel(xlabel=metrics_labels[j], fontsize="xx-large")
    pt.half_violinplot(data=metrics_per_snapshot[j][snapshot], ax=axs[0][j],
                       palette=colors, orient="h", width=0.6, cut=0.0,
                       inner=None)
    sns.stripplot(data=metrics_per_snapshot[j][snapshot], ax=axs[0][j],
                  palette=colors, orient="h", size=2, zorder=0)
    sns.boxplot(data=metrics_per_snapshot[j][snapshot], ax=axs[0][j],
                orient="h", width=0.20, color="black", zorder=10,
                showcaps=True, boxprops={'facecolor': 'none', "zorder": 10},
                showfliers=True, whiskerprops={'linewidth': 2, "zorder": 10},
                flierprops={'markersize': 2})
    axs[0][j].set_xticks(ticks=TICKS, labels=TICKS, fontsize="x-large")
    axs[0][j].set_yticks(ticks=list(range(len(solvers))),
                         labels=[solver_labels[solver] for solver in solvers],
                         fontsize="x-large")
  plt.savefig(path("metrics_snapshots", "raincloud_" + str(snapshot) + ".png"),
              format="png")

  fig, axs = plt.subplots(nrows=len(metrics_per_snapshot),
                          ncols=len(metrics_per_snapshot),
                          figsize=(8.0 * len(metrics_per_snapshot),
                                   8.0 * len(metrics_per_snapshot)),
                          squeeze=False, num=1, clear=True)
  fig.set_size_inches(8.0 * len(metrics_per_snapshot),
                      8.0 * len(metrics_per_snapshot))
  fig.suptitle("Multi-Objective Portfolio Optimization Problem", fontsize=36)
  for i, solver in enumerate(solvers):
    for j in range(len(metrics_per_snapshot)):
      if not metrics_per_snapshot[j][snapshot][i]:
        continue
      axs[j][j].set_xlabel(xlabel=metrics_labels[j], fontsize="xx-large")
      axs[j][j].set_ylabel(ylabel="Density", fontsize="xx-large")
      axs[j][j].set_xticks(ticks=TICKS, labels=TICKS, fontsize="x-large")
      axs[j][j].set_yticks([])
      sns.kdeplot(data=metrics_per_snapshot[j][snapshot][i], ax=axs[j][j],
                  color=colors[i], label=solver_labels[solver],
                  marker=(i + 3, 2, 0), cut=0)
      axs[j][j].legend(loc="best", fontsize="x-large")
      for k in range(len(metrics_per_snapshot)):
        if j != k:
          axs[j][k].set_xlabel(xlabel=metrics_labels[k], fontsize="xx-large")
          axs[j][k].set_ylabel(ylabel=metrics_labels[j], fontsize="xx-large")
          axs[j][k].set_xticks(ticks=TICKS, labels=TICKS, fontsize="x-large")
          axs[j][k].set_yticks(ticks=TICKS, labels=TICKS, fontsize="x-large")
          axs[j][k].scatter(x=metrics_per_snapshot[k][snapshot][i],
                            y=metrics_per_snapshot[j][snapshot][i],
                            color=colors[i], label=solver_labels[solver],
                            marker=(i + 3, 2, 0), alpha=0.60)
          axs[j][k].legend(loc="best", fontsize="x-large")
  plt.savefig(path("metrics_snapshots", "scatter_" + str(snapshot) + ".png"),
              format="png")

report_missing("plotter_metrics_snapshots.py")
