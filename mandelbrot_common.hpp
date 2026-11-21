#ifndef MANDEL_BROT_COMMON_HPP
#define MANDEL_BROT_COMMON_HPP

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

    Image(int width_value, int height_value);

    Pixel& at(int x, int y);
    const Pixel& at(int x, int y) const;
};

struct AppOptions {
    int width = 80;
    int height = 40;
    double xmin = -2.0;
    double xmax = 1.0;
    double ymin = -1.5;
    double ymax = 1.5;
    int max_iter = 100;
    int threads = 0;
    bool grayscale = false;
    bool gradient = false;
    bool display = false;
    std::string save_file;
    std::string load_file;
};

int mandelbrot(double cr, double ci, int max_iter);
Pixel color_black_and_white(int iter, int max_iter);
Pixel color_grayscale(int iter, int max_iter);
Pixel color_gradient(int iter, int max_iter);

bool parse_arguments(int argc, char* argv[], AppOptions& options, std::string& error);
bool validate_arguments(const AppOptions& options, std::string& error);
void print_usage(const char* program);

Image load_ppm(const std::string& filename, bool& success);
bool save_ppm(const Image& image, const std::string& filename);
void display_image(const Image& image, const AppOptions& options);

#endif
