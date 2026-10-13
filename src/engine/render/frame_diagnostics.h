#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstddef>

namespace pt {
// Fixed storage and no hot-path sorting. Percentiles are calculated only when
// diagnostics are reported; each sample spans completed render-loop submissions, including limiter wait.
class FrameDiagnostics {
public:
    static constexpr size_t kCapacity = 2048;
    void Add(float interval_ms, const std::array<float, 5>& cpu) {
        if (!std::isfinite(interval_ms) || interval_ms <= 0) return;
        intervals_[next_] = interval_ms;
        next_ = (next_ + 1) % kCapacity;
        retained_ = std::min(retained_ + 1, kCapacity);
        ++samples;
        for (size_t i = 0; i < cpu.size(); ++i) parts[i] += cpu[i];
        worst_ms = std::max(worst_ms, interval_ms);
        over_33ms += interval_ms > 33.3334f;
    }
    std::array<float, 3> Percentiles() const {
        if (!retained_) return {};
        auto sorted = intervals_;
        std::sort(sorted.begin(), sorted.begin() + retained_);
        const auto pick = [&](float fraction) {
            return sorted[size_t(std::ceil(fraction * retained_)) - 1];
        };
        return {pick(.50f), pick(.95f), pick(.99f)};
    }
    void Clear() { *this = {}; }
    double Mean(size_t part) const { return samples ? parts[part] / samples : 0; }
    uint64_t samples = 0, over_33ms = 0;
    float worst_ms = 0;
    std::array<double, 5> parts{}; // acquire/fence wait, scene, VFX, recording, submit/present
private:
    std::array<float, kCapacity> intervals_{};
    size_t next_ = 0, retained_ = 0;
};
}
