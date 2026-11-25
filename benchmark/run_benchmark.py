#!/usr/bin/env python3
"""Run the Mandelbrot benchmarks and write one CSV row per run."""

import argparse
import csv
import os
import re
import subprocess

TIME_PATTERN = re.compile(r"generation time: (\S+) seconds")

MAX_THREADS = os.cpu_count()

FULL_VIEW = ("-2.0", "1.0", "-1.5", "1.5")
ZOOM_VIEW = ("-1.35", "-1.2", "-0.1", "0.05")

FIELDS = ["suite", "impl", "threads", "grain", "width", "height", "maxiter", "color", "view", "run", "seconds"]


def case(suite, impl, threads=0, size=(1920, 1080), maxiter=500, color="gradient", view="full", grain=0):
    return dict(suite=suite, impl=impl, threads=threads, grain=grain, width=size[0], height=size[1],
                maxiter=maxiter, color=color, view=view)


def suites():
    sizes = [(256, 256), (512, 512), (1024, 1024), (1920, 1080), (3840, 2160)]
    yield "resolution", [case("resolution", impl, threads, size)
                         for size in sizes for impl, threads in (("sequential", 0), ("hpx", MAX_THREADS))]

    yield "threads", [case("threads", "sequential")] + \
                     [case("threads", "hpx", n) for n in (1, 2, 4, 8, 16, 32) if n <= MAX_THREADS]

    yield "iterations", [case("iterations", impl, threads, (1024, 1024), maxiter)
                         for maxiter in (100, 500, 1000, 2000)
                         for impl, threads in (("sequential", 0), ("hpx", MAX_THREADS))]

    yield "color", [case("color", impl, threads, (1024, 1024), color=color)
                    for color in ("blackwhite", "grayscale", "gradient")
                    for impl, threads in (("sequential", 0), ("hpx", MAX_THREADS))]

    pixels = 1024 * 1024
    grains = sorted({1, 16, 256, 1024, 4096, 16384, 65536, pixels // MAX_THREADS})
    yield "grain", [case("grain", "sequential", size=(1024, 1024))] + \
                   [case("grain", "hpx", MAX_THREADS, (1024, 1024), grain=grain) for grain in grains]

    yield "view", [case("view", impl, threads, (1024, 1024), view=view)
                   for view in ("full", "zoom")
                   for impl, threads in (("sequential", 0), ("hpx", MAX_THREADS))]


def command(spec, binaries):
    xmin, xmax, ymin, ymax = FULL_VIEW if spec["view"] == "full" else ZOOM_VIEW
    args = [binaries[spec["impl"]],
            "--width", str(spec["width"]), "--height", str(spec["height"]),
            "--maxiter", str(spec["maxiter"]),
            "--xmin", xmin, "--xmax", xmax, "--ymin", ymin, "--ymax", ymax,
            "--save", "/dev/null"]
    if spec["color"] != "blackwhite":
        args.append("--" + spec["color"])
    if spec["impl"] == "hpx":
        args += ["--threads", str(spec["threads"])]
        if spec["grain"]:
            args += ["--grain", str(spec["grain"])]
    return args


def measure(args):
    output = subprocess.run(args, capture_output=True, text=True, check=True).stdout
    return float(TIME_PATTERN.search(output).group(1))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--runs", type=int, default=5, help="runs per case")
    parser.add_argument("--out", default="benchmark/results.csv")
    parser.add_argument("--sequential", default="build/mandelbrot_sequential")
    parser.add_argument("--hpx", default="build/mandelbrot_hpx")
    parser.add_argument("--suite", action="append", help="run only this suite (repeatable)")
    options = parser.parse_args()

    binaries = {"sequential": options.sequential, "hpx": options.hpx}

    with open(options.out, "w", newline="") as file:
        writer = csv.DictWriter(file, FIELDS)
        writer.writeheader()
        for name, cases in suites():
            if options.suite and name not in options.suite:
                continue
            print(name, flush=True)
            for spec in cases:
                args = command(spec, binaries)
                for run in range(options.runs):
                    writer.writerow(dict(spec, run=run, seconds=measure(args)))
                file.flush()


if __name__ == "__main__":
    main()
