#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstdio>

// Simple PPM image save/load - no external dependencies needed
struct Pixel {
    unsigned char r, g, b;
};

struct Image {
    int width, height;
    std::vector<Pixel> pixels;

    Image(int w, int h) : width(w), height(h), pixels(static_cast<size_t>(w) * h) {}

    Pixel& at(int x, int y) {
        return pixels[static_cast<size_t>(y) * width + x];
    }

    const Pixel& at(int x, int y) const {
        return pixels[static_cast<size_t>(y) * width + x];
    }
};

// Compute Mandelbrot iteration count for a point
int mandelbrot(double cr, double ci, int max_iter) {
    double zr = 0.0, zi = 0.0;
    int iter = 0;
    while (iter < max_iter) {
        double zr2 = zr * zr;
        double zi2 = zi * zi;
        if (zr2 + zi2 > 4.0) break;
        zi = 2.0 * zr * zi + ci;
        zr = zr2 - zi2 + cr;
        iter++;
    }
    return iter;
}

// Black and white color scheme
Pixel color_bw(int iter, int max_iter) {
    if (iter == max_iter) return {0, 0, 0};   // inside set: black
    return {255, 255, 255};                    // outside set: white
}

// Gradient color scheme based on iteration count
Pixel color_gradient(int iter, int max_iter) {
    if (iter == max_iter) return {0, 0, 0};    // inside set: black
    double t = static_cast<double>(iter) / max_iter;
    unsigned char r = static_cast<unsigned char>(9 * (1 - t) * t * t * t * 255);
    unsigned char g = static_cast<unsigned char>(15 * (1 - t) * (1 - t) * t * t * 255);
    unsigned char b = static_cast<unsigned char>(8.5 * (1 - t) * (1 - t) * (1 - t) * t * 255);
    return {r, g, b};
}

// Generate the Mandelbrot image sequentially (single-threaded)
Image generate_mandelbrot(int width, int height,
                          double x_min, double x_max,
                          double y_min, double y_max,
                          int max_iter, bool use_gradient) {
    Image img(width, height);
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            double cr = x_min + (x_max - x_min) * x / (width - 1);
            double ci = y_min + (y_max - y_min) * y / (height - 1);
            int iter = mandelbrot(cr, ci, max_iter);
            img.at(x, y) = use_gradient ? color_gradient(iter, max_iter) : color_bw(iter, max_iter);
        }
    }
    return img;
}

// Save image to PPM file
void save_ppm(const Image& img, const std::string& filename) {
    std::FILE* f = std::fopen(filename.c_str(), "wb");
    if (!f) {
        std::cerr << "Error: could not open file " << filename << " for writing\n";
        return;
    }
    std::fprintf(f, "P6\n%d %d\n255\n", img.width, img.height);
    for (int y = 0; y < img.height; y++) {
        for (int x = 0; x < img.width; x++) {
            const Pixel& p = img.at(x, y);
            unsigned char buf[3] = {p.r, p.g, p.b};
            std::fwrite(buf, 1, 3, f);
        }
    }
    std::fclose(f);
    std::cout << "Saved image to " << filename << "\n";
}

// Load image from PPM file (P6 binary)
Image load_ppm(const std::string& filename) {
    std::FILE* f = std::fopen(filename.c_str(), "rb");
    if (!f) {
        std::cerr << "Error: could not open file " << filename << " for reading\n";
        return Image(0, 0);
    }

    char magic[3];
    if (std::fread(magic, 1, 2, f) != 2 || magic[0] != 'P' || magic[1] != '6') {
        std::cerr << "Error: not a valid PPM P6 file\n";
        std::fclose(f);
        return Image(0, 0);
    }

    int width, height, maxval;
    int c;
    // skip whitespace
    do { c = std::fgetc(f); } while (c == ' ' || c == '\t' || c == '\n' || c == '\r');
    std::ungetc(c, f);
    if (std::fscanf(f, "%d %d %d", &width, &height, &maxval) != 3) {
        std::cerr << "Error: could not read PPM header\n";
        std::fclose(f);
        return Image(0, 0);
    }
    // skip whitespace after maxval
    do { c = std::fgetc(f); } while (c == ' ' || c == '\t' || c == '\n' || c == '\r');
    std::ungetc(c, f);

    Image img(width, height);
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            Pixel& p = img.at(x, y);
            unsigned char buf[3];
            if (std::fread(buf, 1, 3, f) != 3) {
                std::cerr << "Error: truncated PPM data\n";
                std::fclose(f);
                return Image(0, 0);
            }
            p.r = buf[0]; p.g = buf[1]; p.b = buf[2];
        }
    }
    std::fclose(f);
    std::cout << "Loaded image from " << filename << " (" << width << "x" << height << ")\n";
    return img;
}

