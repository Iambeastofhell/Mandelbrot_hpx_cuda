#include "mandelbrot_common.hpp"
#include "mandelbrot_kernel.h"

#include <atomic>
#include <chrono>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

struct MandelbrotView;

extern "C" void stream_init(int num_streams, int width, int max_chunk_height);
extern "C" void stream_cleanup();
extern "C" void launch_mandelbrot_stream(Pixel* host_pixels, const MandelbrotView& view,
                                         int row_begin, int row_end, int stream_idx);
extern "C" void stream_synchronize(int stream_idx);

static void generate_mandelbrot(const AppOptions& options, Image& image) {
    int total_pixels = options.width * options.height;
    int grain = options.grain > 0 ? options.grain : options.width * options.height / 8;
    if (grain < options.width) grain = options.width;
    int num_tasks = (total_pixels + grain - 1) / grain;
    if (num_tasks < 1) num_tasks = 1;

    int num_threads = options.threads > 0 ? options.threads : std::thread::hardware_concurrency();
    if (num_threads > num_tasks) num_threads = num_tasks;
    if (num_threads < 1) num_threads = 1;

    int max_chunk_height = (grain + options.width - 1) / options.width;
    stream_init(num_threads, options.width, max_chunk_height);

    MandelbrotView view = make_view(options);

    std::vector<std::thread> threads;
    threads.reserve(num_threads);

    std::atomic<int> next_task{0};

    auto worker = [&](int stream_idx) {
        while (true) {
            int task_idx = next_task.fetch_add(1);
            if (task_idx >= num_tasks) break;

            int begin = task_idx * grain;
            int end = std::min(begin + grain, total_pixels);
            int row_begin = begin / options.width;
            int row_end = (end + options.width - 1) / options.width;

            launch_mandelbrot_stream(&image.pixels[row_begin * options.width], view,
                                     row_begin, row_end, stream_idx);
        }
    };

    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back(worker, i);
    }

    for (auto& t : threads) {
        t.join();
    }

    for (int i = 0; i < num_threads; ++i) {
        stream_synchronize(i);
    }

    stream_cleanup();
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
        std::cout << "Optimized Hybrid (streams) generation time: " << elapsed.count() << " seconds\n";
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

    return run(options);
}