import plotter_counts
from plotter_utils import report_missing

plotter_counts.plot("num_fronts_snapshots", "Non-dominated Fronts",
                    "num_fronts")

report_missing("plotter_num_fronts_snapshots.py")
