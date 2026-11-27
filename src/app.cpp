#include "app.hpp"

#include <chrono>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <limits>

static void print_usage(const char* program) {
    std::cout << "Usage: " << program << " [options]\n"
              << "  --center X Y      Point at the center of the image (default: -0.5 0)\n"
              << "  --view-width W    Width of the view on the real axis (default: 3)\n"
              << "  --width N         Image width in pixels (default: 1024)\n"
              << "  --height N        Image height in pixels (default: 768)\n"
              << "  --maxiter N       Iteration limit (default: 500)\n"
              << "  --color NAME      bw, gray or gradient (default: gradient)\n"
              << "  --save FILE       Save the image as PPM\n"
              << "  --counts FILE     Save the escape counts as raw 32-bit integers\n"
              << "  --threads N       HPX worker threads (default: chosen by HPX)\n"
              << "  --grain N         Pixels per HPX task, 0 for one row (default: 0)\n"
              << "  --help            Show this help\n";
}

[[noreturn]] static void fail(const char* program, const std::string& message) {
    std::cerr << "Error: " << message << '\n';
    print_usage(program);
    std::exit(1);
}

Options parse_options(int argc, char* argv[]) {
    Options options;
    std::string name;
    int i = 1;

    auto value = [&]() -> std::string {
        if (i + 1 >= argc) {
            fail(argv[0], "Missing value for " + name);
        }
        return argv[++i];
    };
    auto integer = [&]() {
        std::string text = value();
        char* end = nullptr;
        long parsed = std::strtol(text.c_str(), &end, 10);
        if (text.empty() || *end != '\0' || parsed < std::numeric_limits<int>::min() ||
            parsed > std::numeric_limits<int>::max()) {
            fail(argv[0], "Invalid integer value for " + name);
        }
        return static_cast<int>(parsed);
    };
    auto number = [&]() {
        std::string text = value();
        char* end = nullptr;
        double parsed = std::strtod(text.c_str(), &end);
        if (text.empty() || *end != '\0' || !std::isfinite(parsed)) {
            fail(argv[0], "Invalid number value for " + name);
        }
        return parsed;
    };

    for (; i < argc; ++i) {
        name = argv[i];
        if (name == "--help" || name == "-h") {
            print_usage(argv[0]);
            std::exit(0);
        } else if (name == "--center") {
            options.center_x = number();
            options.center_y = number();
        } else if (name == "--view-width") {
            options.view_width = number();
        } else if (name == "--width") {
            options.width = integer();
        } else if (name == "--height") {
            options.height = integer();
        } else if (name == "--maxiter") {
            options.max_iter = integer();
        } else if (name == "--color") {
            std::string color = value();
            if (color == "bw") {
                options.color = Color::black_and_white;
            } else if (color == "gray") {
                options.color = Color::grayscale;
            } else if (color == "gradient") {
                options.color = Color::gradient;
            } else {
                fail(argv[0], "Unknown color: " + color);
            }
        } else if (name == "--save") {
            options.save_file = value();
        } else if (name == "--counts") {
            options.counts_file = value();
        } else if (name == "--threads") {
            options.threads = integer();
        } else if (name == "--grain") {
            options.grain = integer();
        } else {
            fail(argv[0], "Unknown option: " + name);
        }
    }

    if (options.width <= 0 || options.height <= 0) {
        fail(argv[0], "Width and height must be greater than zero");
    }
    if (options.width > std::numeric_limits<int>::max() / options.height) {
        fail(argv[0], "The image is too large");
    }
    if (std::abs(options.center_x) > 4.0 || std::abs(options.center_y) > 4.0) {
        fail(argv[0], "Center coordinates must be between -4 and 4");
    }
    if (options.view_width > MAX_VIEW_WIDTH || options.view_width / options.width < MIN_SPACING) {
        fail(argv[0], "View width is out of range");
    }
    if (options.max_iter <= 0) {
        fail(argv[0], "maxiter must be greater than zero");
    }
    if (options.threads < 0 || options.grain < 0) {
        fail(argv[0], "threads and grain must be zero or greater");
    }

    options.view = make_view(options.center_x, options.center_y, options.view_width, options.width,
                             options.height, options.max_iter);
    return options;
}

Pixel color_pixel(int count, int max_iter, Color color) {
    if (count == max_iter) {
        return {0, 0, 0};
    }
    if (color == Color::black_and_white) {
        return {255, 255, 255};
    }

    double t = static_cast<double>(count) / static_cast<double>(max_iter);
    if (color == Color::grayscale) {
        auto gray = static_cast<unsigned char>(255.0 * t);
        return {gray, gray, gray};
    }

    auto r = static_cast<unsigned char>(9.0 * (1.0 - t) * t * t * t * 255.0);
    auto g = static_cast<unsigned char>(15.0 * (1.0 - t) * (1.0 - t) * t * t * 255.0);
    auto b = static_cast<unsigned char>(8.5 * (1.0 - t) * (1.0 - t) * (1.0 - t) * t * 255.0);
    return {r, g, b};
}

static bool write_file(const std::string& path, const std::string& header, const void* data, std::size_t size) {
    std::ofstream file(path, std::ios::binary);
    file << header;
    file.write(static_cast<const char*>(data), static_cast<std::streamsize>(size));
    file.close();

    if (!file) {
        std::cerr << "Error: could not write " << path << '\n';
        return false;
    }
    std::cout << "Saved " << path << '\n';
    return true;
}

bool save_results(const Options& options, const std::vector<int>& counts) {
    if (!options.save_file.empty()) {
        static_assert(sizeof(Pixel) == 3, "PPM needs three bytes per pixel");
        std::vector<Pixel> pixels;
        pixels.reserve(counts.size());
        for (int count : counts) {
            pixels.push_back(color_pixel(count, options.view.max_iter, options.color));
        }

        std::string header = "P6\n" + std::to_string(options.width) + " " + std::to_string(options.height) + "\n255\n";
        if (!write_file(options.save_file, header, pixels.data(), pixels.size() * sizeof(Pixel))) {
            return false;
        }
    }

    return options.counts_file.empty() ||
           write_file(options.counts_file, "", counts.data(), counts.size() * sizeof(int));
}

int run(const Options& options, const char* name, Renderer render) {
    std::vector<int> counts(static_cast<std::size_t>(options.width) * options.height);

    auto start = std::chrono::steady_clock::now();
    render(options, counts);
    std::chrono::duration<double> elapsed = std::chrono::steady_clock::now() - start;
    std::cout << name << " generation time: " << elapsed.count() << " seconds\n";

    return save_results(options, counts) ? 0 : 1;
}
