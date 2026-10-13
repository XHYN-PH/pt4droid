#pragma once
#include <array>
#include <cstddef>
#include <cstdint>

namespace pt {
class RenderTimingWindow {
public:
    template <typename Stats> void AddGpu(const Stats& stats) {
        if (!stats.gpu_timing_valid || stats.gpu_sample == last_sample_) return;
        last_sample_ = stats.gpu_sample;
        ++gpu_samples;
        gpu_ms += stats.gpu_ms;
        ray_ms += stats.ray_setup_ms;
        for (size_t i = 0; i < 6; ++i) { passes[i] += stats.pass_ms[i]; parts[i] += stats.part_ms[i]; }
    }
    template <typename Stats> void AddCpu(const Stats& stats) {
        ++cpu_samples;
        cpu_ms += stats.cpu_ms;
        reused += stats.shadow_tiles_reused;
        rendered += stats.shadow_tiles_rendered;
        shared += stats.shadow_tiles_shared;
        cache_redraws += stats.shadow_cache_redraws;
        shadow_draws += stats.shadow_draws;
        mirrors_occluded += stats.mirrors_occluded;
    }
    double GpuMean(double sum) const { return gpu_samples ? sum / double(gpu_samples) : 0.0; }
    double CpuMean(double sum) const { return cpu_samples ? sum / double(cpu_samples) : 0.0; }
    void Clear() {
        // Preserve the last sample number: a timing result is never counted twice
        // at a window boundary or while no new timestamp result is available.
        gpu_samples = cpu_samples = 0;
        gpu_ms = ray_ms = cpu_ms = reused = rendered = shared = cache_redraws = shadow_draws = mirrors_occluded = 0;
        passes.fill(0); parts.fill(0);
    }
    void Reset() { Clear(); last_sample_ = 0; }
    uint64_t gpu_samples = 0, cpu_samples = 0;
    double gpu_ms = 0, ray_ms = 0, cpu_ms = 0, reused = 0, rendered = 0, shared = 0, cache_redraws = 0, shadow_draws = 0, mirrors_occluded = 0;
    std::array<double, 6> passes{}, parts{};
private:
    uint64_t last_sample_ = 0;
};
}  // namespace pt
