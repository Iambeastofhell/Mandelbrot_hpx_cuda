#ifndef MANDELBROT_KERNEL_CUH
#define MANDELBROT_KERNEL_CUH

#include "mandelbrot_common.hpp"

#include <cuda_runtime.h>

#include <cstdlib>
#include <iostream>

// The pixel code below repeats mandelbrot_common.cpp so that it can run on the GPU.
// Keep the two in sync. Build with --fmad=false so both give identical pixels.

enum ColorMode { BLACK_AND_WHITE, GRAYSCALE, GRADIENT };

struct MandelbrotView {
    int width;
    int height;
    int max_iter;
    ColorMode color;
    double xmin;
    double xmax;
    double ymin;
    double ymax;
};

inline MandelbrotView make_view(const AppOptions& options) {
    ColorMode color = options.gradient ? GRADIENT
                    : options.grayscale ? GRAYSCALE
                    : BLACK_AND_WHITE;
    return {options.width, options.height, options.max_iter, color,
            options.xmin, options.xmax, options.ymin, options.ymax};
}

inline void cuda_check(cudaError_t status) {
    if (status != cudaSuccess) {
        std::cerr << "CUDA error: " << cudaGetErrorString(status) << '\n';
        std::exit(1);
    }
}

__host__ __device__ inline Pixel shade(int iter, const MandelbrotView& view) {
    if (iter == view.max_iter) {
        return {0, 0, 0};
    }

    double t = static_cast<double>(iter) / static_cast<double>(view.max_iter);

    if (view.color == GRADIENT) {
        unsigned char r = static_cast<unsigned char>(9.0 * (1.0 - t) * t * t * t * 255.0);
        unsigned char g = static_cast<unsigned char>(15.0 * (1.0 - t) * (1.0 - t) * t * t * 255.0);
        unsigned char b = static_cast<unsigned char>(8.5 * (1.0 - t) * (1.0 - t) * (1.0 - t) * t * 255.0);
        return {r, g, b};
    }
    if (view.color == GRAYSCALE) {
        unsigned char gray = static_cast<unsigned char>(255.0 * t);
        return {gray, gray, gray};
    }
    return {255, 255, 255};
}

__host__ __device__ inline Pixel mandelbrot_pixel(const MandelbrotView& view, int x, int y) {
    double cr = view.xmin;
    if (view.width > 1) {
        cr += (view.xmax - view.xmin) * x / (view.width - 1);
    }

    double ci = view.ymin;
    if (view.height > 1) {
        ci += (view.ymax - view.ymin) * y / (view.height - 1);
    }

    double zr = 0.0;
    double zi = 0.0;
    int iter = 0;

    while (iter < view.max_iter) {
        double zr2 = zr * zr;
        double zi2 = zi * zi;
        if (zr2 + zi2 > 4.0) {
            break;
        }
        zi = 2.0 * zr * zi + ci;
        zr = zr2 - zi2 + cr;
        ++iter;
    }

    return shade(iter, view);
}

// Computes rows [row_begin, row_end) of the image, one thread per pixel.
// The grid only needs to cover those rows. pixels points at the start of the whole image.
__global__ void mandelbrot_rows(Pixel* pixels, MandelbrotView view, int row_begin, int row_end) {
    int x = blockIdx.x * blockDim.x + threadIdx.x;
    int y = row_begin + blockIdx.y * blockDim.y + threadIdx.y;

    if (x < view.width && y < row_end) {
        pixels[y * view.width + x] = mandelbrot_pixel(view, x, y);
    }
}

#endif
