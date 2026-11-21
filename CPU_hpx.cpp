#include "mandelbrot_common.hpp"

#include <hpx/hpx_init.hpp>

#include <chrono>
#include <iostream>
#include <string>
#include <vector>

static Pixel pixel_color(int iter, const AppOptions& options) {
    if (options.gradient) {
        return color_gradient(iter, options.max_iter);
    }
    if (options.grayscale) {
        return color_grayscale(iter, options.max_iter);
    }
    return color_black_and_white(iter, options.max_iter);
}

static void generate_mandelbrot(const AppOptions& options, Image& image) {
    std::vector<hpx::future<void>> tasks;

    int task_count = options.threads > 0
        ? options.threads
        : hpx::get_os_thread_count();

    if (task_count < 1) {
        task_count = 1;
    }

    tasks.reserve(static_cast<std::size_t>(task_count));

    for (int task = 0; task < task_count; ++task) {
        int row_start = task * options.height / task_count;
        int row_end = (task + 1) * options.height / task_count;

        tasks.push_back(hpx::async([&, row_start, row_end]() {
            for (int y = row_start; y < row_end; ++y) {
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
        }));
    }

    hpx::wait_all(tasks);
}

int hpx_main(int argc, char* argv[]) {
    AppOptions options;
    std::string error;

    if (!parse_arguments(argc, argv, options, error)) {
        if (error.empty()) {
            return hpx::finalize();
        }
        std::cerr << "Error: " << error << '\n';
        print_usage(argv[0]);
        hpx::finalize();
        return 1;
    }

    if (!validate_arguments(options, error)) {
        std::cerr << "Error: " << error << '\n';
        print_usage(argv[0]);
        hpx::finalize();
        return 1;
    }

    Image image(0, 0);

    if (!options.load_file.empty()) {
        bool loaded = false;
        image = load_ppm(options.load_file, loaded);
        if (!loaded) {
            hpx::finalize();
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

    if (!options.save_file.empty()) {
        if (!save_ppm(image, options.save_file)) {
            hpx::finalize();
            return 1;
        }
    }

    if (options.display) {
        display_image(image, options);
    }

    hpx::finalize();
    return 0;
}

int main(int argc, char* argv[]) {
    std::vector<std::string> arguments;
    arguments.reserve(static_cast<std::size_t>(argc) + 1);

    for (int i = 0; i < argc; ++i) {
        arguments.push_back(argv[i]);
        if (i + 1 < argc && std::string(argv[i]) == "--threads") {
            std::string thread_value = argv[i + 1];
            arguments.push_back(thread_value);
            arguments.push_back("--hpx:threads=" + thread_value);
            ++i;
        }
    }

    std::vector<char*> hpx_argv;
    hpx_argv.reserve(arguments.size());
    for (std::string& argument : arguments) {
        hpx_argv.push_back(argument.data());
    }

    return hpx::init(static_cast<int>(hpx_argv.size()), hpx_argv.data());
}
