#ifndef MANDELBROT_HPP
#define MANDELBROT_HPP

// Everything here is shared by the CPU and GPU programs.

#include <cmath>

#ifdef __CUDACC__
#define HOST_DEVICE __host__ __device__
#else
#define HOST_DEVICE
#endif

// Below this spacing neighbouring pixels are hard to tell apart in double precision.
constexpr double MIN_SPACING = 1e-15;
constexpr double MAX_VIEW_WIDTH = 16.0;

// A block of pixels on a grid fixed to the complex plane, with x to the right and y down.
struct Rect {
    long long left;
    long long top;
    int width;
    int height;
};

struct View {
    Rect area;
    double spacing;
    int max_iter;
};

// Escape counts of the pixels in area, row by row.
struct Canvas {
    int* counts;
    Rect area;

    HOST_DEVICE int& at(long long x, long long y) const {
        return counts[(y - area.top) * area.width + (x - area.left)];
    }
};

inline View make_view(double center_x, double center_y, double view_width, int width, int height, int max_iter) {
    double spacing = view_width / width;
    long long left = std::llround(center_x / spacing) - width / 2;
    long long top = std::llround(-center_y / spacing) - height / 2;
    return {{left, top, width, height}, spacing, max_iter};
}

HOST_DEVICE inline int escape_count(double cr, double ci, int max_iter) {
    double zr = 0.0;
    double zi = 0.0;
    int iter = 0;

    while (iter < max_iter) {
        double zr2 = zr * zr;
        double zi2 = zi * zi;
        if (zr2 + zi2 > 4.0) {
            break;
        }
        zi = 2.0 * zr * zi + ci;
        zr = zr2 - zi2 + cr;
        ++iter;
    }

    return iter;
}

// The coordinate depends only on the grid position, so a point has the same value in every view.
HOST_DEVICE inline int pixel_count(const View& view, long long x, long long y) {
    return escape_count(x * view.spacing, -y * view.spacing, view.max_iter);
}

inline void compute(const View& view, const Rect& rect, const Canvas& canvas) {
    for (long long y = rect.top; y < rect.top + rect.height; ++y) {
        for (long long x = rect.left; x < rect.left + rect.width; ++x) {
            canvas.at(x, y) = pixel_count(view, x, y);
        }
    }
}

#endif
