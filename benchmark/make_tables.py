#!/usr/bin/env python3
"""Turn the CSV written by run_benchmark.py into markdown tables."""

import csv
import statistics
import sys
from collections import defaultdict

CONFIG = ["suite", "width", "height", "maxiter", "color", "view"]


def load(path):
    times = defaultdict(list)
    with open(path) as file:
        for row in csv.DictReader(file):
            key = tuple(row[name] for name in CONFIG) + (row["impl"], row["threads"])
            times[key].append(float(row["seconds"]))
    return times


def print_suite(suite, times):
    print(f"### {suite}\n")
    print("| Size | Max iter | Color | View | Threads | Sequential (s) | HPX min (s) | HPX median (s) | HPX max (s) | Speedup | Efficiency |")
    print("|---|---|---|---|---|---|---|---|---|---|---|")
    for key in sorted(times, key=lambda k: (k[1:6], int(k[7]))):
        if key[0] != suite or key[6] != "hpx":
            continue
        _, width, height, maxiter, color, view, _, threads = key
        runs = times[key]
        sequential = statistics.median(times[key[:6] + ("sequential", "0")])
        median = statistics.median(runs)
        speedup = sequential / median
        print(f"| {width}x{height} | {maxiter} | {color} | {view} | {threads} | {sequential:.4f} | "
              f"{min(runs):.4f} | {median:.4f} | {max(runs):.4f} | {speedup:.2f}x | {speedup / int(threads):.0%} |")
    print()


def cpu_model():
    with open("/proc/cpuinfo") as file:
        return next(line.split(":", 1)[1].strip() for line in file if line.startswith("model name"))


def main():
    print(f"CPU: {cpu_model()}\n")
    times = load(sys.argv[1] if len(sys.argv) > 1 else "benchmark/results.csv")
    for suite in dict.fromkeys(key[0] for key in times):
        print_suite(suite, times)


if __name__ == "__main__":
    main()
