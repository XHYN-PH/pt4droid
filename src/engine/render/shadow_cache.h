#pragma once
#include <glm/glm.hpp>
#include <array>
#include <cstdint>
#include <vector>

namespace pt {
struct ShadowCasterState {
    uintptr_t mesh = 0;
    uint32_t submesh = 0;
    uint32_t material = 0;
    uint32_t flags = 0;
    glm::mat4 transform{1.0f};
    std::array<uint64_t,3> alpha{};
    bool operator==(const ShadowCasterState&) const = default;
};
struct ShadowMapState {
    glm::mat4 view_projection{1.0f};
    glm::mat4 view{1.0f};
    glm::vec4 projection_param{0.0f};
    glm::vec4 eye{0.0f};
    glm::vec4 shadow{0.0f};
    float bias_constant = 0.0f;
    float bias_slope = 0.0f;
    uint32_t cull = 0;
    uint8_t caster_views = 0;
    std::vector<ShadowCasterState> casters;
    // Exact alpha dependencies permit reuse; animated skin stays per frame.
    bool reusable = true;
    bool operator==(const ShadowMapState&) const = default;
};

// The caster masks may differ for camera and mirror views. Sharing is safe
// only after their accepted Draw pointers also compare equal this frame.
inline bool SameShadowProjection(const ShadowMapState& a, const ShadowMapState& b) {
    return a.view_projection == b.view_projection && a.view == b.view && a.projection_param == b.projection_param &&
           a.eye == b.eye && a.shadow == b.shadow && a.bias_constant == b.bias_constant &&
           a.bias_slope == b.bias_slope && a.cull == b.cull;
}

// A small cached tile need not force a depth LOAD of the entire render area.
// Keep reuse when it saves appreciable draw work or most active tiles.
inline bool PreferFullShadowClear(uint32_t active, uint32_t dirty, uint64_t total_draws, uint64_t dirty_draws,
                                  uint64_t total_indices = 0, uint64_t dirty_indices = 0) {
    if (!dirty || dirty >= active || dirty_draws > total_draws || dirty_indices > total_indices) return false;
    return uint64_t(dirty) * 4 >= uint64_t(active) * 3 &&
           (total_draws - dirty_draws) <= total_draws / 8 &&
           (total_indices - dirty_indices) <= total_indices / 8;
}
class ShadowTileCache {
public:
    bool Reuse(const ShadowMapState& state) const { return valid_ && state.reusable && state == previous_; }
    void Recorded(const ShadowMapState& state) {
        valid_ = state.reusable;
        if (valid_) previous_ = state;
    }
    void Invalidate() { valid_ = false; }
private:
    bool valid_ = false;
    ShadowMapState previous_;
};
}  // namespace pt
