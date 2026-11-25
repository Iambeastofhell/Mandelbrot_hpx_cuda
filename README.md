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
- All programs use the same pixel mapping, colors, PPM format, and command line options.

## Build

A C++17 compiler is required.

Build the sequential implementation:

```bash
make sequential
```

Build the CPU implementations:

```bash
make
```

Build the HPX implementation:

```bash
make hpx
```

The HPX build needs an installed HPX. It looks in `/usr` by default. For another install, such as a local build of the submodule, pass its prefix:

```bash
make hpx HPX_PREFIX=$PWD/hpx/install
```

Build the CUDA implementation:

```bash
make cuda
```

This needs the CUDA toolkit (`nvcc`) and an NVIDIA GPU to run. The Makefile targets the installed GPU. To build for another one, set the architecture, for example `make cuda CUDA_ARCH=sm_80`. It is not part of `make all`.

Remove generated executables, temporary test files, and the `output` directory:

```bash
make clean
```

## Inputs

| Option | Description | Default |
|---|---|---|
| `--width W` | Output image width | 80 |
| `--height H` | Output image height | 40 |
| `--xmin X` | Minimum real coordinate | -2.0 |
| `--xmax X` | Maximum real coordinate | 1.0 |
| `--ymin Y` | Minimum imaginary coordinate | -1.5 |
| `--ymax Y` | Maximum imaginary coordinate | 1.5 |
| `--maxiter N` | Maximum escape iterations | 100 |
| `--grayscale` | Use a grayscale color scheme | Off |
| `--gradient` | Use a gradient color scheme | Off |
| `--threads N` | HPX worker threads | HPX default |
| `--grain N` | Pixels per HPX task, zero means one row | 0 |
| `--save FILE` | Save the result as a PPM P6 image | None |
| `--load FILE` | Load a PPM P6 image instead of generating | None |
| `--display` | Open the result using the system image viewer | Off |
| `--help` | Print usage information | None |

The default color mode is black and white. Use only one of `--grayscale` or `--gradient`.

Width and height must be greater than zero. The minimum coordinate must be less than the maximum coordinate on both axes. Maximum iterations must be greater than zero.

`--threads` and `--grain` only affect HPX. Other programs accept and ignore them.

## Sequential example

```bash
./build/mandelbrot_sequential \
  --width 1920 --height 1080 \
  --xmin -2.0 --xmax 1.0 \
  --ymin -1.5 --ymax 1.5 \
  --maxiter 500 \
  --gradient \
  --save mandelbrot_sequential.ppm \
  --display
```

## HPX example

```bash
./build/mandelbrot_hpx \
  --width 1920 --height 1080 \
  --xmin -2.0 --xmax 1.0 \
  --ymin -1.5 --ymax 1.5 \
  --maxiter 500 \
  --threads 8 \
  --gradient \
  --save mandelbrot_hpx.ppm \
  --display
```

Using `--threads 8` asks the HPX runtime to use eight worker threads. The workers take tasks from a shared pool as they become free. `--grain N` changes how many pixels each task covers. HPX does not allow more threads than the machine has CPUs.

## CUDA example

```bash
./build/mandelbrot_cuda --width 1920 --height 1080 --maxiter 500 --gradient --save mandelbrot_cuda.ppm
```

It prints two times. The kernel time covers only the GPU computation. The generation time also includes allocating device memory and copying the image back to the host, which makes it the one to compare with the CPU programs. Starting the CUDA runtime is not counted in either.

The pixel code is in `mandelbrot_kernel.cuh`. It repeats the CPU code in `mandelbrot_common.cpp`, and the Makefile compiles it with `--fmad=false` so the GPU does not fuse multiplications and additions. With that, the image should be identical to the CPU one:

```bash
cmp output/mandelbrot_cuda.ppm output/mandelbrot_seq.ppm
```

## Save and display combinations

Save only:

```bash
./build/mandelbrot_hpx --width 1024 --height 1024 --save output.ppm
```

Display only:

```bash
./build/mandelbrot_hpx --width 1024 --height 1024 --gradient --display
```

Save and display:

```bash
./build/mandelbrot_hpx --width 1024 --height 1024 --save output.ppm --display
```

Load and display:

```bash
./build/mandelbrot_sequential --load output.ppm --display
```

Any of the programs can load a PPM file. Loading does not run Mandelbrot generation.

## Shared code

Shared types and operations are in `mandelbrot_common.hpp` and `mandelbrot_common.cpp`.

They include:

- `Pixel` and `Image`
- Escape iteration calculation
- Black and white, grayscale, and gradient colors
- Command line parsing and validation
- PPM loading and saving
- Image display handling

## Benchmark

Benchmark instructions are in [benchmark/README.md](benchmark/README.md). The CUDA results from the cluster are in [benchmark/cuda_results.md](benchmark/cuda_results.md).

