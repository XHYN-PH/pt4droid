// Recording fixture: actual UploadMesh/QueueScreenshot/PollScreenshots bodies
// are appended by check_test19_refinements.py. No Vulkan driver is simulated.
#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>
#include <algorithm>
#include <array>
#include <atomic>
#include <cassert>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <functional>
#include <future>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <vector>
#include "engine/render/hdr_output.h"
#include "engine/core/hitch_trace.h"
VkResult vmaFlushAllocation(VmaAllocator,VmaAllocation,VkDeviceSize,VkDeviceSize){return VK_SUCCESS;}
VkResult vmaInvalidateAllocation(VmaAllocator,VmaAllocation,VkDeviceSize,VkDeviceSize){return VK_SUCCESS;}
namespace pt {
template<class... T> void LogError(const char*,T...) {}
template<class T> T Handle(uintptr_t n){return reinterpret_cast<T>(n);}
static std::atomic<uintptr_t> serial{100};
static std::atomic<unsigned> buffers{0},pools{0},fence_count{0},encoded{0},composites{0};
static int fail_stage=0;static bool unmapped=false,fail_upload=false;
static std::mutex fence_mutex;static std::condition_variable fence_changed;
struct FenceState {bool signalled=false;VkResult result=VK_SUCCESS;};
static std::unordered_map<VkFence,FenceState> fences;
static void SignalAll(VkResult result=VK_SUCCESS){std::lock_guard lock(fence_mutex);for(auto& [id,f]:fences){f.signalled=true;f.result=result;}fence_changed.notify_all();}
static VkResult vkCreateCommandPool(VkDevice,const VkCommandPoolCreateInfo*,const VkAllocationCallbacks*,VkCommandPool* p){if(fail_stage==2)return VK_ERROR_OUT_OF_HOST_MEMORY;*p=Handle<VkCommandPool>(serial++);++pools;return VK_SUCCESS;}
static VkResult vkCreateFence(VkDevice,const VkFenceCreateInfo*,const VkAllocationCallbacks*,VkFence* f){if(fail_stage==3)return VK_ERROR_OUT_OF_HOST_MEMORY;*f=Handle<VkFence>(serial++);{std::lock_guard lock(fence_mutex);fences[*f]={};}++fence_count;return VK_SUCCESS;}
static void vkDestroyFence(VkDevice,VkFence f,const VkAllocationCallbacks*){std::lock_guard lock(fence_mutex);assert(fences.erase(f)==1);--fence_count;}
static void vkDestroyCommandPool(VkDevice,VkCommandPool,const VkAllocationCallbacks*){--pools;}
static VkResult vkAllocateCommandBuffers(VkDevice,const VkCommandBufferAllocateInfo*,VkCommandBuffer* c){if(fail_stage==4)return VK_ERROR_OUT_OF_HOST_MEMORY;*c=Handle<VkCommandBuffer>(serial++);return VK_SUCCESS;}
static VkResult vkBeginCommandBuffer(VkCommandBuffer,const VkCommandBufferBeginInfo*){return fail_stage==5?VK_ERROR_OUT_OF_HOST_MEMORY:VK_SUCCESS;}
static VkResult vkEndCommandBuffer(VkCommandBuffer){return fail_stage==6?VK_ERROR_OUT_OF_HOST_MEMORY:VK_SUCCESS;}
static VkResult vkQueueSubmit2(VkQueue,uint32_t,const VkSubmitInfo2* s,VkFence){assert(s->commandBufferInfoCount==1);return fail_stage==7?VK_ERROR_OUT_OF_HOST_MEMORY:VK_SUCCESS;}
static VkResult vkWaitForFences(VkDevice,uint32_t n,const VkFence* f,VkBool32,uint64_t){assert(n==1);std::unique_lock lock(fence_mutex);fence_changed.wait(lock,[&]{return fences.at(*f).signalled;});return fences.at(*f).result;}
static std::vector<std::pair<VkBuffer,VkBufferCopy>> copies;
static void vkCmdCopyBuffer(VkCommandBuffer,VkBuffer,VkBuffer dst,uint32_t n,const VkBufferCopy* r){assert(n==1);copies.emplace_back(dst,*r);}
static void vkCmdPipelineBarrier2(VkCommandBuffer,const VkDependencyInfo* d){
    for(unsigned i=0;i<d->bufferMemoryBarrierCount;++i){auto& b=d->pBufferMemoryBarriers[i];assert(b.srcAccessMask==VK_ACCESS_2_TRANSFER_WRITE_BIT&&b.size==VK_WHOLE_SIZE);
        assert(b.dstAccessMask==VK_ACCESS_2_HOST_READ_BIT||b.dstAccessMask==VK_ACCESS_2_MEMORY_READ_BIT);}
}
static void vkCmdBeginRendering(VkCommandBuffer,const VkRenderingInfo*){}
static void vkCmdEndRendering(VkCommandBuffer){}
static void vkCmdCopyImageToBuffer(VkCommandBuffer,VkImage,VkImageLayout layout,VkBuffer,uint32_t n,const VkBufferImageCopy* r){assert(n==1&&layout==VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);assert(r->imageExtent.width&&r->imageExtent.height);}
namespace vk {
struct Context {
    VkDevice device{};VkQueue queue{};uint32_t queue_family=0;VmaAllocator allocator=nullptr;
    Buffer owned;std::vector<uint8_t> staged;
    void CheckDeviceLost(VkResult,const char*){}
    bool CreateBuffer(Buffer& b,VkDeviceSize bytes,VkBufferUsageFlags,bool){if(fail_stage==1)return false;b.buffer=Handle<VkBuffer>(serial++);b.size=bytes;b.allocation=reinterpret_cast<VmaAllocation>(new uint8_t[size_t(bytes)]);b.mapped=unmapped?nullptr:b.allocation;++buffers;return true;}
    void DestroyBuffer(Buffer& b){if(b.buffer){delete[] reinterpret_cast<uint8_t*>(b.allocation);--buffers;}b={};}
    bool SubmitTextureUpload(const std::function<void(VkCommandBuffer)>& record,Buffer& b){staged.assign(static_cast<uint8_t*>(b.mapped),static_cast<uint8_t*>(b.mapped)+b.size);record(nullptr);if(fail_upload)return false;owned=b;b={};return true;}
    bool UploadMesh(Buffer&,const void*,VkDeviceSize,Buffer&,const void*,VkDeviceSize);
};
void ImageBarrier(VkCommandBuffer,VkImage,VkImageAspectFlags,VkPipelineStageFlags2,VkAccessFlags2,VkImageLayout,VkPipelineStageFlags2,VkAccessFlags2,VkImageLayout,uint32_t,uint32_t){}
}
static bool EncodeScreenshot(const std::filesystem::path&,uint32_t w,uint32_t h,VkFormat,const void* data){assert(w==32&&h==16&&data);++encoded;return true;}
struct Renderer {
    vk::Context ctx_;struct Image{VkImage image=Handle<VkImage>(1);VkImageView view{};VkExtent3D extent{64,32,1};VkFormat format=VK_FORMAT_R8G8B8A8_UNORM;}output_;
    bool final_ready_=true,output_ready_=false;VkDescriptorSet final_set_{};
    struct Photo{std::future<bool> result;std::function<void(bool)> complete;};std::vector<Photo> photos_;
    Renderer(){photos_.reserve(2);}
    bool QueueScreenshot(const std::filesystem::path&,glm::vec4,std::function<void(bool)>);
    void PollScreenshots();
    void Composite(VkCommandBuffer,VkDescriptorSet,float,VkExtent2D,VkFormat){++composites;}
};
}
int main(){using namespace pt;
    vk::Context context;vk::Buffer v,i;v.buffer=Handle<VkBuffer>(1);i.buffer=Handle<VkBuffer>(2);v.size=i.size=64;
    const uint8_t vertex[]{1,2,3,4,5},index[]{6,7,8};
    assert(!context.UploadMesh(v,nullptr,5,i,index,3));assert(buffers==0);
    fail_stage=1;assert(!context.UploadMesh(v,vertex,5,i,index,3));assert(buffers==0);fail_stage=0;
    unmapped=true;assert(!context.UploadMesh(v,vertex,5,i,index,3));assert(buffers==0);unmapped=false;
    fail_upload=true;assert(!context.UploadMesh(v,vertex,5,i,index,3));assert(buffers==0);fail_upload=false;copies.clear();
    assert(context.UploadMesh(v,vertex,5,i,index,3));assert(buffers==1&&context.owned.buffer);
    assert(context.staged.size()==19&&std::equal(std::begin(vertex),std::end(vertex),context.staged.begin())&&std::equal(std::begin(index),std::end(index),context.staged.begin()+16));
    assert(copies.size()==2&&copies[0].first==v.buffer&&copies[0].second.size==5&&copies[1].first==i.buffer&&copies[1].second.srcOffset==16&&copies[1].second.size==3);
    context.DestroyBuffer(context.owned);assert(buffers==0);
    Renderer r;const glm::vec4 crop{.25f,.25f,.5f,.5f};unsigned callbacks=0,success=0;
    auto done=[&](bool ok){++callbacks;success+=ok;};
    for(int stage=1;stage<=7;++stage){fail_stage=stage;assert(!r.QueueScreenshot("photo.png",crop,done));assert(buffers==0&&pools==0&&fence_count==0&&r.photos_.empty()&&callbacks==0);}
    fail_stage=0;assert(r.QueueScreenshot("photo.png",crop,done));assert(r.QueueScreenshot("photo2.png",crop,done));
    assert(!r.QueueScreenshot("overflow.png",crop,done)&&r.photos_.size()==2&&buffers==2&&pools==2&&fence_count==2&&encoded==0);
    r.PollScreenshots();assert(callbacks==0);SignalAll();
    for(auto& p:r.photos_)p.result.wait();r.PollScreenshots();assert(callbacks==2&&success==2&&encoded==2&&buffers==0&&pools==0&&fence_count==0&&r.photos_.empty());
    r.output_ready_=true;auto before=composites.load();assert(r.QueueScreenshot("gpu-failure.png",crop,done));assert(composites==before);SignalAll(VK_ERROR_DEVICE_LOST);
    for(auto& p:r.photos_)p.result.wait();r.PollScreenshots();assert(callbacks==3&&success==2&&encoded==2&&buffers==0&&pools==0&&fence_count==0);
    assert(!r.QueueScreenshot("empty.png",{0,0,0,0},done));r.final_ready_=false;assert(!r.QueueScreenshot("unready.png",crop,done));
    puts("Production GPU boundaries: aligned mesh copy bytes/barriers/ownership, seven photo allocation/submission failures, bounded nonblocking queue, fence-gated cleanup and GPU failure callbacks passed");
}
