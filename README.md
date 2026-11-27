# Mandelbrot set project

This repository contains a Mandelbrot set image generator for the course CS5658 GPU Programming.

## Authors

- Rajdeep 112301045
- Zeeshan Mohammed Rangrej 112301040
- Vishesh Srivastava 112301037
- Rajat Bhootra 112301044

## Scope

Implementation of HPX applications for Mandelbrot set generation, compared with other versions such as sequential and pure CUDA.

- `mandelbrot_sequential` uses a normal nested CPU loop.
- `mandelbrot_hpx` splits the image into HPX tasks by one of the methods below.
- `mandelbrot_cuda` runs one CUDA thread per pixel.
- `mandelbrot_explorer` opens a window that can be panned and zoomed.
- All programs use the same pixel mapping, colors, file formats, and command line options.

## Versions

| Version | Description | Program | Compared with |
|---|---|---|---|
| 1 | Sequential CPU renderer, the reference for correctness and time | `mandelbrot_sequential` | |
| 2 | HPX renderer with one task per pixel, row or fixed-size tile, and parallel loops with different chunk sizes | `mandelbrot_hpx` | 1 |
| 3 | HPX renderer that checks tile borders to skip work, with a tree of futures | `mandelbrot_hpx --method border` | Best of 2 |
| 4a | Live explorer that draws tiles as they finish, cancels old work and recomputes everything | `mandelbrot_explorer --no-reuse` | 3 |
| 4b | Live explorer that reuses earlier work | `mandelbrot_explorer` | 4a |
| 5 | CUDA renderer that computes every pixel on the GPU | `mandelbrot_cuda` | 3 |
| 6 | Version 3 with small non-uniform tiles computed by GPU kernels | Not yet | 3 and 5 |

## Build

A C++17 compiler is required.

```bash
make              # sequential and HPX renderers
make sequential
make hpx
make cuda
make explorer
make clean        # removes build/
```

The HPX and explorer builds need an installed HPX, looked for in `/usr` by default. The `hpx` submodule holds the HPX 1.11.0 source. For another install, such as a local build of it, pass its prefix with `make hpx HPX_PREFIX=/path/to/hpx`.

The explorer also needs the SDL2 development files.

The CUDA build needs the CUDA toolkit (`nvcc`) and an NVIDIA GPU to run. It targets the installed GPU. To build for another one, set the architecture, for example `make cuda CUDA_ARCH=sm_80`.

## Run

```bash
./build/mandelbrot_sequential --save full.ppm
./build/mandelbrot_hpx --method tile --center -0.745 0.113 --view-width 0.01 --maxiter 2000 --save seahorse.ppm
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
| `--threads N` | HPX worker threads, at most the number of CPUs | One per core |
| `--method NAME` | HPX method: `pixel`, `row`, `tile`, `loop` or `border` | `row` |
| `--tile N` | Side of a tile in pixels, at most 4096 | 64 |
| `--min-tile N` | Smallest tile the border method splits, at least 3 | 16 |
| `--chunk N` | Pixels per loop chunk, 0 lets HPX choose | 0 |
| `--pans N` | Explorer: pan N times without a window, then exit | 0 |
| `--no-reuse` | Explorer: compute every tile again for each view | |
| `--hpx:OPTION` | HPX renderer: passed on to HPX, for example `--hpx:print-bind` | |
| `--help` | Print usage information | |

`--threads`, `--method`, `--tile`, `--min-tile`, `--chunk`, `--pans` and `--no-reuse` only affect the programs that use them. Other programs accept and ignore them.

The renderers print their generation time, which covers computing the escape counts. Coloring and saving are not included. The CUDA time includes allocating device memory and copying the results back, but not starting the CUDA runtime.

## HPX methods

| Method | Work given to each task |
|---|---|
| `pixel` | One pixel |
| `row` | One row |
| `tile` | One square tile of the pixel grid |
| `loop` | A chunk of a parallel loop over all pixels |
| `border` | One tile, which creates more tasks where needed |

`pixel` creates one task for every pixel, which takes about 500 MB of memory at 1920x1080.

## Border method

`--method border` uses the Mariani-Silver method on tiles of `--tile` pixels. For each tile:

1. A tile narrower or shorter than `--min-tile` has every pixel computed.
2. Otherwise its one pixel border is computed.
3. If every border pixel reaches the iteration limit, the inside is filled with the limit.
4. If every border pixel has the same count below the limit, and the tile does not contain the point 0, the inside is filled with that count.
5. Otherwise the inside is split into four tiles, each a new task. The future of the tile is ready when the futures of the four are (`hpx::when_all`).

Every pixel belongs to one tile, so tasks never write the same pixel.

The method is not exact, since detail thinner than a pixel can pass between border pixels. Comparing with the counts of the sequential program gives the number of mismatched pixels.

## Explorer

```bash
./build/mandelbrot_explorer --width 1280 --height 720
```

| Input | Action |
|---|---|
| Drag | Pan |
| Scroll | Zoom around the cursor |
| Up, Down | Double or halve the iteration limit |
| `c` | Change the color |
| `r` | Return to the starting view |
| `q`, Escape | Quit |

Each tile of `--tile` pixels is computed with the border method and drawn as soon as it finishes, which its tasks report through an HPX channel. Until a tile is ready, the old picture, moved or scaled to the new view, stands in for it.

The explorer keeps a cache of tiles, each with the future of its computation, so that no tile is started twice:

- Finished tiles stay in the cache after they leave the view.
- A tile that leaves the view before it finishes is cancelled with its stop token and removed. Tiles still in view keep running.
- A change of zoom or iteration limit cancels and removes every tile.
- The cache holds at most four screens of tiles and drops those out of view the longest.
- Only the main thread uses the cache, so it needs no lock.

With `--no-reuse`, every new view cancels the old tasks and computes every tile again.

Zooming in stops near the limit of `double`, and zooming out at a view width of 8.

The title of the window shows the view as options for the other programs, and the time the view took.

`--pans N` pans around a square N times without opening a window, then prints the time and the number of tiles computed. `--save` and `--counts` then write the final view.

## Pixel grid

Pixels lie on a grid fixed to the complex plane. The spacing is the view width divided by the image width, the same on both axes, and the center is rounded to the nearest grid point. A point therefore has exactly the same value in every view with the same spacing.

The view width can be at most 16, and the spacing at least 1e-15, near the limit of `double`.

## Comparing results

The CPU and GPU programs share the pixel code in `src/mandelbrot.hpp`, and the Makefile stops both compilers from fusing multiplications and additions. Apart from the border method, the counts should be identical:

```bash
./build/mandelbrot_sequential --counts sequential.bin
./build/mandelbrot_hpx --counts hpx.bin
cmp sequential.bin hpx.bin
```

## Benchmarks

See [benchmark/README.md](benchmark/README.md).

## Source layout

| File | Contents |
|---|---|
| `src/mandelbrot.hpp` | Pixel grid and escape count, used on the CPU and the GPU |
| `src/app.hpp`, `src/app.cpp` | Command line options, colors, timing and output files |
| `src/tiles.hpp` | Tiles of the pixel grid and the border method |
| `src/sequential.cpp` | Sequential renderer |
| `src/hpx.cpp` | HPX renderer |
| `src/cuda.cu` | CUDA renderer |
| `src/explorer.cpp` | Explorer |
