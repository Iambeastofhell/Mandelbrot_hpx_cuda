# Mandelbrot set project

This repository contains a Mandelbrot set image generator for the course CS5658 GPU Programming.

## Authors

- Vishesh Srivastava 112301037
- Rajdeep 112301045
- Zeeshan Mohammed Rangrej 112301040
- Rajat Bhootra 112301044

## Scope

Implementation of HPX applications for Mandelbrot set generation, compared with other versions such as sequential and pure CUDA.

- `mandelbrot_sequential` uses a normal nested CPU loop.
- `mandelbrot_hpx` splits the image into HPX tasks, one per row by default.
- `mandelbrot_cuda` runs one CUDA thread per pixel.
- All programs use the same pixel mapping, colors, file formats, and command line options.

## Versions

| Version | Description | Compared with |
|---|---|---|
| 1 | Sequential CPU renderer, the reference for correctness and time | |
| 2 | HPX renderer with one task per pixel, row or fixed-size tile, and parallel loops with different chunk sizes | 1 |
| 3 | HPX renderer that checks tile borders to skip work, with a tree of futures | Best of 2 |
| 4a | Live explorer that draws tiles as they finish, cancels old work and recomputes everything | 3 |
| 4b | Live explorer that reuses earlier work | 4a |
| 5 | CUDA renderer that computes every pixel on the GPU | 3 |
| 6 | Version 3 with small non-uniform tiles computed by GPU kernels | 3 and 5 |

## Build

A C++17 compiler is required.

```bash
make              # sequential and HPX renderers
make sequential
make hpx
make cuda
make clean        # removes build/
```

The HPX build needs an installed HPX. It looks in `/usr` by default. The `hpx` submodule holds the HPX 1.11.0 source. For another install, such as a local build of it, pass its prefix with `make hpx HPX_PREFIX=/path/to/hpx`.

The CUDA build needs the CUDA toolkit (`nvcc`) and an NVIDIA GPU to run. It targets the installed GPU. To build for another one, set the architecture, for example `make cuda CUDA_ARCH=sm_80`.

## Run

```bash
./build/mandelbrot_sequential --save full.ppm
./build/mandelbrot_hpx --center -0.745 0.113 --view-width 0.01 --maxiter 2000 --threads 8 --save seahorse.ppm
./build/mandelbrot_cuda --width 1920 --height 1080 --save full.ppm
```

| Option | Description | Default |
|---|---|---|
| `--center X Y` | Point at the center of the image | -0.5 0 |
| `--view-width W` | Width of the view on the real axis | 3 |
| `--width N` | Image width in pixels | 1024 |
| `--height N` | Image height in pixels | 768 |
| `--maxiter N` | Iteration limit | 500 |
| `--color NAME` | `bw`, `gray` or `gradient` | `gradient` |
| `--save FILE` | Save the image as PPM | None |
| `--counts FILE` | Save the escape counts as raw 32-bit integers | None |
| `--threads N` | HPX worker threads, at most the number of CPUs | Chosen by HPX |
| `--grain N` | Pixels per HPX task, 0 for one row | 0 |
| `--help` | Print usage information | |

`--threads` and `--grain` only affect HPX. Other programs accept and ignore them.

Each program prints its generation time, which covers computing the escape counts. Coloring and saving are not included. The CUDA time includes allocating device memory and copying the results back, but not starting the CUDA runtime.

## Pixel grid

Pixels lie on a grid fixed to the complex plane. The spacing is the view width divided by the image width, the same on both axes, and the center is rounded to the nearest grid point. A point therefore has exactly the same value in every view with the same spacing.

The view width can be at most 16, and the spacing at least 1e-15, near the limit of `double`.

## Comparing results

The CPU and GPU programs share the pixel code in `src/mandelbrot.hpp`, and the Makefile stops both compilers from fusing multiplications and additions. The counts should be identical:

```bash
./build/mandelbrot_sequential --counts sequential.bin
./build/mandelbrot_hpx --counts hpx.bin
cmp sequential.bin hpx.bin
```

## Source layout

| File | Contents |
|---|---|
| `src/mandelbrot.hpp` | Pixel grid and escape count, used on the CPU and the GPU |
| `src/app.hpp`, `src/app.cpp` | Command line options, colors, timing and output files |
| `src/sequential.cpp` | Sequential renderer |
| `src/hpx.cpp` | HPX renderer |
| `src/cuda.cu` | CUDA renderer |
