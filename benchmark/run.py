#!/usr/bin/env python3
"""Run the benchmarks and write results.csv and results.md."""

import argparse
import array
import csv
import os
import re
import statistics
import subprocess
import tempfile
from pathlib import Path

VIEWS = {
    "full": ["--center", "-0.5", "0", "--view-width", "3", "--maxiter", "1000"],
    "interior": ["--center", "-0.2", "0", "--view-width", "1.2", "--maxiter", "2000"],
    "edges": ["--center", "-0.743643887", "0.131825904", "--view-width", "0.002", "--maxiter", "1500"],
}

# The methods compared on every view. Set their sizes from the results of the tune suite.
METHODS = [
    ["--method", "row"],
    ["--method", "tile", "--tile", "64"],
    ["--method", "loop", "--chunk", "0"],
    ["--method", "border", "--tile", "64", "--min-tile", "16"],
]

TASKS = "/count/cumulative"
IDLE = "/idle-rate"
COLUMNS = ["suite", "view", "program", "options", "run", "seconds", "tasks", "idle", "tiles", "mismatched"]


def suites(gpu):
    """Yields each suite as a name and a list of (program, view, options) cases."""
    methods = [["--method", "pixel"], ["--method", "row"]]
    methods += [["--method", "tile", "--tile", str(tile)] for tile in (16, 32, 64, 128, 256)]
    methods += [["--method", "loop", "--chunk", str(chunk)] for chunk in (0, 64, 512, 4096, 32768)]
    methods += [["--method", "border", "--tile", str(tile), "--min-tile", str(smallest)]
                for tile in (32, 64, 128, 256) for smallest in (8, 16, 32)]
    yield "tune", [("sequential", "full", [])] + [("hpx", "full", options) for options in methods]

    cases = []
    for view in VIEWS:
        cases.append(("sequential", view, []))
        cases += [("hpx", view, options) for options in METHODS]
        if gpu:
            cases.append(("cuda", view, []))
    yield "views", cases

    cpus = os.cpu_count()
    counts = [n for n in (1, 2, 4, 8, 16, 32, 64) if n < cpus] + [cpus]
    yield "threads", [("sequential", "full", [])] + \
        [("hpx", "full", ["--method", method, "--threads", str(n)]) for method in ("row", "border") for n in counts]

    yield "explorer", [("explorer", view, ["--pans", "32", *reuse])
                       for view in ("full", "edges") for reuse in ([], ["--no-reuse"])]


class Benchmark:
    def __init__(self, build, size, scratch):
        self.build = build
        self.size = ["--width", str(size[0]), "--height", str(size[1])]
        self.output = scratch / "counts.bin"
        self.references = {}
        self.counters = [name for name in (TASKS, IDLE) if self.has_counter(name)]

    def execute(self, program, args):
        command = [self.build / f"mandelbrot_{program}", *self.size, *args]
        return subprocess.run(command, capture_output=True, text=True, check=True).stdout

    def works(self, program, args=()):
        try:
            self.execute(program, ["--width", "16", "--height", "16", *args])
        except (OSError, subprocess.CalledProcessError):
            return False
        return True

    def has_counter(self, name):
        return self.works("hpx", [counter_option(name)])

    def counts(self):
        values = array.array("i")
        values.frombytes(self.output.read_bytes())
        return values

    def reference(self, view):
        """The counts of the sequential program, against which the others are checked."""
        if view not in self.references:
            self.execute("sequential", [*VIEWS[view], "--counts", self.output])
            self.references[view] = self.counts()
        return self.references[view]

    def measure(self, program, view, options, runs):
        args = [*VIEWS[view], *options]
        if program in ("hpx", "explorer") and "--threads" not in options:
            args += ["--threads", str(os.cpu_count())]
        if program == "hpx":
            args += [counter_option(name) for name in self.counters]

        reference = self.reference(view)
        mismatched = ""
        for run in range(runs):
            check = ["--counts", self.output] if run == 0 else []
            text = self.execute(program, [*args, *check])
            if check:
                mismatched = sum(a != b for a, b in zip(reference, self.counts()))
            idle = counter_value(text, IDLE)
            yield {
                "run": run,
                "seconds": float(re.search(r"generation time: (\S+)", text).group(1)),
                "tasks": counter_value(text, TASKS),
                "idle": float(idle) / 100 if idle else "",
                "tiles": found(r"Tiles computed: (\d+)", text),
                "mismatched": mismatched,
            }


