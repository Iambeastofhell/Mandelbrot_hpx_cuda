// The main thread owns the window and the list of tiles. HPX tasks compute the tiles
// and report each finished one through a channel.

#include "app.hpp"
#include "tiles.hpp"

#include <SDL.h>
#include <hpx/channel.hpp>
#include <hpx/future.hpp>
#include <hpx/hpx_start.hpp>

#include <algorithm>
#include <chrono>
#include <iostream>
#include <map>
#include <memory>
#include <sstream>
#include <thread>
#include <utility>

constexpr Pixel BACKGROUND{40, 40, 40};
constexpr int MAX_DOUBLED_ITER = 1 << 20;

using Key = std::pair<long long, long long>;  // column and row of a tile on the grid

static Rect overlap(const Rect& a, const Rect& b) {
    long long left = std::max(a.left, b.left);
    long long top = std::max(a.top, b.top);
    long long right = std::min(a.left + a.width, b.left + b.width);
    long long bottom = std::min(a.top + a.height, b.top + b.height);
    return {left, top, static_cast<int>(std::max(right - left, 0LL)), static_cast<int>(std::max(bottom - top, 0LL))};
}

template <typename Function>
static void for_each_pixel(const Rect& rect, Function function) {
    for (long long y = rect.top; y < rect.top + rect.height; ++y) {
        for (long long x = rect.left; x < rect.left + rect.width; ++x) {
            function(x, y);
        }
    }
}

class Explorer {
public:
    explicit Explorer(const Options& options)
        : options(options), view(options.view), color(options.color),
          pixels(static_cast<std::size_t>(options.width) * options.height, BACKGROUND), next(finished.get()) {}

    ~Explorer() {
        cancel();
    }

    // Cancels the old work and starts on every tile of the new view.
    void show(const View& new_view) {
        cancel();
        move_picture(new_view);
        view = new_view;
        for_each_tile(view.area, options.tile, [this](const Rect& part) {
            launch({floor_div(part.left, options.tile), floor_div(part.top, options.tile)});
        });
    }

    void set_color(Color new_color) {
        color = new_color;
        for (auto& [key, tile] : tiles) {
            if (tile.done.is_ready()) {
                draw(key, tile);
            }
        }
    }

    // Draws the tiles reported since the last call. Returns true if the picture changed.
    bool receive() {
        bool changed = false;
        while (next.is_ready()) {
            auto entry = tiles.find(next.get());
            next = finished.get();
            // A report can be left over from a tile that has since been cancelled.
            if (entry != tiles.end() && entry->second.done.is_ready()) {
                draw(entry->first, entry->second);
                changed = true;
            }
        }
        return changed;
    }

    bool complete() const {
        return std::all_of(tiles.begin(), tiles.end(), [](const auto& entry) { return entry.second.drawn; });
    }

    // Escape counts of the view. Only valid once it is complete.
    std::vector<int> counts() const {
        std::vector<int> result(pixels.size());
        Canvas screen{result.data(), view.area};
        for (const auto& [key, tile] : tiles) {
            Canvas canvas{tile.counts->data(), tile_area(key)};
            for_each_pixel(overlap(canvas.area, view.area),
                           [&](long long x, long long y) { screen.at(x, y) = canvas.at(x, y); });
        }
        return result;
    }

    const std::vector<Pixel>& picture() const {
        return pixels;
    }

    int tiles_computed() const {
        return computed;
    }

private:
    struct Tile {
        std::shared_ptr<std::vector<int>> counts;
        hpx::stop_source stop;
        hpx::shared_future<void> done;
        bool drawn = false;
    };

    Rect tile_area(const Key& key) const {
        return {key.first * options.tile, key.second * options.tile, options.tile, options.tile};
    }

    void launch(const Key& key) {
        Rect area = tile_area(key);
        Tile& tile = tiles[key];
        tile.counts = std::make_shared<std::vector<int>>(static_cast<std::size_t>(area.width) * area.height);

        hpx::future<void> work = hpx::async(render_tile, view, Canvas{tile.counts->data(), area}, area,
                                            options.min_tile, tile.stop.get_token());
        tile.done = work.share();
        // The copy of counts keeps the buffer alive while the tasks still write to it.
        tile.done.then([counts = tile.counts, channel = finished, key](auto&&) mutable { channel.set(key); });
        ++computed;
    }

    void cancel() {
        for (auto& [key, tile] : tiles) {
            tile.stop.request_stop();
        }
        tiles.clear();
    }

    void draw(const Key& key, Tile& tile) {
        Canvas canvas{tile.counts->data(), tile_area(key)};
        for_each_pixel(overlap(canvas.area, view.area), [&](long long x, long long y) {
            pixels[(y - view.area.top) * view.area.width + (x - view.area.left)] =
                color_pixel(canvas.at(x, y), view.max_iter, color);
        });
        tile.drawn = true;
    }

    // Moves and scales the old picture to the new view, to stand in until the tiles arrive.
    void move_picture(const View& to) {
        std::vector<Pixel> old = pixels;
        double scale = to.spacing / view.spacing;
        for (int y = 0; y < to.area.height; ++y) {
            long long row = std::llround((to.area.top + y) * scale) - view.area.top;
            for (int x = 0; x < to.area.width; ++x) {
                long long column = std::llround((to.area.left + x) * scale) - view.area.left;
                bool inside = column >= 0 && column < view.area.width && row >= 0 && row < view.area.height;
                pixels[y * to.area.width + x] = inside ? old[row * view.area.width + column] : BACKGROUND;
            }
        }
    }

    const Options& options;
    View view;
    Color color;
    std::vector<Pixel> pixels;
    std::map<Key, Tile> tiles;
    hpx::lcos::local::channel<Key> finished;
    hpx::future<Key> next;
    int computed = 0;
};