// Open image file in a separate window using the system's default viewer
void open_in_window(const std::string& filename) {
#ifdef _WIN32
    std::string cmd = "start \"\" \"" + filename + "\"";
#elif __APPLE__
    std::string cmd = "open \"" + filename + "\"";
#else
    std::string cmd = "xdg-open \"" + filename + "\"";
#endif
    (void)std::system(cmd.c_str());
}

void print_usage(const char* prog) {
    std::cout << "Usage: " << prog << " [options]\n";
    std::cout << "Options:\n";
    std::cout << "  --width W       Image width (default: 80)\n";
    std::cout << "  --height H      Image height (default: 40)\n";
    std::cout << "  --xmin X        Minimum real part (default: -2.0)\n";
    std::cout << "  --xmax X        Maximum real part (default: 1.0)\n";
    std::cout << "  --ymin Y        Minimum imaginary part (default: -1.5)\n";
    std::cout << "  --ymax Y        Maximum imaginary part (default: 1.5)\n";
    std::cout << "  --maxiter N     Maximum iterations (default: 100)\n";
    std::cout << "  --gradient      Use gradient colors instead of black and white\n";
    std::cout << "  --save FILE     Save image to PPM file\n";
    std::cout << "  --load FILE     Load image from PPM file instead of generating\n";
    std::cout << "  --display       Display image\n";
    std::cout << "  --help          Show this help message\n";
}

int main(int argc, char* argv[]) {
    int width = 80;
    int height = 40;
    double x_min = -2.0;
    double x_max = 1.0;
    double y_min = -1.5;
    double y_max = 1.5;
    int max_iter = 100;
    bool use_gradient = false;
    bool do_display = false;
    bool do_save = false;
    std::string save_file;
    std::string load_file;

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--help" || arg == "-h") {
            print_usage(argv[0]);
            return 0;
        } else if (arg == "--width" && i + 1 < argc) {
            width = std::stoi(argv[++i]);
        } else if (arg == "--height" && i + 1 < argc) {
            height = std::stoi(argv[++i]);
        } else if (arg == "--xmin" && i + 1 < argc) {
            x_min = std::stod(argv[++i]);
        } else if (arg == "--xmax" && i + 1 < argc) {
            x_max = std::stod(argv[++i]);
        } else if (arg == "--ymin" && i + 1 < argc) {
            y_min = std::stod(argv[++i]);
        } else if (arg == "--ymax" && i + 1 < argc) {
            y_max = std::stod(argv[++i]);
        } else if (arg == "--maxiter" && i + 1 < argc) {
            max_iter = std::stoi(argv[++i]);
        } else if (arg == "--gradient") {
            use_gradient = true;
        } else if (arg == "--save" && i + 1 < argc) {
            do_save = true;
            save_file = argv[++i];
        } else if (arg == "--load" && i + 1 < argc) {
            load_file = argv[++i];
        } else if (arg == "--display") {
            do_display = true;
        } else {
            std::cerr << "Unknown option: " << arg << "\n";
            print_usage(argv[0]);
            return 1;
        }
    }

    Image img(0, 0);

    if (!load_file.empty()) {
        img = load_ppm(load_file);
        if (img.width == 0 || img.height == 0) {
            return 1;
        }
    } else {
        img = generate_mandelbrot(width, height, x_min, x_max, y_min, y_max,
                                  max_iter, use_gradient);
        std::cout << "Generated Mandelbrot set (" << width << "x" << height
                  << ", max_iter=" << max_iter
                  << ", gradient=" << (use_gradient ? "on" : "off") << ")\n";
    }

    if (do_save && !save_file.empty()) {
        save_ppm(img, save_file);
    }

    if (do_display) {
        std::string display_file = save_file;
        if (display_file.empty()) {
            display_file = "/tmp/mandelbrot_display.ppm";
            save_ppm(img, display_file);
        }
        std::cout << "Opening image ...\n";
        open_in_window(display_file);
    }

    if (!do_save && !do_display) {
        std::cout << "No action specified. Use --save and/or --display.\n";
    }

    return 0;
}