#ifndef TILES_HPP
#define TILES_HPP

#include "mandelbrot.hpp"

#include <algorithm>

// Rounds towards negative infinity, as grid positions can be negative.
inline long long floor_div(long long value, long long size) {
    return value / size - (value % size < 0);
}

// Calls function with every tile of the fixed grid that overlaps area, clipped to area.
template <typename Function>
void for_each_tile(const Rect& area, int size, Function function) {
    long long right = area.left + area.width;
    long long bottom = area.top + area.height;

    for (long long y = floor_div(area.top, size) * size; y < bottom; y += size) {
        for (long long x = floor_div(area.left, size) * size; x < right; x += size) {
            long long left = std::max(x, area.left);
            long long top = std::max(y, area.top);
            function(Rect{left, top, static_cast<int>(std::min(x + size, right) - left),
                          static_cast<int>(std::min(y + size, bottom) - top)});
        }
    }
}

#endif
