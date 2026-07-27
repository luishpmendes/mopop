import plotter_counts
from plotter_utils import report_missing

plotter_counts.plot("num_non_dominated_snapshots", "Non-dominated Solutions",
                    "num_non_dominated")

report_missing("plotter_num_non_dominated_snapshots.py")
