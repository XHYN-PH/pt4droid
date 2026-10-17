#pragma once
#include <algorithm>
#include <cmath>
#include <span>

namespace pt {
// Only cross draws with disjoint depth intervals. Intersecting/coplanar draws
// retain their authored order, including depth ties. Bounded insertion avoids
// quadratic work for large scenes; decals and transparency never use this.
template<class T, class Interval>
void OrderOpaque(std::span<T> draws, Interval interval) {
    for (size_t i = 1; i < draws.size(); ++i) {
        size_t j = i;
        const auto current = interval(draws[i]);
        if (!std::isfinite(current.first) || !std::isfinite(current.second)) continue;
        for (unsigned moves = 0; j && moves < 8; ++moves) {
            const auto previous = interval(draws[j - 1]);
            if (!std::isfinite(previous.first) || !(current.second < previous.first)) break;
            std::swap(draws[j], draws[j - 1]);
            --j;
        }
    }
}
}
