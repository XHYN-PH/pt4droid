#pragma once
#include <glm/glm.hpp>
#include <volk.h>
#include <algorithm>
#include <array>
#include <cmath>
namespace pt {
inline bool SpotBounds(glm::vec3 position,glm::vec3 axis,float cosine,float radius,glm::vec3& lo,glm::vec3& hi){
 const float norm=glm::length(axis);
 if(!std::isfinite(norm+cosine+radius)||norm<1e-5f||cosine<0||cosine>=1||radius<=0)return false;
 axis/=norm;cosine=std::max(0.0f,cosine-1e-5f);const float sine=std::sqrt(std::max(0.0f,1.0f-cosine*cosine));
 for(int i=0;i<3;++i){const float perpendicular=std::sqrt(std::max(0.0f,1.0f-axis[i]*axis[i]));const float maximum=axis[i]>=cosine?1.0f:axis[i]*cosine+perpendicular*sine;const float minimum=axis[i]<=-cosine?-1.0f:axis[i]*cosine-perpendicular*sine;lo[i]=position[i]+radius*std::min(0.0f,minimum)-0.01f;hi[i]=position[i]+radius*std::max(0.0f,maximum)+0.01f;}return true;
}
inline VkRect2D LightScissor(const glm::mat4& clip, VkExtent2D area) {
    std::array<glm::vec4, 8> corners;
    bool clipped = false;
    for (uint32_t i = 0; i < 8; ++i) {
        const auto p = clip * glm::vec4((i & 1) ? 1.0f : -1.0f, (i & 2) ? 1.0f : -1.0f,
                                      (i & 4) ? 1.0f : -1.0f, 1.0f);
        for (int k = 0; k < 4; ++k) if (!std::isfinite(p[k])) return {{0, 0}, area};
        corners[i] = p;
        clipped |= p.w <= 1e-5f || p.z < 0 || p.z > p.w;
    }
    glm::vec2 lo(1.0f), hi(-1.0f);
    bool found = false;
    auto include = [&](const glm::vec4& p) {
        const glm::vec2 q = glm::vec2(p) / p.w;
        lo = glm::min(lo, q); hi = glm::max(hi, q); found = true;
    };
    if (!clipped) {
        for (const auto& p : corners) include(p);
    } else {
        // Clip every face, rather than only the original edges: an oblique
        // mirror near plane can create vertices on edges of earlier cuts.
        // This retains conservative bounds when the camera enters a probe.
        constexpr uint8_t faces[6][4] = {{0, 1, 3, 2}, {4, 6, 7, 5}, {0, 4, 5, 1},
                                        {2, 3, 7, 6}, {0, 2, 6, 4}, {1, 5, 7, 3}};
        auto distance = [](const glm::vec4& p, unsigned plane) {
            return plane == 0 ? p.w - 1e-5f : plane == 1 ? p.z : p.w - p.z;
        };
        for (const auto& face : faces) {
            std::array<glm::vec4, 16> polygon{}, next{};
            size_t count = 4;
            for (size_t i = 0; i < count; ++i) polygon[i] = corners[face[i]];
            for (unsigned plane = 0; plane < 3 && count; ++plane) {
                size_t out = 0;
                glm::vec4 previous = polygon[count - 1];
                float before = distance(previous, plane);
                for (size_t i = 0; i < count; ++i) {
                    const glm::vec4 current = polygon[i];
                    const float after = distance(current, plane);
                    if ((before >= 0) != (after >= 0)) {
                        if (out >= next.size()) return {{0, 0}, area};
                        next[out++] = previous + (current - previous) * (before / (before - after));
                    }
                    if (after >= 0) {
                        if (out >= next.size()) return {{0, 0}, area};
                        next[out++] = current;
                    }
                    previous = current; before = after;
                }
                polygon = next; count = out;
            }
            for (size_t i = 0; i < count; ++i) {
                // Roundoff at a camera-plane cut must never cull visible work.
                if (polygon[i].w <= 0) return {{0, 0}, area};
                include(polygon[i]);
            }
        }
    }
    if (!found) return {};
    const glm::vec2 size(area.width, area.height);
    lo = glm::clamp(glm::floor((lo * .5f + .5f) * size) - 2.0f, glm::vec2(0), size);
    hi = glm::clamp(glm::ceil((hi * .5f + .5f) * size) + 2.0f, glm::vec2(0), size);
    return {{int32_t(lo.x), int32_t(lo.y)}, {uint32_t(std::max(0.0f, hi.x - lo.x)), uint32_t(std::max(0.0f, hi.y - lo.y))}};
}
}
