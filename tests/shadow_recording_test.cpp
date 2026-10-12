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
static std::array<int,16> pixels{};
static int full_clears=0,partial_clears=0,draw_calls=0,current_tile=0;
static void vkCmdPipelineBarrier2(VkCommandBuffer,const VkDependencyInfo* info){barriers.insert(barriers.end(),info->pImageMemoryBarriers,info->pImageMemoryBarriers+info->imageMemoryBarrierCount);}
static void vkCmdClearAttachments(VkCommandBuffer,uint32_t n,const VkClearAttachment* clear,uint32_t count,const VkClearRect* r){assert(n==1&&count==1&&clear->aspectMask==VK_IMAGE_ASPECT_DEPTH_BIT&&clear->clearValue.depthStencil.depth==0);++partial_clears;pixels[r->rect.offset.x+4*r->rect.offset.y]=0;}
static void vkCmdBindPipeline(VkCommandBuffer,VkPipelineBindPoint,VkPipeline){}
static void vkCmdSetDepthBias(VkCommandBuffer,float,float,float){}
static void vkCmdEndRendering(VkCommandBuffer){}
static void UseTargets(VkCommandBuffer,std::initializer_list<TargetUse>);
static void BeginPass(VkCommandBuffer,VkExtent2D area,std::initializer_list<ColorOutput>,RenderTarget*,bool,bool clear){if(clear){++full_clears;for(uint32_t y=0;y<area.height;++y)for(uint32_t x=0;x<area.width;++x)pixels[x+4*y]=0;}}
static void SetViewport(VkCommandBuffer,VkRect2D r){current_tile=r.offset.x+4*r.offset.y;}
template<class... Args> void LogInfo(const char*,Args...){}
struct SubMesh { bool shadow=true,skinned=false,shadow_double_sided=false; uint32_t index_count=3; };
struct GpuMesh { std::string name; std::vector<SubMesh> submeshes{SubMesh{}}; };
static bool MirrorLightSkips(const GpuMesh&,int){return false;}
static bool SphereInPlanes(const glm::vec3& p,float radius,const glm::vec4* planes){for(int i=0;i<6;i++)if(glm::dot(glm::vec3(planes[i]),p)+planes[i].w < -radius)return false;return true;}
struct SceneRenderer {
    struct Draw {const GpuMesh* mesh=nullptr;const SubMesh* sub=nullptr;glm::vec3 center{0};float radius=.5f;GeometryBounds shadow_bounds;glm::vec3 shadow_center{0};float shadow_radius=0;uint8_t hidden_views=0;uint32_t material=0,skin_base=gpu::kInvalid;glm::mat4 transform{1};};
    struct ShadowView {uint32_t view=0,light=0;uint8_t casters=1;VkRect2D rect{{0,0},{1,1}};glm::vec3 center{0};float radius=100;bool frustum=false;glm::vec4 planes[6]{};float bias_constant=0,bias_slope=0;VkCullModeFlags cull=VK_CULL_MODE_FRONT_BIT;};
    struct View {glm::mat4 view_projection{1},view{1};glm::vec4 projection_param{0},eye{0},shadow{0};};
    struct Frame {std::array<View,32> views;} data; Frame* frame_=&data;
    struct Stats {uint32_t shadow_tiles_reused=0,shadow_tiles_rendered=0,shadow_draws=0;} stats_;
    struct Textures {uint32_t flags=0;uint32_t MaterialFlags(uint32_t)const{return flags;}} texture; Textures* textures_=&texture;
    struct Light {uint64_t id=0;std::string name;} light; std::vector<const Light*> light_sources_{&light};
    struct MeshState {void Reset(){}} mesh_draw_state_;
    bool rt_active_=false,rt_reflections_=false,rt_ao_active_=false,rt_contact_active_=false;
    RenderTarget shadow_; VkPipeline shadow_pipeline_=VK_NULL_HANDLE;
    std::vector<ShadowView> shadow_views_;std::vector<Draw> draws_;
    std::array<ShadowTileCache,16> shadow_cache_;std::array<ShadowMapState,16> shadow_states_;std::array<std::vector<const Draw*>,16> shadow_casters_;
    void InvalidateShadowCache();void RecordShadows(VkCommandBuffer);
    void RecordRayTracing(VkCommandBuffer){} void Stamp(VkCommandBuffer,uint32_t index){assert(index==12);}
    void DrawMesh(VkCommandBuffer,const Draw&,uint32_t,uint32_t,bool,bool,const glm::vec4*,VkCullModeFlags,bool){++draw_calls;++pixels[current_tile];}
};
} // namespace pt

