#ifndef TILE_CACHE_HPP
#define TILE_CACHE_HPP

// HPX tasks compute the tiles and report each finished one through a channel.
// All functions of TileCache must be called from the same thread.

#include "app.hpp"
#include "tiles.hpp"

#include <hpx/channel.hpp>
#include <hpx/future.hpp>

#include <algorithm>
#include <map>
#include <memory>
#include <set>
#include <utility>

constexpr Pixel BACKGROUND{40, 40, 40};  // pixels waiting for their tile
constexpr std::size_t CACHE_SCREENS = 4; // how many screens of tiles are kept

// Column and row of a tile on the grid.
using Key = std::pair<long long, long long>;

// The area covered by both a and b. Its size is zero if they are apart.
inline Rect overlap(const Rect &a, const Rect &b) {
    long long left = std::max(a.left, b.left);
    long long top = std::max(a.top, b.top);
    long long right = std::min(a.left + a.width, b.left + b.width);
    long long bottom = std::min(a.top + a.height, b.top + b.height);
    return {left, top, static_cast<int>(std::max(right - left, 0LL)),
            static_cast<int>(std::max(bottom - top, 0LL))};
}

template <typename Function>
void for_each_pixel(const Rect &rect, Function function) {
    for (long long y = rect.top; y < rect.top + rect.height; ++y) {
        for (long long x = rect.left; x < rect.left + rect.width; ++x) {
            function(x, y);
        }
    }
}

// Computes the tiles in view, keeps finished ones for later and holds the
// picture shown on screen.
class TileCache {
  public:
    explicit TileCache(const Options &options)
        : options(options), view(options.view), color(options.color),
          pixels(static_cast<std::size_t>(options.width) * options.height,
                 BACKGROUND),
          next(finished.get()) {}

    ~TileCache() {
        cancel([](const Key &, const Tile &) { return true; });
    }

    // Draws the tiles already computed and starts on the others.
    void show(const View &new_view) {
        // Old tiles fit only if the zoom and iteration limit are the same.
        bool reusable = options.reuse && new_view.spacing == view.spacing &&
                        new_view.max_iter == view.max_iter;
        move_picture(new_view);
        view = new_view;

        visible.clear();
        for_each_tile(view.area, options.tile, [this](const Rect &part) {
            visible.insert({floor_div(part.left, options.tile),
                            floor_div(part.top, options.tile)});
        });

        // Stops all old tiles, or with reuse the unfinished ones out of view.
        cancel([&](const Key &key, const Tile &tile) {
            return !reusable || (!tile.done.is_ready() && !visible.count(key));
        });

        ++frame;
        for (const Key &key : visible) {
            auto entry = tiles.find(key);
            if (entry == tiles.end()) {
                launch(key);
                continue;
            }
            entry->second.shown = frame;
            if (entry->second.done.is_ready()) {
                draw(key, entry->second);
            }
        }
        limit_cache();
    }

    void set_color(Color new_color) {
        color = new_color;
        for (auto &[key, tile] : tiles) {
            if (tile.done.is_ready()) {
                draw(key, tile);
            }
        }
    }

    // Draws the tiles that finished since the last call. Returns true if the
    // picture changed.
    bool receive() {
        bool changed = false;
        while (next.is_ready()) {
            auto entry = tiles.find(next.get());
            next = finished.get();
            // The tile may have been removed after it sent its key.
            if (entry != tiles.end() && entry->second.done.is_ready()) {
                draw(entry->first, entry->second);
                changed = true;
            }
        }
        return changed;
    }

    // True once every tile in view has been drawn.
    bool complete() const {
        return std::all_of(
            visible.begin(), visible.end(),
            [this](const Key &key) { return tiles.at(key).drawn; });
    }

    // Escape counts of the view. Call it once complete() returns true.
    std::vector<int> counts() const {
        std::vector<int> result(pixels.size());
        Canvas screen{result.data(), view.area};
        for (const auto &[key, tile] : tiles) {
            Canvas canvas{tile.counts->data(), tile_area(key)};
            for_each_pixel(overlap(canvas.area, view.area),
                           [&](long long x, long long y) {
                               screen.at(x, y) = canvas.at(x, y);
                           });
        }
        return result;
    }

