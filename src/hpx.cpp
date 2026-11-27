#include "app.hpp"
#include "tiles.hpp"

#include <hpx/algorithm.hpp>
#include <hpx/execution.hpp>
#include <hpx/future.hpp>
#include <hpx/hpx_init.hpp>

// One task for each pixel, row or tile.
static void render_tasks(const Options& options, const Canvas& canvas) {
    const View& view = options.view;
    std::vector<hpx::future<void>> tasks;
    auto launch = [&](const Rect& rect) {
        tasks.push_back(hpx::async([&view, canvas, rect]() { compute(view, rect, canvas); }));
    };

    if (options.method == "row") {
        for (int row = 0; row < view.area.height; ++row) {
            launch({view.area.left, view.area.top + row, view.area.width, 1});
        }
    } else {
        for_each_tile(view.area, options.method == "pixel" ? 1 : options.tile, launch);
    }

    hpx::wait_all(tasks);
}

// A parallel loop over the pixels, with HPX dividing them into tasks.
static void render_loop(const Options& options, const Canvas& canvas) {
    const View& view = options.view;
    const Rect& area = view.area;
    int pixels = area.width * area.height;
    auto body = [&](int i) {
        long long x = area.left + i % area.width;
        long long y = area.top + i / area.width;
        canvas.at(x, y) = pixel_count(view, x, y);
    };

    if (options.chunk > 0) {
        hpx::execution::experimental::static_chunk_size chunk(options.chunk);
        hpx::experimental::for_loop(hpx::execution::par.with(chunk), 0, pixels, body);
    } else {
        hpx::experimental::for_loop(hpx::execution::par, 0, pixels, body);
    }
}

static void render(const Options& options, std::vector<int>& counts) {
    Canvas canvas{counts.data(), options.view.area};

    if (options.method == "loop") {
        render_loop(options, canvas);
    } else {
        render_tasks(options, canvas);
    }
}

int main(int argc, char* argv[]) {
    Options options = parse_options(argc, argv);

    std::string threads_option = "--hpx:threads=" + std::to_string(options.threads);
    char* hpx_argv[] = {argv[0], threads_option.data(), nullptr};
    int hpx_argc = options.threads > 0 ? 2 : 1;

    return hpx::init(
        [&options](hpx::program_options::variables_map&) {
            int status = run(options, "HPX", render);
            hpx::finalize();
            return status;
        },
        hpx_argc, hpx_argv);
}
