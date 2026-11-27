#ifndef MANDELBROT_HPP
#define MANDELBROT_HPP

// Everything here is shared by the CPU and GPU programs.

#ifdef __CUDACC__
#define HOST_DEVICE __host__ __device__
#else
#define HOST_DEVICE
#endif

struct View {
    int width;
    int height;
    int max_iter;
    double xmin;
    double xmax;
    double ymin;
    double ymax;
};

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

HOST_DEVICE inline int pixel_count(const View& view, int x, int y) {
    double cr = view.xmin;
    if (view.width > 1) {
        cr += (view.xmax - view.xmin) * x / (view.width - 1);
    }

    double ci = view.ymin;
    if (view.height > 1) {
        ci += (view.ymax - view.ymin) * y / (view.height - 1);
    }

    return escape_count(cr, ci, view.max_iter);
}

#endif
