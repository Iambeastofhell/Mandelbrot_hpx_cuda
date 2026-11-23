# Mandelbrot set project

This repository contains CPU implementations of a Mandelbrot set image generator for the course CS5658 GPU Programming.

## Authors

- Vishesh Srivastava 112301037
- Rajdeep 112301045
- Zeeshan Mohammed Rangrej 112301040
- Rajat Bhootra 112301044

## Team scope

This implementation is responsible for HPX task based CPU parallelism. The existing sequential implementation is retained as a comparison baseline.

- `mandelbrot_sequential` uses a normal nested CPU loop.
- `mandelbrot_hpx` divides the image rows between HPX tasks.
- Both programs use the same pixel mapping, color functions, PPM code, and command line options.

## Build

A C++17 compiler is required.

Build the sequential implementation:

```bash
make sequential
```

Build both implementations:

```bash
make
```

Build the HPX implementation:

```bash
make hpx
```

The HPX build requires an installed HPX development environment. The Makefile uses `hpxcxx` first. If `hpxcxx` is unavailable, it tries the `hpx_application` or `hpx` pkg-config package.

Remove generated executables and temporary test files:

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
| `--threads N` | HPX worker tasks or runtime workers | HPX default |
| `--save FILE` | Save the result as a PPM P6 image | None |
| `--load FILE` | Load a PPM P6 image instead of generating | None |
| `--display` | Open the result using the system image viewer | Off |
| `--help` | Print usage information | None |

The default color mode is black and white. Use only one of `--grayscale` or `--gradient`.

Width and height must be greater than zero. The minimum coordinate must be less than the maximum coordinate on both axes. Maximum iterations must be greater than zero.

## Sequential example

```bash
./mandelbrot_sequential \
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
./mandelbrot_hpx \
  --width 1920 --height 1080 \
  --xmin -2.0 --xmax 1.0 \
  --ymin -1.5 --ymax 1.5 \
  --maxiter 500 \
  --threads 8 \
  --gradient \
  --save mandelbrot_hpx.ppm \
  --display
```

Using `--threads 8` asks the HPX runtime to use eight workers and divides the image into eight row ranges. In the sequential program, `--threads` is accepted but does not change the single threaded calculation.

## Save and display combinations

Save only:

```bash
./mandelbrot_hpx --width 1024 --height 1024 --save output.ppm
```

Display only:

```bash
./mandelbrot_hpx --width 1024 --height 1024 --gradient --display
```

Save and display:

```bash
./mandelbrot_hpx --width 1024 --height 1024 --save output.ppm --display
```

Load and display:

```bash
./mandelbrot_sequential --load output.ppm --display
```

A PPM file can be loaded by both programs. Loading does not run Mandelbrot generation.

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

Benchmark instructions and the comparison table are in [benchmark/README.md](benchmark/README.md).
