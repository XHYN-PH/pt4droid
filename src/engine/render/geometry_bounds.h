#pragma once
#include <glm/glm.hpp>
#include <cmath>
#include <cstdint>
#include <span>

namespace pt {
struct GeometryBounds {
    glm::vec3 lo{0.0f};
    glm::vec3 hi{0.0f};
    bool valid = false;
};
inline bool FinitePosition(const glm::vec3& p) {
    return std::isfinite(p.x) && std::isfinite(p.y) && std::isfinite(p.z);
}
template <typename VertexType>
GeometryBounds IndexedBounds(std::span<const VertexType> vertices, std::span<const uint32_t> indices,
                             uint32_t first, uint32_t count, int32_t offset) {
    GeometryBounds out;
    if (!count || first > indices.size() || count > indices.size() - first) return out;
    for (size_t i = first; i < size_t(first) + count; ++i) {
        const int64_t index = int64_t(indices[i]) + offset;
        if (index < 0 || uint64_t(index) >= vertices.size()) return {};
        const glm::vec3 p = vertices[size_t(index)].position;
        if (!FinitePosition(p)) return {};
        if (!out.valid) { out.lo = out.hi = p; out.valid = true; }
        else { out.lo = glm::min(out.lo, p); out.hi = glm::max(out.hi, p); }
    }
    return out;
}
// Exact affine enclosure, including shear, reflection and nonuniform scale.
// Padding keeps geometry touching a culling plane visible.
inline GeometryBounds TransformBounds(const GeometryBounds& local, const glm::mat4& transform) {
    if (!local.valid || transform[0].w != 0 || transform[1].w != 0 || transform[2].w != 0 || transform[3].w != 1) return {};
    const glm::vec3 centre = glm::vec3(transform * glm::vec4((local.lo + local.hi) * 0.5f, 1.0f));
    const glm::vec3 half = (local.hi - local.lo) * 0.5f;
    const glm::vec3 reach = glm::abs(glm::vec3(transform[0])) * half.x + glm::abs(glm::vec3(transform[1])) * half.y +
                            glm::abs(glm::vec3(transform[2])) * half.z;
    if (!FinitePosition(centre) || !FinitePosition(reach)) return {};
    const glm::vec3 margin = glm::vec3(1.0e-3f) + (glm::abs(centre) + reach) * 1.0e-6f;
    return {centre - reach - margin, centre + reach + margin, true};
}
inline bool BoundsInPlanes(const GeometryBounds& bounds, const glm::vec4* planes, size_t count = 6) {
    if (!bounds.valid) return true;
    for (size_t i = 0; i < count; ++i) {
        const glm::vec3 n(planes[i]);
        const glm::vec3 far(n.x >= 0 ? bounds.hi.x : bounds.lo.x, n.y >= 0 ? bounds.hi.y : bounds.lo.y,
                            n.z >= 0 ? bounds.hi.z : bounds.lo.z);
        if (glm::dot(n, far) + planes[i].w < 0.0f) return false;
    }
    return true;
}
}  // namespace pt
