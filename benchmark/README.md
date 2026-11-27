# Benchmark

Compares the sequential program with the HPX program. Only the Mandelbrot generation is timed. Saving the image is not included.

## Running

```bash
make benchmark
```

This builds the programs, runs every case five times, and writes:

- `benchmark/results.csv`: one row per run
- `benchmark/results.md`: median, min and max per case, with speedup and efficiency

To run a single suite or change the number of runs:

```bash
python3 benchmark/run_benchmark.py --suite threads --runs 10 --out /tmp/threads.csv
python3 benchmark/make_tables.py /tmp/threads.csv
```

`--sequential` and `--hpx` take the paths of other executables, which makes it easy to compare a new variant against the baseline.

## Suites

| Suite | What changes | Fixed |
|---|---|---|
| resolution | 256x256 up to 3840x2160 | 500 iterations, gradient, all threads |
| threads | 1, 2, 4, ... up to the CPU count | 1920x1080, 500 iterations, gradient |
| iterations | 100, 500, 1000, 2000 | 1024x1024, gradient, all threads |
| color | black and white, grayscale, gradient | 1024x1024, 500 iterations, all threads |
| grain | pixels per HPX task, from 1 up to a static split | 1024x1024, 500 iterations, gradient, all threads |
| view | full set, zoomed region | 1024x1024, 500 iterations, gradient, all threads |

In the grain suite, the grain is the number of consecutive pixels in one task. A grain of 1024 on a 1024 wide image is one task per row, and the last grain is one equal block per thread. The table shows `row` when `--grain` is not given.

The full view is x from -2.0 to 1.0 and y from -1.5 to 1.5. The zoomed view is x from -1.35 to -1.2 and y from -0.1 to 0.05.

## Method

```text
Speedup    = sequential median / HPX median
Efficiency = speedup / HPX thread count
```

HPX refuses to start more worker threads than the machine has CPUs, so the thread counts stop at `nproc`.

Close other programs before a run. Browsers in particular add enough noise to change results by a factor of two.

## Results

The tables are in [results.md](results.md), generated from `results.csv`. The CPU model is printed at the top of that file.

Environment: Fedora 43, g++ 15.3.1 with `-O3 -std=c++17`, HPX 1.11.0 installed from the Fedora package.

## Correctness check

All programs must produce identical files:

```bash
make all cuda hpx-cuda hpx-cuda-opt
./build/mandelbrot_sequential --width 1920 --height 1080 --maxiter 500 --gradient --save /tmp/seq.ppm
./build/mandelbrot_hpx --width 1920 --height 1080 --maxiter 500 --gradient --save /tmp/hpx.ppm
./build/mandelbrot_cuda --width 1920 --height 1080 --maxiter 500 --gradient --save /tmp/cuda.ppm
./build/mandelbrot_hpx_cuda --width 1920 --height 1080 --maxiter 500 --gradient --save /tmp/hpx_cuda.ppm
./build/mandelbrot_hpx_cuda_opt --width 1920 --height 1080 --maxiter 500 --gradient --save /tmp/hpx_cuda_opt.ppm
cmp /tmp/seq.ppm /tmp/hpx.ppm
cmp /tmp/seq.ppm /tmp/cuda.ppm
cmp /tmp/seq.ppm /tmp/hpx_cuda.ppm
cmp /tmp/seq.ppm /tmp/hpx_cuda_opt.ppm
```