def counter_option(name):
    return "--hpx:print-counter=/threads{locality#0/total}" + name


def counter_value(text, name):
    return found(r"\{locality#0/total/total\}" + name + r",[^\[]*\[s\],([\d.]+)", text)


def found(pattern, text):
    match = re.search(pattern, text)
    return match.group(1) if match else ""


def table(rows):
    """One line per case with the median over its runs, leaving out columns that are empty."""
    cases = {}
    for row in rows:
        cases.setdefault((row["view"], row["program"], row["options"]), []).append(row)
    sequential = {view: statistics.median(r["seconds"] for r in runs)
                  for (view, program, _), runs in cases.items() if program == "sequential"}

    lines = []
    for (view, program, options), runs in cases.items():
        times = [r["seconds"] for r in runs]
        median = statistics.median(times)
        speedup = f"{sequential[view] / median:.2f}" if view in sequential else ""
        lines.append({
            "View": view,
            "Program": program,
            "Options": options,
            "Median (s)": f"{median:.4f}",
            "Min (s)": f"{min(times):.4f}",
            "Max (s)": f"{max(times):.4f}",
            "Speedup": speedup,
            "Mismatched": runs[0]["mismatched"],
            "HPX threads": runs[0]["tasks"],
            "Idle (%)": f"{statistics.median(r['idle'] for r in runs):.1f}" if runs[0]["idle"] != "" else "",
            "Tiles": runs[0]["tiles"],
        })

    names = [name for name in lines[0] if any(line[name] != "" for line in lines)]
    text = ["| " + " | ".join(names) + " |", "|" + "---|" * len(names)]
    text += ["| " + " | ".join(str(line[name]) for name in names) + " |" for line in lines]
    return "\n".join(text)


def machine(gpu):
    with open("/proc/cpuinfo") as file:
        cpu = next(line.split(":", 1)[1].strip() for line in file if line.startswith("model name"))
    lines = [f"- CPU: {cpu}, {os.cpu_count()} threads"]
    if gpu:
        query = ["nvidia-smi", "--query-gpu=name", "--format=csv,noheader"]
        names = subprocess.run(query, capture_output=True, text=True, check=True).stdout.splitlines()
        lines.append(f"- GPU: {names[0]}")
    return "\n".join(lines)


def main():
    here = Path(__file__).resolve().parent
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--suite", action="append", help="run only this suite (repeatable)")
    parser.add_argument("--runs", type=int, default=5, help="runs per case")
    parser.add_argument("--size", type=int, nargs=2, default=(1920, 1080), metavar=("W", "H"))
    parser.add_argument("--build", type=Path, default=here.parent / "build")
    parser.add_argument("--out", type=Path, default=here, help="directory for the results")
    options = parser.parse_args()

    with tempfile.TemporaryDirectory() as scratch:
        benchmark = Benchmark(options.build.resolve(), options.size, Path(scratch))
        gpu = benchmark.works("cuda")
        rows = []
        for suite, cases in suites(gpu):
            if options.suite and suite not in options.suite:
                continue
            print(suite, flush=True)
            for program, view, case_options in cases:
                for result in benchmark.measure(program, view, case_options, options.runs):
                    rows.append(dict(result, suite=suite, view=view, program=program,
                                     options=" ".join(case_options)))

    options.out.mkdir(parents=True, exist_ok=True)
    with open(options.out / "results.csv", "w", newline="") as file:
        writer = csv.DictWriter(file, COLUMNS)
        writer.writeheader()
        writer.writerows(rows)

    sections = [f"# Results\n\n{machine(gpu)}\n- Image size: {options.size[0]}x{options.size[1]}"]
    for suite in dict.fromkeys(row["suite"] for row in rows):
        sections.append(f"## {suite}\n\n" + table([row for row in rows if row["suite"] == suite]))
    (options.out / "results.md").write_text("\n\n".join(sections) + "\n")


if __name__ == "__main__":
    main()