int main(int argc,char**) {
    using namespace pt; bool disabled=argc>1;
    if(disabled) setenv("PT_SHADOW_CACHE","0",1);
    GpuMesh mesh; SceneRenderer r;
    SceneRenderer::Draw d;d.mesh=&mesh;d.sub=&mesh.submeshes[0];r.draws_.push_back(d);
    SceneRenderer::ShadowView a,b,c;a.view=0;b.view=1;b.rect.offset.x=1;c.view=2;c.rect.offset.y=2;
    r.shadow_views_={a,b,c};
    r.RecordShadows(nullptr);assert(full_clears==1&&partial_clears==0&&draw_calls==3&&r.stats_.shadow_draws==3);
    assert(pixels[0]==1&&pixels[1]==1&&pixels[8]==1);
    draw_calls=0;r.RecordShadows(nullptr);
    if(disabled){assert(draw_calls==3&&r.stats_.shadow_tiles_reused==0);puts("Shadow recording fallback passed");return 0;}
    assert(draw_calls==0&&r.stats_.shadow_tiles_reused==3&&r.stats_.shadow_tiles_rendered==0);
    // Change one projection: clear and render only its tile, keeping both other
    // tiles untouched. Point-light halves are separate rectangles as well.
    r.data.views[1].eye.w=1;r.RecordShadows(nullptr);
    assert(draw_calls==1&&partial_clears==1&&r.stats_.shadow_tiles_reused==2&&r.stats_.shadow_draws==1);
    assert(pixels[0]==1&&pixels[1]==1&&pixels[8]==1);
    // A caster moves: all active maps invalidate and the inactive row's former
    // tile is cleared, so revisiting it may not reuse an obsolete cached image.
    r.shadow_views_={a,b};r.draws_[0].transform[3].x=2;r.RecordShadows(nullptr);
    assert(full_clears==2&&r.stats_.shadow_tiles_rendered==2);
    r.shadow_views_={c};r.draws_[0].transform=glm::mat4(1);draw_calls=0;r.RecordShadows(nullptr);assert(draw_calls==1);
    // Alpha streaming and skin never use a cached map.
    r.texture.flags=gpu::kMatAlphaTest;draw_calls=0;r.RecordShadows(nullptr);r.RecordShadows(nullptr);assert(draw_calls==2);
    r.texture.flags=0;mesh.submeshes[0].skinned=true;draw_calls=0;r.RecordShadows(nullptr);r.RecordShadows(nullptr);assert(draw_calls==2);
    mesh.submeshes[0].skinned=false;r.RecordShadows(nullptr);draw_calls=0;r.RecordShadows(nullptr);assert(draw_calls==0);
    // Removing a caster clears its map to zero instead of preserving a ghost.
    r.draws_.clear();r.RecordShadows(nullptr);assert(pixels[8]==0&&r.stats_.shadow_draws==0);
    // Reassignment and invalidation (atlas resize / mesh destruction / shutdown).
    r.InvalidateShadowCache();draw_calls=0;r.draws_.push_back(d);r.RecordShadows(nullptr);assert(draw_calls==1);
    for(const auto& barrier:barriers){if(barrier.newLayout==VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL)assert(barrier.dstAccessMask&VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT);if(barrier.newLayout==VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL)assert(barrier.srcAccessMask&VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT);}
    puts("Production shadow recording passed: complete/partial clears, exact reuse, light/caster movement, streaming alpha, skin, ghost removal and fallback");
}
