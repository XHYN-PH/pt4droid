#include <vulkan/vulkan.h>
#include <array>
#include <algorithm>
#include <cassert>
#include <chrono>
#include <cstdio>
#include <functional>
#include <stdexcept>
#include <unordered_map>
namespace pt::vk {
struct Buffer{VkBuffer buffer{};VkDeviceSize size=0;};
static std::unordered_map<VkFence,bool> fences;static std::unordered_map<VkBuffer,bool> buffers;static bool fail_submit=false;static int retired=0;
static VkResult vkGetFenceStatus(VkDevice,VkFence f){return fences.at(f)?VK_SUCCESS:VK_NOT_READY;}
static VkResult vkWaitForFences(VkDevice,uint32_t n,const VkFence* f,VkBool32,uint64_t){for(uint32_t i=0;i<n;++i)fences.at(f[i])=true;return VK_SUCCESS;}
static VkResult vkAllocateCommandBuffers(VkDevice,const VkCommandBufferAllocateInfo*,VkCommandBuffer* cmd){static uintptr_t n=100;*cmd=reinterpret_cast<VkCommandBuffer>(n++);return VK_SUCCESS;}
static VkResult vkCreateFence(VkDevice,const VkFenceCreateInfo*,const VkAllocationCallbacks*,VkFence* fence){static uintptr_t n=1;*fence=reinterpret_cast<VkFence>(n++);fences[*fence]=false;return VK_SUCCESS;}
static VkResult vkResetFences(VkDevice,uint32_t,const VkFence* fence){fences[*fence]=false;return VK_SUCCESS;}
static VkResult vkResetCommandBuffer(VkCommandBuffer,VkCommandBufferResetFlags){return VK_SUCCESS;}
static VkResult vkBeginCommandBuffer(VkCommandBuffer,const VkCommandBufferBeginInfo*){return VK_SUCCESS;}
static VkResult vkEndCommandBuffer(VkCommandBuffer){return VK_SUCCESS;}
static VkResult vkQueueSubmit2(VkQueue,uint32_t,const VkSubmitInfo2* submit,VkFence){assert(submit->commandBufferInfoCount==1);return fail_submit?VK_ERROR_OUT_OF_HOST_MEMORY:VK_SUCCESS;}
static bool Check(VkResult r,const char*){return r==VK_SUCCESS;}
struct Context {
 VkDevice device{};VkQueue queue{};VkCommandPool upload_pool_{};
 struct TextureUpload{VkCommandBuffer cmd{};VkFence fence{};Buffer staging;bool pending=false;};
 std::array<TextureUpload,3> texture_uploads_{};size_t next_texture_upload_=0;
 struct TextureUploadStats{uint64_t submitted=0,waits=0,peak_bytes=0;double wait_ms=0;}texture_upload_stats_;
 void CheckDeviceLost(VkResult,const char*){}
 void DestroyBuffer(Buffer& b){if(b.buffer){assert(buffers.erase(b.buffer)==1);++retired;}b={};}
 bool Submit(const std::function<void(VkCommandBuffer)>& fn){fn(nullptr);return !fail_submit;}
 void RetireTextureUploads(bool wait=false);
 bool SubmitTextureUpload(const std::function<void(VkCommandBuffer)>&,Buffer&);
};
static Buffer Make(uintptr_t id,VkDeviceSize size=32ull*1024*1024){Buffer b{reinterpret_cast<VkBuffer>(id),size};buffers[b.buffer]=true;return b;}
}
int main(){using namespace pt::vk;Context context;int recordings=0;
 auto queue=[&](Buffer& b){VkBuffer handle=b.buffer;return context.SubmitTextureUpload([&](VkCommandBuffer){assert(buffers.contains(handle));++recordings;},b);};
 for(int i=0;i<3;++i){auto b=Make(i+1);assert(queue(b)&&!b.buffer);}
 assert(buffers.size()==3&&retired==0&&context.texture_upload_stats_.waits==0);
 context.RetireTextureUploads();assert(buffers.size()==3&&retired==0);
 auto fourth=Make(4);assert(queue(fourth)&&!fourth.buffer&&buffers.size()==3&&retired==1&&context.texture_upload_stats_.waits==1);
 assert(context.texture_upload_stats_.peak_bytes==96ull*1024*1024);
 fences[context.texture_uploads_[1].fence]=true;context.RetireTextureUploads();assert(buffers.size()==2&&retired==2);
 auto fifth=Make(5);assert(queue(fifth)&&context.texture_upload_stats_.waits==1);
 fences[context.texture_uploads_[2].fence]=true;context.RetireTextureUploads();
 auto failed=Make(6);fail_submit=true;assert(!queue(failed)&&failed.buffer&&buffers.contains(failed.buffer));fail_submit=false;context.DestroyBuffer(failed);
 context.RetireTextureUploads(true);assert(buffers.empty());for(auto& slot:context.texture_uploads_)assert(!slot.pending&&!slot.staging.buffer);
 auto large=Make(7,64ull*1024*1024);assert(queue(large)&&large.buffer);context.DestroyBuffer(large);assert(buffers.empty()&&recordings==7);
 puts("Production async ring: fence retirement, 96 MiB bound, exhaustion-only wait, failed submission ownership and oversized fallback passed");
}
