import matplotlib
matplotlib.use("agg")
import matplotlib.pyplot as plt
import seaborn as sns
import ptitprince as pt
from plotter_definitions import *
from plotter_utils import path, read_column, report_missing

metrics_labels = ["Hypervolume Ratio",
                  "Normalized Modified Inverted Generational Distance"]
metrics_dirs = ["hvr", "nigd_plus"]


def collect(selected_instances):
  """Return metrics[metric][solver] pooled over the given instances."""
  return [[[value
            for instance in selected_instances
            for seed in seeds
            for value in read_column(
                path(directory,
                     instance + "_" + solver + "_" + str(seed) + ".txt"))]
           for solver in solvers]
          for directory in metrics_dirs]


def raincloud(metrics, filename):
  """One raincloud panel per metric, one row per solver."""
  fig, axs = plt.subplots(nrows=1, ncols=len(metrics),
                          figsize=(12.0 * len(metrics), 12.0), squeeze=False,
                          num=1, clear=True)
  fig.set_size_inches(12.0 * len(metrics), 12.0)
  for j in range(len(metrics)):
    axs[0][j].set_xlabel(xlabel=metrics_labels[j], fontsize="xx-large")
    pt.half_violinplot(data=metrics[j], ax=axs[0][j], palette=colors,
                       orient="h", width=0.6, cut=0.0, inner=None)
    sns.stripplot(data=metrics[j], ax=axs[0][j], palette=colors, orient="h",
                  size=2, zorder=0)
    sns.boxplot(data=metrics[j], ax=axs[0][j], orient="h", width=0.20,
                color="black", zorder=10, showcaps=True,
                boxprops={'facecolor': 'none', "zorder": 10}, showfliers=True,
                whiskerprops={'linewidth': 2, "zorder": 10},
                flierprops={'markersize': 2})
    axs[0][j].set_yticks(ticks=list(range(len(solvers))),
                         labels=[solver_labels[solver] for solver in solvers],
                         fontsize="x-large")
  plt.savefig(filename, format="png")


def scatter(metrics, filename):
  """Scatter matrix of every metric against every other, coloured by solver."""
  fig, axs = plt.subplots(nrows=len(metrics), ncols=len(metrics),
                          figsize=(5.0 * len(metrics), 5.0 * len(metrics)),
                          squeeze=False, num=1, clear=True)
  fig.set_size_inches(5.0 * len(metrics), 5.0 * len(metrics))
  for i, solver in enumerate(solvers):
    for j in range(len(metrics)):
      if not metrics[j][i]:
        continue
      axs[j][j].set_xlabel(xlabel=metrics_labels[j])
      axs[j][j].set_yticks([])
      axs[j][j].set_ylabel(ylabel="Density")
      sns.kdeplot(data=metrics[j][i], ax=axs[j][j], color=colors[i],
                  label=solver_labels[solver], marker=(i + 3, 2, 0), cut=0)
      axs[j][j].legend(loc="best")
      for k in range(len(metrics)):
        if j != k:
          axs[j][k].set_xlabel(xlabel=metrics_labels[k])
          axs[j][k].set_ylabel(ylabel=metrics_labels[j])
          axs[j][k].scatter(x=metrics[k][i], y=metrics[j][i], color=colors[i],
                            label=solver_labels[solver], marker=(i + 3, 2, 0),
                            alpha=0.60)
          axs[j][k].legend(loc="best")
  plt.savefig(filename, format="png")


# Pooled over every instance.
metrics = collect(instances)
if any(any(per_solver) for per_solver in metrics):
  raincloud(metrics, path("metrics", "raincloud.png"))
  scatter(metrics, path("metrics", "scatter.png"))

# One pair of figures per instance.
for instance in instances:
  metrics = collect([instance])
  if not any(any(per_solver) for per_solver in metrics):
    continue
  raincloud(metrics, path("metrics", "raincloud_" + instance + ".png"))
  scatter(metrics, path("metrics", "scatter_" + instance + ".png"))

report_missing("plotter_metrics.py")
