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
