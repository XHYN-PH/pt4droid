#include "engine/core/bounded_retirement.h"
#include "engine/render/light_scissor.h"
#include "engine/render/opaque_order.h"
#include "engine/render/render_timing.h"
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <atomic>
#include <cassert>
#include <iostream>
#include <numeric>
#include <random>

int main() {
    std::mt19937 rng(17017);
    auto random = [&](float low, float high) { return std::uniform_real_distribution<float>(low, high)(rng); };
    const VkExtent2D extent{1152, 518};
    unsigned checked = 0, tightened = 0;
    for (unsigned trial = 0; trial < 12000; ++trial) {
        glm::mat4 projection = glm::perspectiveRH_ZO(1.2f, 2.22f, .05f, 40.0f);
        if (trial % 2) { projection[0][2] += .13f; projection[1][2] -= .07f; }
        glm::mat4 world = glm::translate(glm::mat4(1), glm::vec3(random(-8, 8), random(-5, 5), random(-20, 3)));
        world = glm::rotate(world, random(-3, 3), glm::normalize(glm::vec3(.3f, .7f, .2f)));
        world = glm::scale(world, glm::vec3(random(.01f, 4), random(.01f, 4), random(.01f, 6)));
        const glm::mat4 clip = projection * world;
        const auto bounds = pt::LightScissor(clip, extent);
        assert(bounds.offset.x >= 0 && bounds.offset.y >= 0);
        assert(uint32_t(bounds.offset.x) + bounds.extent.width <= extent.width);
        assert(uint32_t(bounds.offset.y) + bounds.extent.height <= extent.height);
        bool crossed = false;
        for (unsigned corner = 0; corner < 8; ++corner) {
            const auto p = clip * glm::vec4((corner & 1) ? 1 : -1, (corner & 2) ? 1 : -1, (corner & 4) ? 1 : -1, 1);
            crossed |= p.w <= 1e-5f || p.z < 0 || p.z > p.w;
        }
        if (crossed && uint64_t(bounds.extent.width) * bounds.extent.height < uint64_t(extent.width) * extent.height) ++tightened;
        // Interior samples, not just the vertices used by the implementation.
        for (int x = 0; x < 5; ++x) for (int y = 0; y < 5; ++y) for (int z = 0; z < 5; ++z) {
            const auto p = clip * glm::vec4(x * .5f - 1, y * .5f - 1, z * .5f - 1, 1);
            if (p.w <= 1e-4f || p.z < 0 || p.z > p.w) continue;
            const glm::vec2 q = glm::vec2(p) / p.w;
            if (q.x < -1 || q.x >= 1 || q.y < -1 || q.y >= 1) continue;
            const glm::vec2 pixel = (q * .5f + .5f) * glm::vec2(extent.width, extent.height);
            assert(pixel.x >= bounds.offset.x && pixel.y >= bounds.offset.y);
            assert(pixel.x < bounds.offset.x + bounds.extent.width && pixel.y < bounds.offset.y + bounds.extent.height);
            ++checked;
        }
    }
    assert(checked > 100000 && tightened > 1000);
    auto bad = glm::mat4(1); bad[0][0] = NAN;
    assert(pt::LightScissor(bad, extent).extent.width == extent.width);
    for (unsigned trial = 0; trial < 1000; ++trial) {
        std::vector<std::pair<float, float>> intervals(80);
        for (auto& interval : intervals) { interval.first = random(-10, 50); interval.second = interval.first + random(0, 10); }
        if (trial % 2) intervals[17] = {NAN, NAN};
        std::vector<unsigned> order(intervals.size()); std::iota(order.begin(), order.end(), 0);
        pt::OrderOpaque(std::span(order), [&](unsigned i) { return intervals[i]; });
        auto sorted = order; std::sort(sorted.begin(), sorted.end());
        for (unsigned i = 0; i < sorted.size(); ++i) assert(sorted[i] == i);
        std::vector<unsigned> positions(order.size());
        for (unsigned i = 0; i < order.size(); ++i) positions[order[i]] = i;
        for (unsigned a = 0; a < order.size(); ++a) for (unsigned b = a + 1; b < order.size(); ++b) {
            const auto x = intervals[a], y = intervals[b];
            if (!std::isfinite(x.first) || !std::isfinite(y.first) || !(x.second < y.first || y.second < x.first))
                assert(positions[a] < positions[b]);
        }
    }
    std::promise<void> entered, release;
    auto released = release.get_future().share();
    std::atomic<unsigned> destroyed{0};
    struct Packet {
        std::promise<void>* entered; std::shared_future<void> release; std::atomic<unsigned>* destroyed;
        ~Packet() { if (entered) { entered->set_value(); release.wait(); } ++*destroyed; }
    };
    {
        pt::BoundedRetirement<Packet> retirement;
        auto ready = entered.get_future();
        assert(retirement.Retire(std::unique_ptr<Packet>(new Packet{&entered, released, &destroyed})));
        assert(ready.wait_for(std::chrono::seconds(3)) == std::future_status::ready);
        assert(!retirement.Retire(std::unique_ptr<Packet>(new Packet{nullptr, {}, &destroyed})));
        assert(destroyed == 1); release.set_value();
    }
    assert(destroyed == 2);
    struct GpuStats {
        bool gpu_timing_valid = true;
        uint64_t gpu_sample = 1;
        float gpu_ms = 15, ray_setup_ms = 0;
        float pass_ms[6]{}, part_ms[6]{};
        float composition_ms[5]{1, 2, 3, 4, 5};
    } gpu;
    pt::RenderTimingWindow window;
    window.AddGpu(gpu); window.AddGpu(gpu);
    assert(window.gpu_samples == 1 && window.GpuMean(window.composition[4]) == 5);
    window.Clear(); window.AddGpu(gpu); assert(window.gpu_samples == 0);
    ++gpu.gpu_sample; window.AddGpu(gpu);
    assert(window.gpu_samples == 1 && window.GpuMean(window.composition[0]) == 1);
    std::cout << "Test 17: " << checked << " visible volume samples preserved, " << tightened
              << " clipped bounds tightened; 1000 opaque-order cases and bounded retirement passed\n";
}
