#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <new>
#include <random>
#include <vector>
#include <glm/gtc/matrix_transform.hpp>
#include "engine/core/stable_top.h"
#include "engine/render/geometry_bounds.h"
#include "engine/render/light_cull.h"
#include "engine/render/render_sync.h"
#include "engine/render/render_timing.h"
#include "engine/render/shadow_cache.h"
#include "engine/render/scene_lighting.h"
#include "engine/vfx/vfx_system.h"
#include "reference/mobile_light_block_test6.h"

namespace pt::game { pt::vfx::LightBlock LightBlockAt(const SceneLighting&, const glm::vec3&, const glm::vec3&); }
static bool count_allocations = false;
static size_t allocations = 0;
void* operator new(size_t n) { if (count_allocations) ++allocations; if (void* p = std::malloc(n ? n : 1)) return p; throw std::bad_alloc(); }
void operator delete(void* p) noexcept { std::free(p); }
void operator delete(void* p, size_t) noexcept { std::free(p); }
static void CheckBlock(const pt::vfx::LightBlock& a, const pt::vfx::LightBlock& b) {
    assert(a.count == b.count && a.sky == b.sky && a.directional == b.directional);
    for (size_t i = 0; i < 3; ++i) assert(a.position[i] == b.position[i] && a.color[i] == b.color[i] && a.inv_r4[i] == b.inv_r4[i]);
}

