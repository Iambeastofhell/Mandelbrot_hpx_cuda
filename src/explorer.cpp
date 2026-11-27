// The window of the explorer. The main thread runs it while HPX computes the
// tiles in the background.

#include "app.hpp"
#include "tile_cache.hpp"

#include <SDL.h>
#include <hpx/future.hpp>
#include <hpx/hpx_start.hpp>

#include <algorithm>
#include <chrono>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>

constexpr int MAX_DOUBLED_ITER = 1 << 20; // highest limit the Up key reaches

// The view written as command line options.
static std::string describe(const View &view) {
    std::ostringstream out;
    out.precision(17);
    out << "--center " << (view.area.left + view.area.width / 2) * view.spacing
        << ' ' << -(view.area.top + view.area.height / 2) * view.spacing
        << " --view-width " << view.spacing * view.area.width << " --maxiter "
        << view.max_iter;
    return out.str();
}

// What the mouse and keyboard change.
struct Controls {
    View view;
    Color color;
    bool running = true;
};

// Zooms, keeping the point at screen position (x, y) where it is.
static void zoom(View &view, double factor, int x, int y) {
    double spacing = std::clamp(view.spacing * factor, 2 * MIN_SPACING,
                                MAX_VIEW_WIDTH / 2 / view.area.width);
    double scale = view.spacing / spacing;
    view.area.left = std::llround((view.area.left + x) * scale) - x;
    view.area.top = std::llround((view.area.top + y) * scale) - y;
    view.spacing = spacing;
}

// Keeps both center coordinates between -4 and 4.
static void limit_center(View &view) {
    auto limit = static_cast<long long>(4.0 / view.spacing);
    int half_width = view.area.width / 2;
    int half_height = view.area.height / 2;
    view.area.left =
        std::clamp(view.area.left, -limit - half_width, limit - half_width);
    view.area.top =
        std::clamp(view.area.top, -limit - half_height, limit - half_height);
}

static void apply(const SDL_Event &event, const Options &options,
                  Controls &controls) {
    View &view = controls.view;

    if (event.type == SDL_QUIT) {
        controls.running = false;
    } else if (event.type == SDL_MOUSEMOTION &&
               (event.motion.state & SDL_BUTTON_LMASK)) {
        view.area.left -= event.motion.xrel;
        view.area.top -= event.motion.yrel;
    } else if (event.type == SDL_MOUSEWHEEL) {
        int x = 0;
        int y = 0;
        SDL_GetMouseState(&x, &y);
        zoom(view, std::pow(0.8, event.wheel.preciseY), x, y);
    } else if (event.type == SDL_KEYDOWN) {
        SDL_Keycode key = event.key.keysym.sym;
        if (key == SDLK_ESCAPE || key == SDLK_q) {
            controls.running = false;
        } else if (key == SDLK_UP && view.max_iter <= MAX_DOUBLED_ITER / 2) {
            view.max_iter *= 2;
        } else if (key == SDLK_DOWN && view.max_iter > 1) {
            view.max_iter /= 2;
        } else if (key == SDLK_c) {
            controls.color =
                static_cast<Color>((static_cast<int>(controls.color) + 1) % 3);
        } else if (key == SDLK_r) {
            view = options.view;
        }
    }

    limit_center(view);
}

static bool same_view(const View &a, const View &b) {
    return a.area.left == b.area.left && a.area.top == b.area.top &&
           a.spacing == b.spacing && a.max_iter == b.max_iter;
}

