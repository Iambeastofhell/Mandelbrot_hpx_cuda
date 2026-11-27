#include "app.hpp"

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <limits>

static Pixel color_black_and_white(int iter, int max_iter) {
    if (iter == max_iter) {
        return {0, 0, 0};
    }
    return {255, 255, 255};
}

static Pixel color_grayscale(int iter, int max_iter) {
    if (iter == max_iter) {
        return {0, 0, 0};
    }

    double value = static_cast<double>(iter) / static_cast<double>(max_iter);
    auto gray = static_cast<unsigned char>(255.0 * value);
    return {gray, gray, gray};
}

static Pixel color_gradient(int iter, int max_iter) {
    if (iter == max_iter) {
        return {0, 0, 0};
    }

    double t = static_cast<double>(iter) / static_cast<double>(max_iter);
    unsigned char r = static_cast<unsigned char>(9.0 * (1.0 - t) * t * t * t * 255.0);
    unsigned char g = static_cast<unsigned char>(15.0 * (1.0 - t) * (1.0 - t) * t * t * 255.0);
    unsigned char b = static_cast<unsigned char>(8.5 * (1.0 - t) * (1.0 - t) * (1.0 - t) * t * 255.0);
    return {r, g, b};
}

static Pixel color_pixel(const Options& options, int count) {
    if (options.gradient) {
        return color_gradient(count, options.view.max_iter);
    }
    if (options.grayscale) {
        return color_grayscale(count, options.view.max_iter);
    }
    return color_black_and_white(count, options.view.max_iter);
}

static void print_usage(const char* program) {
    std::cout << "Usage: " << program << " [options]\n";
    std::cout << "Options:\n";
    std::cout << "  --width W       Image width (default: 80)\n";
    std::cout << "  --height H      Image height (default: 40)\n";
    std::cout << "  --xmin X        Minimum real value (default: -2.0)\n";
    std::cout << "  --xmax X        Maximum real value (default: 1.0)\n";
    std::cout << "  --ymin Y        Minimum imaginary value (default: -1.5)\n";
    std::cout << "  --ymax Y        Maximum imaginary value (default: 1.5)\n";
    std::cout << "  --maxiter N     Maximum iterations (default: 100)\n";
    std::cout << "  --grayscale     Use a grayscale color scheme\n";
    std::cout << "  --gradient      Use a gradient color scheme (default: black and white)\n";
    std::cout << "  --threads N     HPX worker threads; zero uses the HPX default\n";
    std::cout << "  --grain N       Pixels per HPX task; zero means one row per task\n";
    std::cout << "  --save FILE     Save the image as PPM\n";
    std::cout << "  --load FILE     Load a PPM image instead of generating it\n";
    std::cout << "  --display       Open the image in a separate window\n";
    std::cout << "  --help          Show this help message\n";
}

static bool parse_int(const char* text, int& value) {
    char* end = nullptr;
    long parsed = std::strtol(text, &end, 10);
    if (end == text || *end != '\0' || parsed < std::numeric_limits<int>::min() || parsed > std::numeric_limits<int>::max()) {
        return false;
    }
    value = static_cast<int>(parsed);
    return true;
}

static bool parse_double(const char* text, double& value) {
    char* end = nullptr;
    value = std::strtod(text, &end);
    return end != text && *end == '\0';
}

static bool parse_arguments(int argc, char* argv[], Options& options, std::string& error) {
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "--help" || arg == "-h") {
            print_usage(argv[0]);
            return false;
        }

        if (arg == "--width" || arg == "--height" || arg == "--maxiter" || arg == "--threads" ||
            arg == "--grain") {
            if (i + 1 >= argc) {
                error = "Missing value for " + arg;
                return false;
            }
            int value = 0;
            if (!parse_int(argv[++i], value)) {
                error = "Invalid integer value for " + arg;
                return false;
            }
            if (arg == "--width") options.view.width = value;
            if (arg == "--height") options.view.height = value;
            if (arg == "--maxiter") options.view.max_iter = value;
            if (arg == "--threads") options.threads = value;
            if (arg == "--grain") options.grain = value;
            continue;
        }

        if (arg == "--xmin" || arg == "--xmax" || arg == "--ymin" || arg == "--ymax") {
            if (i + 1 >= argc) {
                error = "Missing value for " + arg;
                return false;
            }
            double value = 0.0;
            if (!parse_double(argv[++i], value)) {
                error = "Invalid number value for " + arg;
                return false;
            }
            if (arg == "--xmin") options.view.xmin = value;
            if (arg == "--xmax") options.view.xmax = value;
            if (arg == "--ymin") options.view.ymin = value;
            if (arg == "--ymax") options.view.ymax = value;
            continue;
        }

        if (arg == "--grayscale") {
            options.grayscale = true;
            continue;
        }
        if (arg == "--gradient") {
            options.gradient = true;
            continue;
        }
        if (arg == "--save" || arg == "--load") {
            if (i + 1 >= argc) {
                error = "Missing value for " + arg;
                return false;
            }
            if (arg == "--save") options.save_file = argv[++i];
            if (arg == "--load") options.load_file = argv[++i];
            continue;
        }
        if (arg == "--display") {
            options.display = true;
            continue;
        }

        error = "Unknown option: " + arg;
        return false;
    }

    return true;
}

