#ifndef APP_HPP
#define APP_HPP

#include "mandelbrot.hpp"

#include <string>
#include <vector>

struct Pixel {
    unsigned char r;
    unsigned char g;
    unsigned char b;
};

struct Image {
    int width;
    int height;
    std::vector<Pixel> pixels;
};

struct Options {
    View view{80, 40, 100, -2.0, 1.0, -1.5, 1.5};
    int threads = 0;
    int grain = 0;
    bool grayscale = false;
    bool gradient = false;
    bool display = false;
    std::string save_file;
    std::string load_file;
};

// Fills counts with one escape count per pixel, row by row.
using Renderer = void (*)(const Options& options, std::vector<int>& counts);

// Exits the program for --help or an invalid argument.
Options parse_options(int argc, char* argv[]);

// Renders or loads the image, then saves and displays it as requested.
int run(const Options& options, const char* name, Renderer render);

#endif