// The view as renderer options, so that it can be copied.
static std::string describe(const View& view) {
    std::ostringstream out;
    out.precision(17);
    out << "--center " << (view.area.left + view.area.width / 2) * view.spacing << ' '
        << -(view.area.top + view.area.height / 2) * view.spacing << " --view-width " << view.spacing * view.area.width
        << " --maxiter " << view.max_iter;
    return out.str();
}

struct Controls {
    View view;
    Color color;
    bool running = true;
};

// Changes the pixel spacing, keeping the point at screen position (x, y) in place.
// The limits stay inside the renderer's, so every view shown can also be rendered.
static void zoom(View& view, double factor, int x, int y) {
    double spacing = std::clamp(view.spacing * factor, 2 * MIN_SPACING, MAX_VIEW_WIDTH / 2 / view.area.width);
    double scale = view.spacing / spacing;
    view.area.left = std::llround((view.area.left + x) * scale) - x;
    view.area.top = std::llround((view.area.top + y) * scale) - y;
    view.spacing = spacing;
}

// Keeps the center within the same limits as --center.
static void limit_center(View& view) {
    auto limit = static_cast<long long>(4.0 / view.spacing);
    int half_width = view.area.width / 2;
    int half_height = view.area.height / 2;
    view.area.left = std::clamp(view.area.left, -limit - half_width, limit - half_width);
    view.area.top = std::clamp(view.area.top, -limit - half_height, limit - half_height);
}

static void apply(const SDL_Event& event, const Options& options, Controls& controls) {
    View& view = controls.view;

    if (event.type == SDL_QUIT) {
        controls.running = false;
    } else if (event.type == SDL_MOUSEMOTION && (event.motion.state & SDL_BUTTON_LMASK)) {
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
            controls.color = static_cast<Color>((static_cast<int>(controls.color) + 1) % 3);
        } else if (key == SDLK_r) {
            view = options.view;
        }
    }

    limit_center(view);
}

static bool same_view(const View& a, const View& b) {
    return a.area.left == b.area.left && a.area.top == b.area.top && a.spacing == b.spacing && a.max_iter == b.max_iter;
}

static void run_window(const Options& options, SDL_Window* window, SDL_Renderer* renderer, SDL_Texture* texture) {
    Explorer explorer(options);
    Controls controls{options.view, options.color};
    explorer.show(controls.view);
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
            explorer.show(controls.view);
            started = std::chrono::steady_clock::now();
            computing = true;
            redraw = true;
        }
        if (before.color != controls.color) {
            explorer.set_color(controls.color);
            redraw = true;
        }
        redraw = explorer.receive() || redraw;

        if (redraw) {
            SDL_UpdateTexture(texture, nullptr, explorer.picture().data(), options.width * sizeof(Pixel));
            SDL_RenderCopy(renderer, texture, nullptr, nullptr);
            SDL_RenderPresent(renderer);
        }
        if (computing && explorer.complete()) {
            std::chrono::duration<double, std::milli> elapsed = std::chrono::steady_clock::now() - started;
            std::string title = describe(controls.view) + "  (" + std::to_string(std::lround(elapsed.count())) + " ms)";
            SDL_SetWindowTitle(window, title.c_str());
            computing = false;
        }
    }
}

static int explore(const Options& options) {
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    SDL_Texture* texture = nullptr;

    if (SDL_Init(SDL_INIT_VIDEO) == 0) {
        window = SDL_CreateWindow("Mandelbrot", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, options.width,
                                  options.height, 0);
    }
    if (window) {
        renderer = SDL_CreateRenderer(window, -1, 0);
    }
    if (renderer) {
        texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STREAMING, options.width,
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

static void wait_until_complete(Explorer& explorer) {
    while (!explorer.complete()) {
        if (!explorer.receive()) {
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }
    }
}

// Pans around a square without opening a window and times the frames after the first.
static int replay(const Options& options) {
    const int directions[4][2] = {{1, 0}, {0, 1}, {-1, 0}, {0, -1}};
    Explorer explorer(options);
    View view = options.view;
    explorer.show(view);
    wait_until_complete(explorer);

    int first = explorer.tiles_computed();
    auto start = std::chrono::steady_clock::now();
    for (int pan = 0; pan < options.pans; ++pan) {
        const int* direction = directions[(pan / 4) % 4];
        view.area.left += direction[0] * (view.area.width / 8);
        view.area.top += direction[1] * (view.area.height / 8);
        explorer.show(view);
        wait_until_complete(explorer);
    }
    std::chrono::duration<double> elapsed = std::chrono::steady_clock::now() - start;

    std::cout << "Explorer generation time: " << elapsed.count() << " seconds\n"
              << "Tiles computed: " << explorer.tiles_computed() - first << '\n'
              << "Final view: " << describe(view) << '\n';
    return save_results(options, explorer.counts()) ? 0 : 1;
}

int main(int argc, char* argv[]) {
    Options options = parse_options(argc, argv);

    std::string threads_option = "--hpx:threads=" + std::to_string(options.threads);
    char* hpx_argv[] = {argv[0], threads_option.data(), nullptr};
    int hpx_argc = options.threads > 0 ? 2 : 1;

    // The HPX runtime starts in the background, so the window stays on the main thread.
    // Signals are left alone so that SDL turns Ctrl+C into a normal quit.
    hpx::init_params params;
    params.cfg = {"hpx.handle_signals=0"};
    hpx::start(nullptr, hpx_argc, hpx_argv, params);
    int status = options.pans > 0 ? replay(options) : explore(options);
    hpx::post([]() { hpx::finalize(); });
    hpx::stop();
    return status;
}
