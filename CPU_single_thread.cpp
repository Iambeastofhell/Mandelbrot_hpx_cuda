#include "mandelbrot_common.hpp"

#include <chrono>
#include <iostream>
#include <string>

static Image generate_mandelbrot(const AppOptions& options) {
    Image image(options.width, options.height);

    for (int y = 0; y < options.height; ++y) {
        for (int x = 0; x < options.width; ++x) {
            image.at(x, y) = render_pixel(options, x, y);
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
