#include "app.hpp"
#include "tiles.hpp"

#include <hpx/algorithm.hpp>
#include <hpx/execution.hpp>
#include <hpx/future.hpp>
#include <hpx/hpx_init.hpp>

// Computes the image with one task for each pixel, row or tile.
static void render_tasks(const Options &options, const Canvas &canvas) {
    const View &view = options.view;
    std::vector<hpx::future<void>> tasks;
    auto launch = [&](const Rect &rect) {
        tasks.push_back(hpx::async(
            [&view, canvas, rect]() { compute(view, rect, canvas); }));
    };

    if (options.method == "row") {
        for (int row = 0; row < view.area.height; ++row) {
            launch({view.area.left, view.area.top + row, view.area.width, 1});
        }
    } else {
        for_each_tile(view.area, options.method == "pixel" ? 1 : options.tile,
                      launch);
    }

    hpx::wait_all(tasks);
}

// Computes the image with a parallel loop over the pixels, which HPX divides
// into tasks.
static void render_loop(const Options &options, const Canvas &canvas) {
    const View &view = options.view;
    const Rect &area = view.area;
    int pixels = area.width * area.height;
    auto body = [&](int i) {
        long long x = area.left + i % area.width;
        long long y = area.top + i / area.width;
        canvas.at(x, y) = pixel_count(view, x, y);
    };

    if (options.chunk > 0) {
        hpx::execution::experimental::static_chunk_size chunk(options.chunk);
        hpx::experimental::for_loop(hpx::execution::par.with(chunk), 0, pixels,
                                    body);
    } else {
        hpx::experimental::for_loop(hpx::execution::par, 0, pixels, body);
    }
}

// Computes the image with one task for each tile, which creates more tasks
// where it has to split.
static void render_border(const Options &options, const Canvas &canvas) {
    std::vector<hpx::future<void>> tiles;
    for_each_tile(options.view.area, options.tile, [&](const Rect &tile) {
        tiles.push_back(hpx::async(render_tile, options.view, canvas, tile,
                                   options.min_tile, hpx::stop_token{}));
    });

    hpx::wait_all(tiles);
}

static void render(const Options &options, std::vector<int> &counts) {
    Canvas canvas{counts.data(), options.view.area};

    if (options.method == "loop") {
        render_loop(options, canvas);
    } else if (options.method == "border") {
        render_border(options, canvas);
    } else {
        render_tasks(options, canvas);
    }
}

int main(int argc, char *argv[]) {
    Options options = parse_options(argc, argv);

    // HPX takes its settings as command line arguments.
    std::vector<std::string> arguments = {argv[0]};
    if (options.threads > 0) {
        arguments.push_back("--hpx:threads=" + std::to_string(options.threads));
    }
    arguments.insert(arguments.end(), options.hpx_options.begin(),
                     options.hpx_options.end());

    std::vector<char *> hpx_argv;
    for (std::string &argument : arguments) {
        hpx_argv.push_back(argument.data());
    }
    hpx_argv.push_back(nullptr);

    // Starts HPX, runs the function below inside it and returns its result.
    return hpx::init(
        [&options](hpx::program_options::variables_map &) {
            int status = run(options, "HPX", render);
            hpx::finalize();
            return status;
        },
        static_cast<int>(arguments.size()), hpx_argv.data());
}
