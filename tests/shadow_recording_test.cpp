// The test driver appends the production RecordShadows and UseTargets bodies.
#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <string_view>
#include <vector>
#include <span>
#include <vulkan/vulkan.h>
#include "engine/render/geometry_bounds.h"
#include "engine/render/shadow_cache.h"
#include "engine/render/render_sync.h"

namespace pt {
namespace gpu { constexpr uint32_t kInvalid=UINT32_MAX,kMatAlphaTest=1; }
constexpr uint32_t kShadowTiles=4;
constexpr uint64_t kMirrorLightId=123;
struct Image { VkImage image=reinterpret_cast<VkImage>(1); };
struct RenderTarget { Image image; VkImageLayout layout=VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL; VkImageAspectFlags aspect=VK_IMAGE_ASPECT_DEPTH_BIT; bool Valid()const{return image.image!=VK_NULL_HANDLE;} VkExtent2D Extent()const{return {4,4};} };
struct TargetUse { RenderTarget* target; VkImageLayout layout; };
struct ColorOutput {};
static std::vector<VkImageMemoryBarrier2> barriers;
static std::array<int,16> pixels{},static_pixels{};
static std::array<int,16>* drawing=&pixels;
static int full_clears=0,partial_clears=0,draw_calls=0,current_tile=0;
static void vkCmdPipelineBarrier2(VkCommandBuffer,const VkDependencyInfo* info){barriers.insert(barriers.end(),info->pImageMemoryBarriers,info->pImageMemoryBarriers+info->imageMemoryBarrierCount);}
static void vkCmdClearAttachments(VkCommandBuffer,uint32_t n,const VkClearAttachment* clear,uint32_t count,const VkClearRect* r){assert(n==1&&count==1&&clear->aspectMask==VK_IMAGE_ASPECT_DEPTH_BIT&&clear->clearValue.depthStencil.depth==0);++partial_clears;pixels[r->rect.offset.x+4*r->rect.offset.y]=0;}
static void vkCmdBindPipeline(VkCommandBuffer,VkPipelineBindPoint,VkPipeline){}
static void vkCmdSetDepthBias(VkCommandBuffer,float,float,float){}
static void vkCmdEndRendering(VkCommandBuffer){}
static void UseTargets(VkCommandBuffer,std::initializer_list<TargetUse>);
static void BeginPass(VkCommandBuffer,VkExtent2D area,std::initializer_list<ColorOutput>,RenderTarget* target,bool,bool clear){drawing=target->image.image==reinterpret_cast<VkImage>(2)?&static_pixels:&pixels;if(clear){if(drawing==&pixels)++full_clears;for(uint32_t y=0;y<area.height;++y)for(uint32_t x=0;x<area.width;++x)(*drawing)[x+4*y]=0;}}
static void BeginPass(VkCommandBuffer,VkRect2D area,std::span<const ColorOutput>,RenderTarget* target,bool,bool clear){drawing=target->image.image==reinterpret_cast<VkImage>(2)?&static_pixels:&pixels;current_tile=area.offset.x+4*area.offset.y;if(clear)(*drawing)[current_tile]=0;}
static void vkCmdCopyImage(VkCommandBuffer,VkImage source,VkImageLayout,VkImage destination,VkImageLayout,uint32_t n,const VkImageCopy* copy){assert(source==reinterpret_cast<VkImage>(2)&&destination==reinterpret_cast<VkImage>(1)&&n==1);int tile=copy->dstOffset.x+4*copy->dstOffset.y;pixels[tile]=static_pixels[tile];}
static void SetViewport(VkCommandBuffer,VkRect2D r){current_tile=r.offset.x+4*r.offset.y;}
template<class... Args> void LogInfo(const char*,Args...){}
struct SubMesh { bool shadow=true,skinned=false,shadow_double_sided=false; uint32_t index_count=3; };
struct GpuMesh { std::string name; std::vector<SubMesh> submeshes{SubMesh{}}; uint32_t skin_required=0; };
static bool MirrorLightSkips(const GpuMesh&,int){return false;}
static bool SphereInPlanes(const glm::vec3& p,float radius,const glm::vec4* planes){for(int i=0;i<6;i++)if(glm::dot(glm::vec3(planes[i]),p)+planes[i].w < -radius)return false;return true;}
struct SceneRenderer {
    struct Draw {const GpuMesh* mesh=nullptr;const SubMesh* sub=nullptr;glm::vec3 center{0};float radius=.5f;GeometryBounds shadow_bounds;glm::vec3 shadow_center{0};float shadow_radius=0;uint8_t hidden_views=0;uint32_t material=0,skin_base=gpu::kInvalid,skin_count=0;glm::mat4 transform{1};};
    struct ShadowView {uint32_t view=0,light=0;uint8_t casters=1;VkRect2D rect{{0,0},{1,1}};glm::vec3 center{0};float radius=100;bool frustum=false;glm::vec4 planes[6]{};float bias_constant=0,bias_slope=0;VkCullModeFlags cull=VK_CULL_MODE_FRONT_BIT;};
    struct View {glm::mat4 view_projection{1},view{1};glm::vec4 projection_param{0},eye{0},shadow{0};};
    struct GpuLight { glm::ivec4 info{0}; glm::vec4 shadow_rect{0}; };
    struct Frame {std::array<View,32> views;std::array<GpuLight,32> lights;} data; Frame* frame_=&data;
    struct Stats {std::array<uint32_t,4> shadow_misses{};uint32_t shadow_tiles_reused=0,shadow_tiles_rendered=0,shadow_tiles_shared=0,shadow_cache_redraws=0,shadow_draws=0,shadow_static_updates=0,shadow_static_copies=0,shadow_static_saved_draws=0;} stats_;
    struct Textures {uint32_t flags=0;uint32_t MaterialFlags(uint32_t)const{return flags;} uint64_t alpha=0;std::array<uint64_t,3> ShadowAlphaState(uint32_t)const{return {flags,0,alpha};}} texture; Textures* textures_=&texture;
    struct Light {uint64_t id=0;std::string name;} light; std::vector<const Light*> light_sources_{&light};
    struct MeshState {void Reset(){}} mesh_draw_state_;
    bool rt_active_=false,rt_reflections_=false,rt_ao_active_=false,rt_contact_active_=false;
    RenderTarget shadow_; VkPipeline shadow_pipeline_=VK_NULL_HANDLE;
    std::vector<ShadowView> shadow_views_;std::vector<Draw> draws_;
    std::array<ShadowTileCache,16> shadow_cache_,shadow_static_cache_,shadow_static_seen_;
    RenderTarget shadow_static_;bool shadow_static_unavailable_=false;
    std::array<ShadowMapState,16> shadow_static_states_;
    std::array<std::vector<const Draw*>,16> shadow_static_casters_;
    std::vector<glm::mat4> skin_matrices_;
    static constexpr VkFormat kDepthTargetFormat=VK_FORMAT_D32_SFLOAT;
    SceneRenderer(){shadow_static_.image.image=nullptr;}
    bool CreateTarget(RenderTarget& t,VkFormat,VkExtent2D,VkImageUsageFlags,VkImageAspectFlags){t.image.image=reinterpret_cast<VkImage>(2);return true;}
    std::array<bool,16> RecordStaticShadows(VkCommandBuffer,const std::array<bool,16>&);
    std::vector<ShadowCasterState> shadow_draw_states_;
    std::vector<uint8_t> shadow_draw_state_ready_, shadow_draw_stable_;std::array<ShadowMapState,16> shadow_states_;std::array<std::vector<const Draw*>,16> shadow_casters_;std::array<bool,16> shadow_shared_{};std::array<uint64_t,16> shadow_indices_{};
    void InvalidateShadowCache();void PrepareShadowCasters();void RecordShadows(VkCommandBuffer);
    void TestShadows(VkCommandBuffer c){PrepareShadowCasters();RecordShadows(c);}
    void RecordRayTracing(VkCommandBuffer){} void Stamp(VkCommandBuffer,uint32_t index){assert(index==12);}
    void DrawMesh(VkCommandBuffer,const Draw&,uint32_t,uint32_t,bool,bool,const glm::vec4*,VkCullModeFlags,bool){++draw_calls;++(*drawing)[current_tile];}
};
} // namespace pt

