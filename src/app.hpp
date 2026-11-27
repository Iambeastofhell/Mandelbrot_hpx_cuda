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

enum class Color { black_and_white, grayscale, gradient };

struct Options {
    double center_x = -0.5;
    double center_y = 0.0;
    double view_width = 3.0;
    int width = 1024;
    int height = 768;
    int max_iter = 500;
    Color color = Color::gradient;
    int threads = 0;
    std::string method = "row";
    int tile = 64;
    int min_tile = 16;
    int chunk = 0;
    int pans = 0;
    std::string save_file;
    std::string counts_file;
    View view{};  // built from the values above
};

// Fills counts with one escape count per pixel, row by row.
using Renderer = void (*)(const Options& options, std::vector<int>& counts);

// Exits the program for --help or an invalid argument.
Options parse_options(int argc, char* argv[]);

Pixel color_pixel(int count, int max_iter, Color color);

// Writes the files requested by --save and --counts.
bool save_results(const Options& options, const std::vector<int>& counts);

// Times the renderer and saves its results. Returns the exit status.
int run(const Options& options, const char* name, Renderer render);

#endif
