#ifndef MANDELBROT_KERNEL_H
#define MANDELBROT_KERNEL_H

#include "mandelbrot_common.hpp"

#include <algorithm>

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

// Rows per GPU strip. The default is an eighth of the image.
inline int strip_rows(const AppOptions& options) {
    int rows = options.grain > 0 ? options.grain / options.width : options.height / 8;
    return std::max(rows, 1);
}

extern "C" void cuda_startup();
extern "C" void launch_mandelbrot_kernel(Pixel* host_pixels, const MandelbrotView& view, int row_begin, int row_end);

extern "C" void stream_init(int num_streams, Image& image, int strip_height);
extern "C" void stream_cleanup(Image& image);
extern "C" void launch_mandelbrot_stream(Pixel* host_pixels, const MandelbrotView& view,
                                         int row_begin, int row_end, int stream_idx);
extern "C" bool stream_done(int stream_idx);

#endif