import matplotlib
matplotlib.use("agg")
import matplotlib.pyplot as plt
import seaborn as sns
import ptitprince as pt
from plotter_definitions import *
from plotter_utils import path, read_column, report_missing


def raincloud(xs, filename, formats=("png",)):
  """Draw one raincloud row per solver and save it in every given format."""
  if not any(xs):
    return
  plt.figure(figsize=(11, 11))
  plt.xlabel("Hypervolume Ratio", fontsize="x-large")
  plt.tick_params(axis="x", which="both", labelsize="large")
  plt.grid(alpha=0.5, color="gray", linestyle="dashed",
           linewidth=0.5, which="both")
  pt.half_violinplot(data=xs, palette=colors, orient="h",
                     width=0.6, cut=0.0, inner=None)
  sns.stripplot(data=xs, palette=colors, orient="h", size=2, zorder=0)
  sns.boxplot(data=xs, orient="h", width=0.20, color="black", zorder=10,
              showcaps=True, boxprops={'facecolor': 'none', "zorder": 10},
              showfliers=True, whiskerprops={'linewidth': 2, "zorder": 10},
              flierprops={'markersize': 2})
  plt.yticks(ticks=list(range(len(solvers))),
             labels=[solver_labels[solver] for solver in solvers],
             fontsize="large")
  plt.tight_layout()
  for extension in formats:
    plt.savefig(bbox_inches='tight', fname=filename + "." + extension,
                format=extension)
  plt.close()


# One raincloud per instance, from the per-solver aggregated files.
for instance in instances:
  xs = [read_column(path("hvr", instance + "_" + solver + ".txt"))
        for solver in solvers]
  raincloud(xs, path("hvr", instance))

# One raincloud pooling every instance, solver and seed.
xs = []
for solver in solvers:
  x = []
  for instance in instances:
    for seed in seeds:
      x.extend(read_column(
          path("hvr", instance + "_" + solver + "_" + str(seed) + ".txt")))
  xs.append(x)
raincloud(xs, path("hvr", "hvr"), formats=("png", "pdf"))

report_missing("plotter_hvr.py")
