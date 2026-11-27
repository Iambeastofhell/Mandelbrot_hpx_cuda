#include "app.hpp"

static void render(const Options& options, std::vector<int>& counts) {
    const View& view = options.view;

    for (int y = 0; y < view.height; ++y) {
        for (int x = 0; x < view.width; ++x) {
            counts[y * view.width + x] = pixel_count(view, x, y);
        }
    }
}

int main(int argc, char* argv[]) {
    Options options = parse_options(argc, argv);
    return run(options, "Sequential", render);
}
