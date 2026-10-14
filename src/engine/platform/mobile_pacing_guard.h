#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace pt::android {
// A blocked presentation path must not silently turn a 60 FPS request into 30.
// Ignore loading spikes and require two stable windows with GPU headroom (or
// a demonstrably incompatible display interval) before using the native cap.
class PacingGuard {
public:
    void Reset() { *this = {}; }
    bool Observe(float interval_ms, float present_ms, int fps, float hz,
                 uint64_t swap_interval_ns, float gpu_ms) {
        if (fps <= 0 || !std::isfinite(hz) || hz <= 0) { Reset(); return false; }
        if (fps_ != fps || std::abs(hz_ - hz) > .5f) {
            Reset(); fps_ = fps; hz_ = hz;
        }
        if (!std::isfinite(interval_ms) || !std::isfinite(present_ms) ||
            interval_ms <= 0 || interval_ms > 250 || present_ms < 0) {
            samples_ = bad_windows_ = 0; interval_sum_ = present_sum_ = gpu_sum_ = 0; gpu_samples_ = 0;
            warmup_ = 20; return false;
        }
        if (warmup_ > 0) { --warmup_; return false; }
        interval_sum_ += interval_ms; present_sum_ += present_ms;
        if (std::isfinite(gpu_ms) && gpu_ms > 0) { gpu_sum_ += gpu_ms; ++gpu_samples_; }
        if (++samples_ < std::clamp(fps / 2, 15, 60)) return false;
        const float budget = 1000.f / fps;
        const float multiple = std::round(hz / fps);
        const bool compatible = multiple >= 1 && std::abs(hz - multiple * fps) < .5f;
        const bool headroom = gpu_samples_ >= samples_ / 2 && gpu_sum_ / gpu_samples_ < budget * .85f;
        const bool incompatible_interval = !compatible && swap_interval_ns > (1000000000.0 / fps) * 1.2;
        const bool blocked = interval_sum_ / samples_ > budget * 1.35f &&
                             present_sum_ / samples_ > budget * .75f && (headroom || incompatible_interval);
        bad_windows_ = blocked ? bad_windows_ + 1 : 0;
        samples_ = gpu_samples_ = 0; interval_sum_ = present_sum_ = gpu_sum_ = 0;
        return bad_windows_ >= 2;
    }
private:
    int fps_ = 0, samples_ = 0, gpu_samples_ = 0, bad_windows_ = 0, warmup_ = 20;
    float hz_ = 0, interval_sum_ = 0, present_sum_ = 0, gpu_sum_ = 0;
};
}
