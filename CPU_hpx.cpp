#include "mandelbrot_common.hpp"

#include <hpx/future.hpp>
#include <hpx/hpx_init.hpp>

#include <chrono>
#include <iostream>
#include <string>
#include <vector>

static void generate_mandelbrot(const AppOptions& options, Image& image) {
    std::vector<hpx::future<void>> rows;
    rows.reserve(static_cast<std::size_t>(options.height));

    for (int y = 0; y < options.height; ++y) {
        rows.push_back(hpx::async([&options, &image, y]() {
            for (int x = 0; x < options.width; ++x) {
                image.at(x, y) = render_pixel(options, x, y);
            }
        }));
    }

    hpx::wait_all(rows);
}

static int run(const AppOptions& options) {
    Image image(0, 0);

    if (!options.load_file.empty()) {
        bool loaded = false;
        image = load_ppm(options.load_file, loaded);
        if (!loaded) {
            return 1;
        }
    } else {
        image = Image(options.width, options.height);

        auto start = std::chrono::steady_clock::now();
        generate_mandelbrot(options, image);
        auto end = std::chrono::steady_clock::now();

        std::chrono::duration<double> elapsed = end - start;
        std::cout << "HPX generation time: " << elapsed.count() << " seconds\n";
        std::cout << "Generated Mandelbrot set (" << options.width << "x"
                  << options.height << ", max_iter=" << options.max_iter
                  << ")\n";
    }

    if (!options.save_file.empty() && !save_ppm(image, options.save_file)) {
        return 1;
    }

    if (options.display) {
        display_image(image, options);
    }

    return 0;
}

int main(int argc, char* argv[]) {
    AppOptions options;
    std::string error;

    if (!parse_arguments(argc, argv, options, error)) {
        if (error.empty()) {
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

    std::string threads_option = "--hpx:threads=" + std::to_string(options.threads);
    char* hpx_argv[] = {argv[0], threads_option.data(), nullptr};
    int hpx_argc = options.threads > 0 ? 2 : 1;

    return hpx::init(
        [&options](hpx::program_options::variables_map&) {
            int status = run(options);
            hpx::finalize();
            return status;
        },
        hpx_argc, hpx_argv);
}
