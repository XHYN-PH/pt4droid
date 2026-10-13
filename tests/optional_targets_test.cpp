#include <vulkan/vulkan.h>
#include <algorithm>
#include <cassert>
#include <functional>
#include <cstdio>
#include <vector>
namespace pt {
constexpr VkFormat kAoFormat=VK_FORMAT_R8G8_UNORM,kPostFormat=VK_FORMAT_R16G16B16A16_SFLOAT;
struct Image{VkImage image{};VkExtent2D extent{};};
struct RenderTarget{Image image;VkImageLayout layout=VK_IMAGE_LAYOUT_UNDEFINED;bool Valid()const{return image.image!=VK_NULL_HANDLE;}VkExtent2D Extent()const{return image.extent;}};
struct TargetUse{RenderTarget* target;VkImageLayout layout;};struct ColorOutput{RenderTarget* target;bool clear;VkClearColorValue value;};
static int waits=0,created=0,clears=0,writes=0,fail_at=0;static bool submit_ok=true;
static VkResult vkDeviceWaitIdle(VkDevice){++waits;return VK_SUCCESS;}static void vkCmdEndRendering(VkCommandBuffer){}
static void UseTargets(VkCommandBuffer,std::initializer_list<TargetUse> uses){for(auto use:uses){assert(use.target->Valid());use.target->layout=use.layout;}}
static void BeginPass(VkCommandBuffer,VkExtent2D size,std::initializer_list<ColorOutput> colors){assert(size.width&&size.height);for(auto c:colors){assert(c.target->Valid()&&c.clear);++clears;}}
struct Context{void DestroyImage(Image& image){image={};}bool Submit(const std::function<void(VkCommandBuffer)>& fn){if(!submit_ok)return false;fn(nullptr);return true;}};
struct Renderer{pt::Context context;pt::Context& Context(){return context;}};
struct SceneRenderer{
 VkDevice device_{};Renderer renderer;Renderer* renderer_=&renderer;struct{bool occlusion=false,depth_of_field=false,motion_blur=false;}toggles;
 RenderTarget ao_[2],dof_half_,dof_quarter_[2],dof_eighth_[2],velocity_,object_velocity_,mb_blur_[2],mb_tile_[5],mb_neighbour_,mb_bake_;
 bool CreateTarget(RenderTarget& t,VkFormat,VkExtent2D size,VkImageUsageFlags,VkImageAspectFlags){assert(size.width&&size.height);++created;if(fail_at&&created==fail_at)return false;t.image.image=reinterpret_cast<VkImage>(uintptr_t(created));t.image.extent=size;t.layout=VK_IMAGE_LAYOUT_UNDEFINED;return true;}
 void WriteImageDescriptors(){++writes;}bool EnsureOptionalTargets(VkExtent2D,VkExtent2D,bool,bool=true);
 std::vector<RenderTarget*> All(){return {&ao_[0],&ao_[1],&dof_half_,&dof_quarter_[0],&dof_quarter_[1],&dof_eighth_[0],&dof_eighth_[1],&velocity_,&object_velocity_,&mb_blur_[0],&mb_blur_[1],&mb_tile_[0],&mb_tile_[1],&mb_tile_[2],&mb_tile_[3],&mb_tile_[4],&mb_neighbour_,&mb_bake_};}
};
}
int main(){using namespace pt;const VkExtent2D scene{1200,540},output{2400,1080};SceneRenderer r;
 for(int i=0;i<4000;++i)assert(r.EnsureOptionalTargets(scene,output,false));assert(waits==0&&created==0&&writes==0);
 assert(r.EnsureOptionalTargets(scene,output,true));assert(created==1&&r.object_velocity_.Valid()&&waits==1);r.toggles={true,true,true};assert(r.EnsureOptionalTargets(scene,output,true));assert(created==18&&waits==2&&writes==2&&clears==18);
 for(auto* t:r.All())assert(t->Valid()&&t->layout==VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);const int count=created;r.toggles={false,false,false};for(int i=0;i<4000;++i)assert(r.EnsureOptionalTargets(scene,output,false));assert(created==count&&waits==2&&writes==2);r.toggles={true,true,true};assert(r.EnsureOptionalTargets(scene,output,false));assert(created==count);
 for(int f=1;f<=18;++f){SceneRenderer test;created=0;fail_at=f;test.toggles={true,true,true};assert(!test.EnsureOptionalTargets(scene,output,false));for(auto* t:test.All())assert(!t->Valid());fail_at=0;assert(test.EnsureOptionalTargets(scene,output,false));for(auto* t:test.All())assert(t->Valid());}
 SceneRenderer failed;created=0;failed.toggles={true,true,true};submit_ok=false;int previous=writes;assert(!failed.EnsureOptionalTargets(scene,output,false));assert(writes==previous);for(auto* t:failed.All())assert(!t->Valid());submit_ok=true;assert(failed.EnsureOptionalTargets({1,1},{1,1},false));
 puts("Optional target production checks passed: 8000 steady frames, later enabling, retained targets, 18 allocation failures, queue failure and 1px extents");
}
