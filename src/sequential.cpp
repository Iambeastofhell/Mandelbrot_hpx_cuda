#include "app.hpp"

static void render(const Options &options, std::vector<int> &counts) {
    const View &view = options.view;
    compute(view, view.area, {counts.data(), view.area});
}

int main(int argc, char *argv[]) {
    Options options = parse_options(argc, argv);
    return run(options, "Sequential", render);
}
