#pragma once
#include <glm/glm.hpp>
#include <array>
#include <cstdint>
#include <vector>
#include <memory>
#include <algorithm>
#include <bit>
#include <tuple>

namespace pt {
struct ShadowCasterState {
    uintptr_t mesh = 0;
    uint32_t submesh = 0;
    uint32_t material = 0;
    uint32_t flags = 0;
    glm::mat4 transform{1.0f};
    std::array<uint64_t,3> alpha{};
    std::shared_ptr<const std::vector<glm::mat4>> pose;
    bool operator==(const ShadowCasterState& b) const {
        return mesh==b.mesh && submesh==b.submesh && material==b.material && flags==b.flags && transform==b.transform && alpha==b.alpha &&
            (pose==b.pose || (pose && b.pose && *pose==*b.pose));
    }
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
    // Exact alpha and immutable pose snapshots permit reuse only while unchanged.
    bool reusable = true;
    bool operator==(const ShadowMapState&) const = default;
    void CanonicalizeCasters() {
        // Depth writes are order independent. Preserve every dependency while
        // avoiding misses caused only by the scene's changing emission order.
        auto matrix_less=[](const glm::mat4& a,const glm::mat4& b) {
            for(int c=0;c<4;++c)for(int r=0;r<4;++r) {
                auto x=std::bit_cast<uint32_t>(a[c][r]),y=std::bit_cast<uint32_t>(b[c][r]);
                if(x!=y)return x<y;
            }
            return false;
        };
        std::sort(casters.begin(),casters.end(),[&](const auto& a,const auto& b) {
            auto x=std::tie(a.mesh,a.submesh,a.material,a.flags,a.alpha),y=std::tie(b.mesh,b.submesh,b.material,b.flags,b.alpha);
            if(x!=y)return x<y;
            if(matrix_less(a.transform,b.transform))return true;
            if(matrix_less(b.transform,a.transform))return false;
            if(bool(a.pose)!=bool(b.pose))return !a.pose;
            if(!a.pose)return false;
            return std::lexicographical_compare(a.pose->begin(),a.pose->end(),b.pose->begin(),b.pose->end(),matrix_less);
        });
    }
    void CopyProjection(const ShadowMapState& source) {
        view_projection=source.view_projection;view=source.view;projection_param=source.projection_param;
        eye=source.eye;shadow=source.shadow;bias_constant=source.bias_constant;bias_slope=source.bias_slope;
        cull=source.cull;caster_views=source.caster_views;reusable=source.reusable;casters.clear();
    }
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
enum class ShadowMiss : uint8_t { None, Cold, Dynamic, Projection, Casters };
class ShadowTileCache {
public:
    ShadowMiss Miss(const ShadowMapState& state) const {
        if (!state.reusable) return ShadowMiss::Dynamic;
        if (!valid_) return ShadowMiss::Cold;
        if (!SameShadowProjection(state, previous_) || state.caster_views != previous_.caster_views) return ShadowMiss::Projection;
        if (state.casters != previous_.casters) return ShadowMiss::Casters;
        return ShadowMiss::None;
    }
    bool Reuse(const ShadowMapState& state) const { return Miss(state) == ShadowMiss::None; }
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
