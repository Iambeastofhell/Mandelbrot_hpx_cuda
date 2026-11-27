#ifndef TILES_HPP
#define TILES_HPP

#include "mandelbrot.hpp"

#include <hpx/future.hpp>
#include <hpx/stop_token.hpp>

#include <algorithm>
#include <vector>

// Divides and rounds down, for negative values too. size must be positive.
inline long long floor_div(long long value, long long size) {
    return value / size - (value % size < 0);
}

// Cuts area along the edges of a fixed grid of size x size tiles and calls
// function with each piece.
template <typename Function>
void for_each_tile(const Rect &area, int size, Function function) {
    long long right = area.left + area.width;
    long long bottom = area.top + area.height;

    for (long long y = floor_div(area.top, size) * size; y < bottom;
         y += size) {
        for (long long x = floor_div(area.left, size) * size; x < right;
             x += size) {
            long long left = std::max(x, area.left);
            long long top = std::max(y, area.top);
            function(Rect{left, top,
                          static_cast<int>(std::min(x + size, right) - left),
                          static_cast<int>(std::min(y + size, bottom) - top)});
        }
    }
}

// Sets the count of every point in rect. canvas.area must contain rect.
inline void fill(const Rect &rect, int count, const Canvas &canvas) {
    for (long long y = rect.top; y < rect.top + rect.height; ++y) {
        for (long long x = rect.left; x < rect.left + rect.width; ++x) {
            canvas.at(x, y) = count;
        }
    }
}

// Computes the pixels on the edge of tile. Returns their count if they all
// match, otherwise -1.
inline int compute_border(const View &view, const Rect &tile,
                          const Canvas &canvas) {
    long long right = tile.left + tile.width - 1;
    long long bottom = tile.top + tile.height - 1;
    int first = pixel_count(view, tile.left, tile.top);
    bool same = true;
    auto visit = [&](long long x, long long y) {
        canvas.at(x, y) = pixel_count(view, x, y);
        same = same && canvas.at(x, y) == first;
    };

    for (long long x = tile.left; x <= right; ++x) {
        visit(x, tile.top);
        visit(x, bottom);
    }
    for (long long y = tile.top + 1; y < bottom; ++y) {
        visit(tile.left, y);
        visit(right, y);
    }
    return same ? first : -1;
}

// Computes a tile with the Mariani-Silver method. If every pixel on its edge
// has the same count, the inside is filled with that count. Otherwise the
// inside is split into four smaller tiles, each computed by a new task. The
// returned future is ready when the whole tile is done. Each pixel is written
// by one task. min_size must be at least 3.
inline hpx::future<void> render_tile(View view, Canvas canvas, Rect tile,
                                     int min_size, hpx::stop_token stop) {
    if (stop.stop_requested()) {
        return hpx::make_ready_future();
    }
    if (tile.width < min_size || tile.height < min_size) {
        compute(view, tile, canvas);
        return hpx::make_ready_future();
    }

    int count = compute_border(view, tile, canvas);
    Rect inner{tile.left + 1, tile.top + 1, tile.width - 2, tile.height - 2};

    // An edge with one count below max_iter could go all the way round the
    // set. A tile that does this contains the point 0, as the set does.
    bool has_zero = tile.left <= 0 && tile.left + tile.width > 0 &&
                    tile.top <= 0 && tile.top + tile.height > 0;
    if (count == view.max_iter || (count >= 0 && !has_zero)) {
        fill(inner, count, canvas);
        return hpx::make_ready_future();
    }

    int width = inner.width / 2;
    int height = inner.height / 2;
    const Rect quadrants[] = {
        {inner.left, inner.top, width, height},
        {inner.left + width, inner.top, inner.width - width, height},
        {inner.left, inner.top + height, width, inner.height - height},
        {inner.left + width, inner.top + height, inner.width - width,
         inner.height - height},
    };

    std::vector<hpx::future<void>> children;
    for (const Rect &child : quadrants) {
        if (child.width > 0 && child.height > 0) {
            children.push_back(
                hpx::async(render_tile, view, canvas, child, min_size, stop));
        }
    }
    return hpx::when_all(std::move(children)).then([](auto &&done) {
        for (auto &child : done.get()) {
            child.get();
        }
    });
}

#endif
