#ifndef MANDELBROT_KERNEL_H
#define MANDELBROT_KERNEL_H

#include "mandelbrot_common.hpp"

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

extern "C" void launch_mandelbrot_kernel(Pixel* host_pixels, const MandelbrotView& view, int row_begin, int row_end);

#endif