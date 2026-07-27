import plotter_counts
from plotter_utils import report_missing

# NS-BRKGA is the only solver that records an elite set.
plotter_counts.plot("num_elites_snapshots", "Number of elites", "num_elites",
                    only_solvers=["nsbrkga"])

report_missing("plotter_num_elites_snapshots.py")