static bool validate_arguments(const Options& options, std::string& error) {
    if (options.view.width <= 0 || options.view.height <= 0) {
        error = "Width and height must be greater than zero";
        return false;
    }
    if (options.view.xmin >= options.view.xmax) {
        error = "xmin must be less than xmax";
        return false;
    }
    if (options.view.ymin >= options.view.ymax) {
        error = "ymin must be less than ymax";
        return false;
    }
    if (options.view.max_iter <= 0) {
        error = "maxiter must be greater than zero";
        return false;
    }
    if (options.threads < 0) {
        error = "threads must be zero or greater";
        return false;
    }
    if (options.grain < 0) {
        error = "grain must be zero or greater";
        return false;
    }
    if (options.grayscale && options.gradient) {
        error = "Use either --grayscale or --gradient, not both";
        return false;
    }
    if (options.save_file.empty() && options.load_file.empty() && !options.display) {
        error = "No action specified. Use --save, --load, or --display";
        return false;
    }
    return true;
}

static bool save_ppm(const Image& image, const std::string& filename) {
    std::FILE* file = std::fopen(filename.c_str(), "wb");
    if (!file) {
        std::cerr << "Error: could not open " << filename << " for writing\n";
        return false;
    }

    std::fprintf(file, "P6\n%d %d\n255\n", image.width, image.height);
    for (const Pixel& pixel : image.pixels) {
        unsigned char bytes[3] = {pixel.r, pixel.g, pixel.b};
        std::fwrite(bytes, 1, 3, file);
    }

    if (std::fclose(file) != 0) {
        std::cerr << "Error: could not finish writing " << filename << '\n';
        return false;
    }

    std::cout << "Saved image to " << filename << '\n';
    return true;
}

static void skip_ppm_header_whitespace(std::FILE* file) {
    int character = std::fgetc(file);
    while (character == ' ' || character == '\t' || character == '\n' || character == '\r') {
        character = std::fgetc(file);
    }
    if (character != EOF) {
        std::ungetc(character, file);
    }
}

static Image load_ppm(const std::string& filename, bool& success) {
    success = false;
    std::FILE* file = std::fopen(filename.c_str(), "rb");
    if (!file) {
        std::cerr << "Error: could not open " << filename << " for reading\n";
        return {};
    }

    char magic[2];
    if (std::fread(magic, 1, 2, file) != 2 || magic[0] != 'P' || magic[1] != '6') {
        std::cerr << "Error: " << filename << " is not a PPM P6 file\n";
        std::fclose(file);
        return {};
    }

    skip_ppm_header_whitespace(file);

    int width = 0;
    int height = 0;
    int max_value = 0;
    if (std::fscanf(file, "%d %d %d", &width, &height, &max_value) != 3 ||
        width <= 0 || height <= 0 || max_value != 255) {
        std::cerr << "Error: invalid PPM header in " << filename << '\n';
        std::fclose(file);
        return {};
    }

    int separator = std::fgetc(file);
    if (separator != ' ' && separator != '\t' && separator != '\n' && separator != '\r') {
        std::cerr << "Error: invalid PPM header in " << filename << '\n';
        std::fclose(file);
        return {};
    }
    if (separator == '\r') {
        int next_character = std::fgetc(file);
        if (next_character != '\n') {
            std::ungetc(next_character, file);
        }
    }

    Image image{width, height, std::vector<Pixel>(static_cast<std::size_t>(width) * height)};
    for (Pixel& pixel : image.pixels) {
        unsigned char bytes[3];
        if (std::fread(bytes, 1, 3, file) != 3) {
            std::cerr << "Error: truncated PPM data in " << filename << '\n';
            std::fclose(file);
            return {};
        }
        pixel = {bytes[0], bytes[1], bytes[2]};
    }

    std::fclose(file);
    std::cout << "Loaded image from " << filename << " (" << width << "x" << height << ")\n";
    success = true;
    return image;
}

static void display_image(const Image& image, const Options& options) {
    std::string display_file = options.save_file;
    if (display_file.empty()) {
        display_file = "/tmp/mandelbrot_hpx.ppm";
        if (!save_ppm(image, display_file)) {
            return;
        }
    }

    std::cout << "Opening image...\n";

#if defined(_WIN32)
    std::string command = "start \"\" \"" + display_file + "\"";
#elif defined(__APPLE__)
    std::string command = "open \"" + display_file + "\"";
#else
    std::string command = "xdg-open \"" + display_file + "\"";
#endif

    int exit_code = std::system(command.c_str());
    if (exit_code != 0) {
        std::cerr << "Could not open the image.\n";
    }
}

Options parse_options(int argc, char* argv[]) {
    Options options;
    std::string error;

    if (!parse_arguments(argc, argv, options, error)) {
        if (error.empty()) {
            std::exit(0);
        }
    } else if (validate_arguments(options, error)) {
        return options;
    }

    std::cerr << "Error: " << error << '\n';
    print_usage(argv[0]);
    std::exit(1);
}

int run(const Options& options, const char* name, Renderer render) {
    Image image{};

    if (!options.load_file.empty()) {
        bool loaded = false;
        image = load_ppm(options.load_file, loaded);
        if (!loaded) {
            return 1;
        }
    } else {
        const View& view = options.view;
        std::vector<int> counts(static_cast<std::size_t>(view.width) * view.height);

        auto start = std::chrono::steady_clock::now();
        render(options, counts);
        auto end = std::chrono::steady_clock::now();

        std::chrono::duration<double> elapsed = end - start;
        std::cout << name << " generation time: " << elapsed.count() << " seconds\n";

        image = {view.width, view.height, std::vector<Pixel>(counts.size())};
        for (std::size_t i = 0; i < counts.size(); ++i) {
            image.pixels[i] = color_pixel(options, counts[i]);
        }
    }

    if (!options.save_file.empty() && !save_ppm(image, options.save_file)) {
        return 1;
    }

    if (options.display) {
        display_image(image, options);
    }

    return 0;
}
