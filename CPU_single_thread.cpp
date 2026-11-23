#include "mandelbrot_common.hpp"

#include <chrono>
#include <iostream>
#include <string>

static Pixel pixel_color(int iter, const AppOptions& options) {
    if (options.gradient) {
        return color_gradient(iter, options.max_iter);
    }
    if (options.grayscale) {
        return color_grayscale(iter, options.max_iter);
    }
    return color_black_and_white(iter, options.max_iter);
}

static Image generate_mandelbrot(const AppOptions& options) {
    Image image(options.width, options.height);

    for (int y = 0; y < options.height; ++y) {
        for (int x = 0; x < options.width; ++x) {
            double cr;
            double ci;

            if (options.width == 1) {
                cr = options.xmin;
            } else {
                cr = options.xmin +
                    (options.xmax - options.xmin) *
                    static_cast<double>(x) /
                    static_cast<double>(options.width - 1);
            }

            if (options.height == 1) {
                ci = options.ymin;
            } else {
                ci = options.ymin +
                    (options.ymax - options.ymin) *
                    static_cast<double>(y) /
                    static_cast<double>(options.height - 1);
            }

            int iter = mandelbrot(cr, ci, options.max_iter);
            image.at(x, y) = pixel_color(iter, options);
        }
    }

    return image;
}

int main(int argc, char* argv[]) {
    AppOptions options;
    std::string error;

    if (!parse_arguments(argc, argv, options, error)) {
        if (error == "") {
            return 0;
        }
        std::cerr << "Error: " << error << '\n';
        print_usage(argv[0]);
        return 1;
    }

    if (!validate_arguments(options, error)) {
        std::cerr << "Error: " << error << '\n';
        print_usage(argv[0]);
        return 1;
    }

    Image image(0, 0);

    if (!options.load_file.empty()) {
        bool loaded = false;
        image = load_ppm(options.load_file, loaded);
        if (!loaded) {
            return 1;
        }
    } else {
        auto start = std::chrono::steady_clock::now();
        image = generate_mandelbrot(options);
        auto end = std::chrono::steady_clock::now();

        std::chrono::duration<double> elapsed = end - start;
        std::cout << "Sequential generation time: " << elapsed.count() << " seconds\n";
        std::cout << "Generated Mandelbrot set (" << options.width << "x"
                  << options.height << ", max_iter=" << options.max_iter
                  << ")\n";
    }

    if (!options.save_file.empty()) {
        if (!save_ppm(image, options.save_file)) {
            return 1;
        }
    }

    if (options.display) {
        display_image(image, options);
    }

    return 0;
}
