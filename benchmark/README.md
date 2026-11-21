# Benchmark comparison

## Purpose

This benchmark compares the sequential CPU implementation with the HPX task based CPU implementation. It measures Mandelbrot generation only. PPM saving and opening the system viewer are not included in the generation time.

## Build

```bash
make sequential
make hpx
```

## Test environment

Record the following values before running the benchmark:

- CPU model
- CPU core and hardware thread count
- Available memory
- Operating system
- Compiler and version
- HPX version
- Optimization mode from the Makefile
- Whether the system is idle or shared

Example environment command:

```bash
uname -a
g++ --version
lscpu | grep -E 'Model name|Socket|Core|Thread'
```

## Comparison parameters

### Resolution

Test at least these resolutions:

- 256 by 256
- 512 by 512
- 1024 by 1024
- 1920 by 1080
- 3840 by 2160

### Maximum iterations

Test at least:

- 100
- 500
- 1000
- 2000

### HPX worker count

Test the HPX executable with:

- 1 thread
- 2 threads
- 4 threads
- 8 threads
- The full hardware thread count

For example:

```bash
./mandelbrot_hpx --width 1024 --height 1024 --maxiter 500 --threads 4 --save /dev/null
```

Saving to `/dev/null` may not produce a PPM, so normal benchmarks should save to a temporary file outside the generation timer. The generation timer excludes saving.

### Complex plane ranges

Use the full Mandelbrot view:

```text
xmin=-2.0
xmax=1.0
ymin=-1.5
ymax=1.5
```

A zoomed range should also be tested:

```text
xmin=-1.35
xmax=-1.20
ymin=-0.10
ymax=0.05
```

### Color modes

Run each implementation with:

- Black and white
- Grayscale
- Gradient

Color calculation should be excluded from timing only if the implementation is instrumented at the calculation stage. The current timing reports whole Mandelbrot image generation including color assignment.

## Timing procedure

Run each case five times and record:

- Minimum time
- Maximum time
- Mean time
- Median time
- Speedup
- Efficiency

The formulas are:

```text
Speedup = sequential mean / HPX mean
Efficiency = speedup / HPX worker count
```

Keep the same resolution, range, iteration count, and color mode for every sequential and HPX pair.

An example sequential run is:

```bash
./mandelbrot_sequential --width 1024 --height 1024 --maxiter 500 --save /tmp/sequential.ppm
```

An example HPX run is:

```bash
./mandelbrot_hpx --width 1024 --height 1024 --maxiter 500 --threads 8 --save /tmp/hpx.ppm
```

## Correctness check

Compare the sequential and HPX PPM files before comparing performance.

Both files must match:

- Width and height
- PPM header
- Pixel count
- RGB values
- File size
- Output file hash

Example hash comparison:

```bash
sha256sum /tmp/sequential.ppm /tmp/hpx.ppm
cmp /tmp/sequential.ppm /tmp/hpx.ppm
```

## Results table

Fill this table after running benchmarks on the target machine.

| Resolution | Iterations | HPX threads | Sequential mean | HPX mean | Speedup | Efficiency |
|---|---:|---:|---:|---:|---:|---:|
| 256 by 256 | 100 | 1 | Not measured | Not measured | Not measured | Not measured |
| 256 by 256 | 100 | 2 | Not measured | Not measured | Not measured | Not measured |
| 512 by 512 | 500 | 4 | Not measured | Not measured | Not measured | Not measured |
| 1024 by 1024 | 500 | 8 | Not measured | Not measured | Not measured | Not measured |
| 1920 by 1080 | 500 | 8 | Not measured | Not measured | Not measured | Not measured |
| 3840 by 2160 | 1000 | 8 | Not measured | Not measured | Not measured | Not measured |
| 1024 by 1024 | 1000 | Hardware count | Not measured | Not measured | Not measured | Not measured |
