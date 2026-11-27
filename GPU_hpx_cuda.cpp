#include "mandelbrot_common.hpp"
#include "mandelbrot_kernel.h"

#include <hpx/future.hpp>
#include <hpx/hpx_init.hpp>

#include <algorithm>
#include <chrono>
#include <iostream>
#include <string>
#include <vector>

static void generate_mandelbrot(const AppOptions& options, Image& image) {
    int rows = strip_rows(options);
    MandelbrotView view = make_view(options);

    std::vector<hpx::future<void>> tasks;
    for (int row_begin = 0; row_begin < view.height; row_begin += rows) {
        int row_end = std::min(row_begin + rows, view.height);
        tasks.push_back(hpx::async([&image, view, row_begin, row_end]() {
            launch_mandelbrot_kernel(&image.pixels[row_begin * view.width], view, row_begin, row_end);
        }));
    }

    hpx::wait_all(tasks);
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
        cuda_startup();

        auto start = std::chrono::steady_clock::now();
        image = Image(options.width, options.height);
        generate_mandelbrot(options, image);
        auto end = std::chrono::steady_clock::now();

        std::chrono::duration<double> elapsed = end - start;
        std::cout << "HPX CUDA generation time: " << elapsed.count() << " seconds\n";
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