int main(int argc,char**) {
    using namespace pt; bool disabled=argc>1;
    setenv("PT_SHADOW_ADAPTIVE","1",0);
    if(disabled) setenv("PT_SHADOW_CACHE","0",1);
    GpuMesh mesh; SceneRenderer r;
    SceneRenderer::Draw d;d.mesh=&mesh;d.sub=&mesh.submeshes[0];r.draws_.push_back(d);
    SceneRenderer::ShadowView a,b,c;a.view=0;b.view=1;b.rect.offset.x=1;c.view=2;c.rect.offset.y=2;
    r.shadow_views_={a,b,c};
    r.TestShadows(nullptr);assert(full_clears==1&&partial_clears==0&&draw_calls==3&&r.stats_.shadow_draws==3);
    assert(pixels[0]==1&&pixels[1]==1&&pixels[8]==1);
    draw_calls=0;r.TestShadows(nullptr);
    if(disabled){assert(draw_calls==3&&r.stats_.shadow_tiles_reused==0);puts("Shadow recording fallback passed");return 0;}
    assert(draw_calls==0&&r.stats_.shadow_tiles_reused==3&&r.stats_.shadow_tiles_rendered==0);
    // Change one projection: clear and render only its tile, keeping both other
    // tiles untouched. Point-light halves are separate rectangles as well.
    r.data.views[1].eye.w=1;r.TestShadows(nullptr);
    assert(draw_calls==1&&partial_clears==1&&r.stats_.shadow_tiles_reused==2&&r.stats_.shadow_draws==1);
    assert(pixels[0]==1&&pixels[1]==1&&pixels[8]==1);
    // A caster moves: all active maps invalidate and the inactive row's former
    // tile is cleared, so revisiting it may not reuse an obsolete cached image.
    r.shadow_views_={a,b};r.draws_[0].transform[3].x=2;r.TestShadows(nullptr);
    assert(full_clears==2&&r.stats_.shadow_tiles_rendered==2);
    r.shadow_views_={c};r.draws_[0].transform=glm::mat4(1);draw_calls=0;r.TestShadows(nullptr);assert(draw_calls==1);
    // Alpha changes and moving poses invalidate; an unchanged exact pose reuses.
    r.texture.flags=gpu::kMatAlphaTest;draw_calls=0;r.TestShadows(nullptr);r.TestShadows(nullptr);assert(draw_calls==1);
    ++r.texture.alpha;draw_calls=0;r.TestShadows(nullptr);assert(draw_calls==1);
    r.texture.flags=0;mesh.submeshes[0].skinned=true;r.draws_[0].skin_base=0;r.draws_[0].skin_count=1;r.skin_matrices_={glm::mat4(1)};
    draw_calls=0;r.TestShadows(nullptr);r.TestShadows(nullptr);assert(draw_calls==1);
    r.skin_matrices_[0][3].x=.01f;r.TestShadows(nullptr);assert(draw_calls==2);
    r.draws_[0].skin_count=2;draw_calls=0;r.TestShadows(nullptr);r.TestShadows(nullptr);assert(draw_calls==2);
    r.draws_[0].skin_base=gpu::kInvalid;r.draws_[0].skin_count=0;
    mesh.submeshes[0].skinned=false;r.TestShadows(nullptr);draw_calls=0;r.TestShadows(nullptr);assert(draw_calls==0);
    // Removing a caster clears its map to zero instead of preserving a ghost.
    r.draws_.clear();r.TestShadows(nullptr);assert(pixels[8]==0&&r.stats_.shadow_draws==0);
    // Reassignment and invalidation (atlas resize / mesh destruction / shutdown).
    r.InvalidateShadowCache();draw_calls=0;r.draws_.push_back(d);r.TestShadows(nullptr);assert(draw_calls==1);
    for(const auto& barrier:barriers){if(barrier.newLayout==VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL)assert(barrier.dstAccessMask&VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT);if(barrier.newLayout==VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL)assert(barrier.srcAccessMask&VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT);}
    // Seven expensive maps change while one cheap map can be retained. Exercise
    // the actual full-clear transition and its opt-out, including stored pixels.
    SceneRenderer adaptive; GpuMesh heavy; heavy.submeshes[0].index_count=30;
    auto cheap=d;cheap.hidden_views=2;adaptive.draws_.push_back(cheap);
    auto expensive=d;expensive.mesh=&heavy;expensive.sub=&heavy.submeshes[0];expensive.hidden_views=1;
    for(int i=0;i<20;++i)adaptive.draws_.push_back(expensive);
    for(int i=0;i<8;++i){SceneRenderer::ShadowView view;view.view=i;view.casters=i?2:1;view.rect.offset={i%4,i/4};adaptive.shadow_views_.push_back(view);}
    adaptive.TestShadows(nullptr);
    const int clears_before=full_clears,partial_before=partial_clears;
    for(int i=1;i<8;++i)adaptive.data.views[i].eye.z=1;
    draw_calls=0;adaptive.TestShadows(nullptr);
    const bool full=std::string_view(std::getenv("PT_SHADOW_ADAPTIVE"))!="0";
    assert(draw_calls==(full?141:140)&&full_clears==clears_before+(full?1:0));
    assert(partial_clears==partial_before+(full?0:7));
    assert(adaptive.stats_.shadow_cache_redraws==(full?1u:0u)&&adaptive.stats_.shadow_tiles_reused==(full?0u:1u));
    assert(pixels[0]==1);for(int i=1;i<8;++i)assert(pixels[i]==20);
    draw_calls=0;adaptive.TestShadows(nullptr);assert(draw_calls==0&&adaptive.stats_.shadow_tiles_reused==8);
    // Production static template, copies and moving overlays keep identical
    // depth contributions. Projection/alpha/removal changes force rebuilds.
    SceneRenderer templ;GpuMesh animated;animated.submeshes[0].skinned=true;
    a.view=0;a.rect.offset={0,0};templ.shadow_views_={a};
    for(int i=0;i<32;++i)templ.draws_.push_back(d);
    auto moving=d;moving.mesh=&animated;moving.sub=&animated.submeshes[0];moving.skin_base=0;moving.skin_count=1;
    templ.draws_.push_back(moving);templ.skin_matrices_={glm::mat4(1)};
    draw_calls=0;templ.TestShadows(nullptr);assert(draw_calls==33&&pixels[0]==33);
    templ.skin_matrices_[0][3].x=1;draw_calls=0;templ.TestShadows(nullptr);
    assert(pixels[0]==33&&draw_calls==33&&templ.stats_.shadow_static_updates==1&&templ.stats_.shadow_static_copies==1);
    templ.skin_matrices_[0][3].x=2;draw_calls=0;templ.TestShadows(nullptr);
    assert(pixels[0]==33&&draw_calls==1&&templ.stats_.shadow_static_updates==0&&templ.stats_.shadow_static_saved_draws==32);
    templ.data.views[0].eye.x=1;templ.skin_matrices_[0][3].x=3;draw_calls=0;templ.TestShadows(nullptr);assert(draw_calls==33&&templ.stats_.shadow_static_copies==0&&pixels[0]==33);
    templ.skin_matrices_[0][3].x=4;draw_calls=0;templ.TestShadows(nullptr);assert(draw_calls==33&&templ.stats_.shadow_static_updates==1&&pixels[0]==33);
    templ.draws_.erase(templ.draws_.begin());templ.skin_matrices_[0][3].x=5;draw_calls=0;templ.TestShadows(nullptr);assert(draw_calls==32&&pixels[0]==32&&templ.stats_.shadow_static_copies==0);
    // A frozen skeletal pose may be part of the template. It must not be
    // overlaid a second time alongside the genuinely moving skeleton.
    SceneRenderer frozen;a.view=0;a.rect.offset={0,0};frozen.shadow_views_={a};
    for(int i=0;i<32;++i)frozen.draws_.push_back(d);
    auto stationary=moving;stationary.skin_base=0;auto active=moving;active.skin_base=1;
    frozen.draws_.push_back(stationary);frozen.draws_.push_back(active);frozen.skin_matrices_={glm::mat4(1),glm::mat4(1)};
    for(int frame=0;frame<4;++frame){
        frozen.skin_matrices_[1][3].x=float(frame);draw_calls=0;frozen.TestShadows(nullptr);
        assert(pixels[0]==34);
        if(frame==2)assert(draw_calls==34&&frozen.stats_.shadow_static_updates==1);
        if(frame==3)assert(draw_calls==1&&frozen.stats_.shadow_static_saved_draws==33);
    }
    // Equivalent main and mirror captures share a spot map before frame upload.
    SceneRenderer shared; shared.light_sources_ = {&shared.light, &shared.light}; shared.draws_.push_back(d);
    a.light=0;a.view=0;a.casters=1;a.rect.offset={0,0};
    b=a;b.light=1;b.view=1;b.casters=2;b.rect.offset={1,0};
    shared.shadow_views_={a,b}; shared.data.lights[0].info.x=0; shared.data.lights[1].info.x=1;
    shared.data.lights[0].shadow_rect=glm::vec4(0,0,.25f,.25f);shared.data.lights[1].shadow_rect=glm::vec4(.25f,0,.25f,.25f);
    draw_calls=0;shared.TestShadows(nullptr);
    if(const char* sharing=std::getenv("PT_SHADOW_SHARE");sharing&&std::string_view(sharing)=="0"){
        assert(draw_calls==2&&shared.stats_.shadow_tiles_shared==0&&shared.data.lights[1].info.x==1);
        assert(shared.data.lights[1].shadow_rect==glm::vec4(.25f,0,.25f,.25f));
        puts("Shadow sharing opt-out and adaptive recording passed");return 0;
    }
    assert(draw_calls==1&&shared.stats_.shadow_tiles_shared==1);
    assert(shared.data.lights[1].info.x==0&&shared.data.lights[1].shadow_rect==shared.data.lights[0].shadow_rect);
    // A view-specific caster must unshare and redraw its formerly aliased tile.
    shared.draws_[0].hidden_views=1;shared.data.lights[1].info.x=1;
    draw_calls=0;shared.TestShadows(nullptr);assert(shared.stats_.shadow_tiles_shared==0&&shared.stats_.shadow_tiles_rendered==2);
    assert(pixels[0]==0&&pixels[1]==1&&shared.data.lights[1].info.x==1);
    // Both hemispheres of a point light must match, including its exact pose.
    mesh.submeshes[0].skinned=true;shared.draws_[0].hidden_views=0;
    shared.draws_[0].skin_base=0;shared.draws_[0].skin_count=1;shared.skin_matrices_={glm::mat4(1)};
    a.rect.offset={0,0};b=a;b.view=1;b.rect.offset={1,0};shared.data.views[1].eye.w=1;
    c=a;c.light=1;c.view=2;c.casters=2;c.rect.offset={2,0};
    auto e=b;e.light=1;e.view=3;e.casters=2;e.rect.offset={3,0};shared.data.views[3].eye.w=1;
    shared.shadow_views_={a,b,c,e};draw_calls=0;shared.TestShadows(nullptr);assert(draw_calls==2&&shared.stats_.shadow_tiles_shared==2);
    draw_calls=0;shared.TestShadows(nullptr);assert(draw_calls==0&&shared.stats_.shadow_tiles_reused==2);
    shared.skin_matrices_[0][3].x=1;draw_calls=0;shared.TestShadows(nullptr);assert(draw_calls==2&&shared.stats_.shadow_tiles_reused==0);
    shared.skin_matrices_[0][3].x=2;shared.data.views[3].eye.w=2;draw_calls=0;shared.TestShadows(nullptr);assert(draw_calls==4&&shared.stats_.shadow_tiles_shared==0);
    mesh.submeshes[0].skinned=false;
    assert(PreferFullShadowClear(12,11,1290,1270));
    assert(!PreferFullShadowClear(12,11,1290,1270,1000000,600000));
    assert(!PreferFullShadowClear(12,0,1290,0));assert(!PreferFullShadowClear(12,11,1290,900));
    assert(!PreferFullShadowClear(4,1,100,75));
    puts("Production shadow recording passed: clear/reuse transitions, exact spot and whole-point sharing, view masks, dynamic skin and adaptive policy");
}
