#pragma once

#include <algorithm>
#include <cstdint>

namespace pt {
// Rendering, simulation and fence waits have already used part of this frame's budget.
// A late frame must continue immediately instead of sleeping for another full period.
inline int64_t FrameLimitWaitNs(int64_t elapsed_ns, int fps_limit) {
    if (fps_limit <= 0) return 0;
    const int64_t period = 1'000'000'000LL / fps_limit;
    return std::max<int64_t>(0, period - std::max<int64_t>(0, elapsed_ns));
}
}