    const std::vector<Pixel> &picture() const { return pixels; }

    int tiles_computed() const { return computed; }

  private:
    struct Tile {
        std::shared_ptr<std::vector<int>> counts; // shared with the tasks
        hpx::stop_source stop;
        hpx::shared_future<void> done; // ready when the whole tile is
        bool drawn = false;
        int shown = 0; // the last frame that had this tile in view
    };

    // The rectangle of grid points that a tile covers.
    Rect tile_area(const Key &key) const {
        return {key.first * options.tile, key.second * options.tile,
                options.tile, options.tile};
    }

    // Starts computing a tile and stores it with the others.
    void launch(const Key &key) {
        Rect area = tile_area(key);
        Tile &tile = tiles[key];
        tile.shown = frame;
        tile.counts = std::make_shared<std::vector<int>>(
            static_cast<std::size_t>(area.width) * area.height);

        hpx::future<void> work =
            hpx::async(render_tile, view, Canvas{tile.counts->data(), area},
                       area, options.min_tile, tile.stop.get_token());
        tile.done = work.share();
        // Holding counts here stops the memory being freed while tasks still
        // write to it.
        tile.done.then([counts = tile.counts, channel = finished,
                        key](auto &&) mutable { channel.set(key); });
        ++computed;
    }

    // Stops the tiles that match and removes them.
    template <typename Predicate> void cancel(Predicate matches) {
        for (auto entry = tiles.begin(); entry != tiles.end();) {
            if (matches(entry->first, entry->second)) {
                entry->second.stop.request_stop();
                entry = tiles.erase(entry);
            } else {
                ++entry;
            }
        }
    }

    // If there are too many tiles, removes those out of view the longest.
    void limit_cache() {
        while (tiles.size() > CACHE_SCREENS * visible.size()) {
            tiles.erase(std::min_element(
                tiles.begin(), tiles.end(), [](const auto &a, const auto &b) {
                    return a.second.shown < b.second.shown;
                }));
        }
    }

    // Colors in the part of a finished tile that is on screen.
    void draw(const Key &key, Tile &tile) {
        Canvas canvas{tile.counts->data(), tile_area(key)};
        for_each_pixel(
            overlap(canvas.area, view.area), [&](long long x, long long y) {
                pixels[(y - view.area.top) * view.area.width +
                       (x - view.area.left)] =
                    color_pixel(canvas.at(x, y), view.max_iter, color);
            });
        tile.drawn = true;
    }

    // Moves and resizes the old picture to fit the new view, to be shown until
    // the tiles finish.
    void move_picture(const View &to) {
        std::vector<Pixel> old = pixels;
        double scale = to.spacing / view.spacing;
        std::vector<long long> columns(to.area.width);
        for (int x = 0; x < to.area.width; ++x) {
            columns[x] =
                std::llround((to.area.left + x) * scale) - view.area.left;
        }
        for (int y = 0; y < to.area.height; ++y) {
            long long row =
                std::llround((to.area.top + y) * scale) - view.area.top;
            for (int x = 0; x < to.area.width; ++x) {
                bool inside = columns[x] >= 0 && columns[x] < view.area.width &&
                              row >= 0 && row < view.area.height;
                pixels[y * to.area.width + x] =
                    inside ? old[row * view.area.width + columns[x]]
                           : BACKGROUND;
            }
        }
    }

    const Options &options;
    View view;
    Color color;
    std::vector<Pixel> pixels;
    // Tiles kept for reuse, including ones still being computed.
    std::map<Key, Tile> tiles;
    std::set<Key> visible;                   // tiles in view
    hpx::lcos::local::channel<Key> finished; // tiles whose tasks are done
    hpx::future<Key> next;                   // the next key from finished
    int frame = 0;                           // views shown so far
    int computed = 0;                        // tiles started so far
};

#endif
