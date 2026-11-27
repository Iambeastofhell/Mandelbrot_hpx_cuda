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
    int threads = 0; // 0 leaves the number to HPX
    std::string method = "row";
    int tile = 64;     // side of a tile in pixels
    int min_tile = 16; // smaller tiles are computed pixel by pixel
    int chunk = 0;     // pixels per piece of a loop, 0 lets HPX choose
    int pans = 0;      // automatic pans, 0 opens the window
    bool reuse = true; // keep finished tiles for later views
    std::vector<std::string> hpx_options; // passed on to HPX as they are
    std::string save_file;
    std::string counts_file;
    View view{}; // built from the values above
};

// Fills counts with one escape count per pixel, row by row.
using Renderer = void (*)(const Options &options, std::vector<int> &counts);

// Exits the program for --help or an invalid argument.
Options parse_options(int argc, char *argv[]);

// Counts equal to max_iter are black.
Pixel color_pixel(int count, int max_iter, Color color);

// Writes the files requested by --save and --counts.
bool save_results(const Options &options, const std::vector<int> &counts);

// Times render and saves its results. Returns the exit status.
int run(const Options &options, const char *name, Renderer render);

#endif
