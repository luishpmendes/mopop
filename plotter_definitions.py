import os

# Defaults describe the full experiment. run.sh exports the MOPOP_* variables so
# that a reduced profile (run_smoke.sh) plots exactly the runs it produced.
DEFAULT_INSTANCES = ("ibov_2011 ibov_2012 ibov_2013 ibov_2014 ibov_2015 "
                     "ibov_2016 ibov_2017 ibov_2018 ibov_2019 ibov_2020")
DEFAULT_SEEDS = ("305089489 511812191 608055156 467424509 944441939 "
                 "414977408 819312498 562386085 287613914 755772793")
DEFAULT_NUM_SNAPSHOTS = "30"

instances = os.environ.get("MOPOP_INSTANCES", DEFAULT_INSTANCES).split()
seeds = [int(seed)
         for seed in os.environ.get("MOPOP_SEEDS", DEFAULT_SEEDS).split()]
num_snapshots = int(os.environ.get("MOPOP_MAX_NUM_SNAPSHOTS",
                                   DEFAULT_NUM_SNAPSHOTS))

solvers = ["nsga2", "nspso", "moead", "mhaco", "ihs", "nsbrkga"]
solver_labels = {"nsga2": "NSGA-II",
                 "nspso": "NSPSO",
                 "moead": "MOEA/D-DE",
                 "mhaco": "MHACO",
                 "ihs": "IHS",
                 "nsbrkga": "NS-BRKGA"}
versions = ["best", "median"]
colors = ["#1f77b4", "#ff7f0e", "#2ca02c", "#d62728", "#9467bd", "#8c564b",
          "#e377c2", "#7f7f7f", "#bcbd22", "#17becf", "#8c7e6e", "#738191"]
colors2 = ["#103c5a", "#804007", "#165016", "#6b1414", "#4a345f", "#462b26",
           "#723c61", "#404040", "#5e5f11", "#0c5f68", "#463f37", "#3a4149"]
m = 4
