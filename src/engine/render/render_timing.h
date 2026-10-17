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
        if constexpr (requires { stats.composition_ms; })
            for (size_t i = 0; i < 5; ++i) composition[i] += stats.composition_ms[i];
        if constexpr (requires { stats.lighting_ms; })
            for (size_t i = 0; i < 3; ++i) lighting[i] += stats.lighting_ms[i];
        for (size_t i = 0; i < 6; ++i) { passes[i] += stats.pass_ms[i]; parts[i] += stats.part_ms[i]; }
    }
    template <typename Stats> void AddCpu(const Stats& stats) {
        if constexpr (requires { stats.cpu_sample; }) {
            if (!stats.cpu_sample || stats.cpu_sample == last_cpu_sample_) return;
            last_cpu_sample_ = stats.cpu_sample;
        }
        ++cpu_samples;
        cpu_ms += stats.cpu_ms;
        reused += stats.shadow_tiles_reused;
        rendered += stats.shadow_tiles_rendered;
        shared += stats.shadow_tiles_shared;
        cache_redraws += stats.shadow_cache_redraws;
        shadow_draws += stats.shadow_draws;
        if constexpr (requires { stats.shadow_static_updates; }) {
            static_updates+=stats.shadow_static_updates; static_copies+=stats.shadow_static_copies;
            static_saved+=stats.shadow_static_saved_draws;
        }
        mirrors_occluded += stats.mirrors_occluded;
        if constexpr (requires { stats.scene_copies; }) scene_copies += stats.scene_copies;
        if constexpr (requires { stats.shadow_misses; })
            for (size_t i = 0; i < 4; ++i) shadow_misses[i] += stats.shadow_misses[i];
    }
    double GpuMean(double sum) const { return gpu_samples ? sum / double(gpu_samples) : 0.0; }
    double CpuMean(double sum) const { return cpu_samples ? sum / double(cpu_samples) : 0.0; }
    void Clear() {
        // Preserve the last sample number: a timing result is never counted twice
        // at a window boundary or while no new timestamp result is available.
        gpu_samples = cpu_samples = 0;
        gpu_ms = ray_ms = cpu_ms = reused = rendered = shared = cache_redraws = shadow_draws = mirrors_occluded = 0;
        static_updates=static_copies=static_saved=0;
        passes.fill(0); parts.fill(0); lighting.fill(0); shadow_misses.fill(0);
        composition.fill(0); scene_copies = 0;
    }
    void Reset() { Clear(); last_sample_ = last_cpu_sample_ = 0; }
    uint64_t gpu_samples = 0, cpu_samples = 0;
    double gpu_ms = 0, ray_ms = 0, cpu_ms = 0, reused = 0, rendered = 0, shared = 0, cache_redraws = 0, shadow_draws = 0, mirrors_occluded = 0;
    std::array<double, 6> passes{}, parts{};
    std::array<double, 5> composition{};
    double scene_copies = 0;
    std::array<double, 3> lighting{};
    std::array<double, 4> shadow_misses{};
    double static_updates=0, static_copies=0, static_saved=0;
private:
    uint64_t last_sample_ = 0, last_cpu_sample_ = 0;
};
}  // namespace pt
