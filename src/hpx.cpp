#include "app.hpp"

#include <hpx/future.hpp>
#include <hpx/hpx_init.hpp>

#include <algorithm>

static void render(const Options& options, std::vector<int>& counts) {
    const View& view = options.view;
    const Rect& area = view.area;
    int total = area.width * area.height;
    int grain = options.grain > 0 ? options.grain : area.width;

    std::vector<hpx::future<void>> tasks;
    tasks.reserve(static_cast<std::size_t>((total + grain - 1) / grain));

    for (int begin = 0; begin < total; begin += grain) {
        int end = std::min(begin + grain, total);
        tasks.push_back(hpx::async([&view, &area, &counts, begin, end]() {
            for (int i = begin; i < end; ++i) {
                counts[i] = pixel_count(view, area.left + i % area.width, area.top + i / area.width);
            }
        }));
    }

    hpx::wait_all(tasks);
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