int main() {
    using namespace pt;
    std::mt19937 rng(0x505437);
    std::uniform_real_distribution<float> scalar(-5, 5);
    auto vec = [&] { return glm::vec3(scalar(rng), scalar(rng), scalar(rng)); };
    // Full production light blocks, compared exactly with the independent test.6
    // oracle. Deliberate equal priorities/scores exercise stable tie handling.
    for (int test = 0; test < 12000; ++test) {
        SceneLighting scene;
        const glm::vec3 centre = test % 4 ? vec() : glm::vec3(0);
        const glm::vec3 half = glm::abs(vec());
        for (uint32_t i = 0, n = rng() % 65; i < n; ++i) {
            SceneLight l; l.type = i % 2 ? LightType::Point : LightType::Spot;
            l.position = i % 3 ? vec() : glm::vec3(0);
            l.direction = glm::normalize(vec() + glm::vec3(.01f));
            l.intensity = i % 3 ? glm::abs(vec()) : glm::vec3(1);
            l.inner_range = .01f + std::abs(scalar(rng)); l.outer_range = i % 7 ? 10 + scalar(rng) : 0;
            l.dimmer = std::abs(scalar(rng)); l.cos_outer = -.7f; l.inv_cone_range = 1.0f;
            l.hidden_views = i % 9 == 0 ? 1 : 0; scene.lights.push_back(l);
        }
        for (uint32_t i = 0, n = rng() % 17; i < n; ++i) {
            SceneProbe p; p.priority = int(rng() % 3); p.weight = float(rng() % 10) / 10;
            p.world_to_box = glm::scale(glm::translate(glm::mat4(1), -vec()), glm::vec3(.1f));
            p.positive_scale = p.negative_scale = glm::vec3(1);
            for (auto& sh : p.sh) sh = vec(); scene.probes.push_back(p);
        }
        const auto expected = reference::LightBlockAt(scene, centre, half);
        count_allocations = true;
        const auto actual = game::LightBlockAt(scene, centre, half);
        count_allocations = false;
        CheckBlock(expected, actual);
    }
    assert(allocations == 0);
    // Indexed bounds: signed offsets, unused vertices and malformed input.
    struct V { glm::vec3 position; };
    std::vector<V> vertices{{{-1,-2,-3}}, {{2,3,4}}, {{100,100,100}}};
    std::vector<uint32_t> indices{0,1,1};
    auto b = IndexedBounds<V>(vertices, indices, 0, 3, 0);
    assert(b.valid && b.lo == vertices[0].position && b.hi == vertices[1].position);
    assert(!IndexedBounds<V>(vertices, indices, 0, 3, -1).valid);
    assert(!IndexedBounds<V>(vertices, indices, UINT32_MAX, 3, 0).valid);
    assert(!IndexedBounds<V>(vertices, indices, 0, UINT32_MAX, 0).valid);
    for (int test = 0; test < 8000; ++test) {
        glm::mat4 m(1); m[0] = glm::vec4(vec(),0); m[1] = glm::vec4(vec(),0); m[2] = glm::vec4(vec(),0); m[3] = glm::vec4(vec(),1);
        auto world = TransformBounds(b,m); assert(world.valid);
        for (int k = 0; k < 8; ++k) {
            auto local = glm::vec3(k&1 ? b.hi.x:b.lo.x, k&2 ? b.hi.y:b.lo.y, k&4 ? b.hi.z:b.lo.z);
            auto p = glm::vec3(m*glm::vec4(local,1));
            assert(glm::all(glm::greaterThanEqual(p, world.lo)) && glm::all(glm::lessThanEqual(p, world.hi)));
        }
        glm::vec4 plane(vec(),scalar(rng));
        if (!BoundsInPlanes(world,&plane,1)) {
            for (const auto& v : vertices) { if(v.position==vertices[2].position) continue; auto p=glm::vec3(m*glm::vec4(v.position,1)); assert(glm::dot(glm::vec3(plane),p)+plane.w<0); }
        }
    }
    glm::mat4 projective(1); projective[0].w=.01f; assert(!TransformBounds(b,projective).valid);
    GeometryBounds touching{{0,0,0},{1,1,1},true}; glm::vec4 plane(1,0,0,0); assert(BoundsInPlanes(touching,&plane,1));
    // Fully hidden mirrors skip; edge visibility, a wall-coincident mirror,
    // disabling an occluder (an open door), and an empty scene stay visible.
    glm::mat4 projection(0); float f=1/std::tan(glm::radians(60.0f)*.5f);
    projection[0][0]=f;projection[1][1]=-f;projection[2][3]=-1;projection[3][2]=.1f;
    glm::mat4 vp=projection*glm::lookAt(glm::vec3(0),glm::vec3(0,0,-1),glm::vec3(0,1,0));
    SceneOccluder wall;wall.count=4;wall.points[0]={-2,-2,-5};wall.points[1]={2,-2,-5};wall.points[2]={2,2,-5};wall.points[3]={-2,2,-5};
    auto volumes=lightcull::BuildOccluderVolumes(std::span(&wall,1),vp,glm::vec3(0),lightcull::OccluderLimits{});
    assert(lightcull::FindOccludingVolume({-.26f,-.26f,-10.02f},{.26f,.26f,-9.98f},volumes)>=0);
    assert(lightcull::FindOccludingVolume({3.9f,0,-10.02f},{4.2f,.2f,-9.98f},volumes)<0);
    assert(lightcull::FindOccludingVolume({-.26f,-.26f,-5.01f},{.26f,.26f,-4.99f},volumes)<0);
    wall.count=0;volumes=lightcull::BuildOccluderVolumes(std::span(&wall,1),vp,glm::vec3(0),lightcull::OccluderLimits{});
    assert(lightcull::FindOccludingVolume({-.26f,-.26f,-10.02f},{.26f,.26f,-9.98f},volumes)<0);
    // Exact cache matching covers light/projection motion, caster changes,
    // material/culling changes, volatile maps, reassignment and invalidation.
    ShadowTileCache cache; ShadowMapState state; state.casters.push_back({1,0,2,0,glm::mat4(1)});
    assert(!cache.Reuse(state)); cache.Recorded(state); assert(cache.Reuse(state));
    for (int test = 0; test < 3000; ++test) {
        auto changed = state;
        switch(test%10) {
            case 0: changed.view[3].x += .01f; break; case 1: changed.view_projection[0].x += .01f; break;
            case 2: changed.eye.w += 1; break; case 3: changed.shadow.x += .01f; break;
            case 4: changed.projection_param.w += 1; break; case 5: changed.casters[0].transform[3].x += .01f; break;
            case 6: changed.casters.clear(); break; case 7: changed.casters[0].material++; break;
            case 8: changed.cull++; break; case 9: changed.caster_views++; break;
        }
        assert(!cache.Reuse(changed));
    }
    auto volatile_map = state; volatile_map.reusable=false; cache.Recorded(volatile_map); assert(!cache.Reuse(state));
    cache.Recorded(state); cache.Invalidate(); assert(!cache.Reuse(state));
    std::array<ShadowTileCache,16> atlas;
    for(auto& tile:atlas) tile.Recorded(state);
    for(auto& tile:atlas) tile.Invalidate(); // a full atlas clear invalidates inactive tiles too
    for(auto& tile:atlas) assert(!tile.Reuse(state));
    // Synchronization: preserve RAW, WAR and WAW; no redundant read/read waits.
    for(auto write:{VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,VK_IMAGE_LAYOUT_GENERAL}) {
        assert(TargetBarrierRequired(write,write)); assert(TargetBarrierRequired(write,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));
        assert(TargetBarrierRequired(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,write));
    }
    assert(!TargetBarrierRequired(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));
    assert(!TargetBarrierRequired(VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL,VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL));
    assert(TargetAccessScope(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL).stages==VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT);
    assert(TargetAccessScope(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL).stages&VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT);
    assert(TargetAccessScope(VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL).access==VK_ACCESS_2_TRANSFER_READ_BIT);
    assert(TargetAccessScope(VK_IMAGE_LAYOUT_UNDEFINED).access==0);
    assert(TargetAccessScope(VK_IMAGE_LAYOUT_GENERAL).stages==VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT);
    // Averages accept each completed GPU sample once; stale/unavailable samples
    // and logging restarts do not bias the totals toward zero or repeat frames.
    struct Stats { bool gpu_timing_valid=true; uint64_t gpu_sample=1; float gpu_ms=21,ray_setup_ms=1,cpu_ms=2; float pass_ms[6]={2,3,4,5,3,3},part_ms[6]={}; int shadow_tiles_reused=2,shadow_tiles_rendered=1,shadow_tiles_shared=2,shadow_cache_redraws=1,shadow_draws=3,mirrors_occluded=0; } stats;
    RenderTimingWindow timing; timing.AddGpu(stats); timing.AddGpu(stats); assert(timing.gpu_samples==1);
    stats.gpu_timing_valid=false; stats.gpu_sample=2; timing.AddGpu(stats); assert(timing.gpu_samples==1);
    stats.gpu_timing_valid=true; timing.AddGpu(stats); assert(timing.gpu_samples==2 && timing.GpuMean(timing.gpu_ms)==21);
    double sum=timing.GpuMean(timing.ray_ms); for(auto p:timing.passes) sum+=timing.GpuMean(p); assert(sum==21);
    timing.AddCpu(stats); assert(timing.CpuMean(timing.cpu_ms)==2); timing.Clear(); timing.AddGpu(stats); assert(timing.gpu_samples==0);
    timing.Reset(); timing.AddGpu(stats); assert(timing.gpu_samples==1);
    for(auto blend:{vfx::BlendMode::Alpha,vfx::BlendMode::Add,vfx::BlendMode::Sub,vfx::BlendMode::Mul,vfx::BlendMode::Min,vfx::BlendMode::Opaque})
        for(int refract=0;refract<2;++refract) for(int opaque=0;opaque<2;++opaque) for(int legacy=0;legacy<2;++legacy) {
            vfx::Draw draw; draw.blend=blend; draw.refract=refract; draw.opaque=opaque;
            bool expected=(!refract || (!opaque&&!legacy)) && (blend==vfx::BlendMode::Alpha||blend==vfx::BlendMode::Add||blend==vfx::BlendMode::Sub);
            assert(vfx::BufferedEffect(draw,legacy)==expected);
        }
    std::puts("Renderer refinement checks passed: 12000 exact light blocks with zero allocations, 8000 affine bounds, 3000 cache mutations, synchronization and timing/particle classification");
}