// Handles events, draws finished tiles and shows the view in the window title.
static void run_window(const Options &options, SDL_Window *window,
                       SDL_Renderer *renderer, SDL_Texture *texture) {
    TileCache cache(options);
    Controls controls{options.view, options.color};
    cache.show(controls.view);
    auto started = std::chrono::steady_clock::now();
    bool computing = true;

    while (controls.running) {
        Controls before = controls;
        bool redraw = false;
        SDL_Event event;
        if (SDL_WaitEventTimeout(&event, 5)) {
            do {
                apply(event, options, controls);
                redraw = redraw || event.type == SDL_WINDOWEVENT;
            } while (SDL_PollEvent(&event));
        }

        if (!same_view(before.view, controls.view)) {
            cache.show(controls.view);
            started = std::chrono::steady_clock::now();
            computing = true;
            redraw = true;
        }
        if (before.color != controls.color) {
            cache.set_color(controls.color);
            redraw = true;
        }
        redraw = cache.receive() || redraw;

        if (redraw) {
            SDL_UpdateTexture(texture, nullptr, cache.picture().data(),
                              options.width * sizeof(Pixel));
            SDL_RenderCopy(renderer, texture, nullptr, nullptr);
            SDL_RenderPresent(renderer);
        }
        if (computing && cache.complete()) {
            std::chrono::duration<double, std::milli> elapsed =
                std::chrono::steady_clock::now() - started;
            std::string title = describe(controls.view) + "  (" +
                                std::to_string(std::lround(elapsed.count())) +
                                " ms)";
            SDL_SetWindowTitle(window, title.c_str());
            computing = false;
        }
    }
}

static int explore(const Options &options) {
    SDL_Window *window = nullptr;
    SDL_Renderer *renderer = nullptr;
    SDL_Texture *texture = nullptr;

    if (SDL_Init(SDL_INIT_VIDEO) == 0) {
        window = SDL_CreateWindow("Mandelbrot", SDL_WINDOWPOS_CENTERED,
                                  SDL_WINDOWPOS_CENTERED, options.width,
                                  options.height, 0);
    }
    if (window) {
        renderer = SDL_CreateRenderer(window, -1, 0);
    }
    if (renderer) {
        texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB24,
                                    SDL_TEXTUREACCESS_STREAMING, options.width,
                                    options.height);
    }

    if (texture) {
        run_window(options, window, renderer, texture);
    } else {
        std::cerr << "SDL error: " << SDL_GetError() << '\n';
    }

    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return texture ? 0 : 1;
}

static void wait_until_complete(TileCache &cache) {
    while (!cache.complete()) {
        if (!cache.receive()) {
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }
    }
}

// Moves the view around a square and times it, leaving out the first picture.
static int replay(const Options &options) {
    const int directions[4][2] = {{1, 0}, {0, 1}, {-1, 0}, {0, -1}};
    TileCache cache(options);
    View view = options.view;
    cache.show(view);
    wait_until_complete(cache);

    int first = cache.tiles_computed();
    auto start = std::chrono::steady_clock::now();
    for (int pan = 0; pan < options.pans; ++pan) {
        const int *direction = directions[(pan / 4) % 4];
        view.area.left += direction[0] * (view.area.width / 8);
        view.area.top += direction[1] * (view.area.height / 8);
        cache.show(view);
        wait_until_complete(cache);
    }
    std::chrono::duration<double> elapsed =
        std::chrono::steady_clock::now() - start;

    std::cout << "Explorer generation time: " << elapsed.count() << " seconds\n"
              << "Tiles computed: " << cache.tiles_computed() - first << '\n'
              << "Final view: " << describe(view) << '\n';
    return save_results(options, cache.counts()) ? 0 : 1;
}

int main(int argc, char *argv[]) {
    Options options = parse_options(argc, argv);

    std::string threads_option =
        "--hpx:threads=" + std::to_string(options.threads);
    char *hpx_argv[] = {argv[0], threads_option.data(), nullptr};
    int hpx_argc = options.threads > 0 ? 2 : 1;

    // HPX runs in the background, which leaves the main thread to the window.
    // SDL handles signals, so that Ctrl+C closes the window.
    hpx::init_params params;
    params.cfg = {"hpx.handle_signals=0"};
    if (!hpx::start(nullptr, hpx_argc, hpx_argv, params)) {
        return 1;
    }
    int status = options.pans > 0 ? replay(options) : explore(options);
    hpx::post([]() { hpx::finalize(); });
    hpx::stop();
    return status;
}
