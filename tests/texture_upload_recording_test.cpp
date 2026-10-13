#include <vulkan/vulkan.h>
#include <algorithm>
#include <array>
#include <bit>
#include <cassert>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <functional>
#include <span>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>
#include "engine/assets/bc_texture.h"
namespace pt {
template<class T>T handle(uintptr_t n){return reinterpret_cast<T>(n);}
struct RecordedImage{VkFormat format;uint32_t levels,layers;std::vector<std::vector<uint8_t>> bytes;};
static std::unordered_map<VkImage,RecordedImage> records;static std::unordered_map<VkBuffer,std::pair<uint8_t*,size_t>> buffers;
static bool native_bc=true,fail_buffer=false,fail_submit=false;static uint32_t submissions=0,decodes=0,format_queries=0;
template<class...T>void LogInfo(const char*,T...){}template<class...T>void LogError(const char*,T...){}
uint32_t FormatBlockBytes(VkFormat,bool&);
static void vkGetPhysicalDeviceFormatProperties(VkPhysicalDevice,VkFormat f,VkFormatProperties*out){++format_queries;bool bc=f>=VK_FORMAT_BC1_RGB_UNORM_BLOCK&&f<=VK_FORMAT_BC7_SRGB_BLOCK;out->optimalTilingFeatures=!bc||native_bc?VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT|VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT|VK_FORMAT_FEATURE_TRANSFER_DST_BIT:0;}
static VkResult vkGetPhysicalDeviceImageFormatProperties(VkPhysicalDevice,VkFormat,VkImageType,VkImageTiling,VkImageUsageFlags,VkImageCreateFlags,VkImageFormatProperties*out){out->maxExtent={16384,16384,1};out->maxMipLevels=15;out->maxArrayLayers=6;return VK_SUCCESS;}
struct VmaBudget{uint64_t budget=0,usage=0;};static void vmaGetHeapBudgets(void*,VmaBudget*out){out[0].budget=64ull*1024*1024;}
static void vkGetPhysicalDeviceMemoryProperties(VkPhysicalDevice,VkPhysicalDeviceMemoryProperties*out){out->memoryHeapCount=1;out->memoryHeaps[0].flags=VK_MEMORY_HEAP_DEVICE_LOCAL_BIT;}
static void vmaFlushAllocation(void*,void*,size_t,size_t){}
static void vkCmdCopyBufferToImage(VkCommandBuffer,VkBuffer buffer,VkImage image,VkImageLayout,uint32_t count,const VkBufferImageCopy*regions){auto&b=buffers.at(buffer);auto&r=records.at(image);for(uint32_t i=0;i<count;++i){auto&v=regions[i];bool bc;uint32_t block=FormatBlockBytes(r.format,bc);size_t n=bc?size_t((v.imageExtent.width+3)/4)*((v.imageExtent.height+3)/4)*block:size_t(v.imageExtent.width)*v.imageExtent.height*block;assert(v.bufferOffset+n<=b.second&&v.imageSubresource.mipLevel<r.levels&&v.imageSubresource.baseArrayLayer<r.layers);r.bytes[v.imageSubresource.baseArrayLayer*r.levels+v.imageSubresource.mipLevel].assign(b.first+v.bufferOffset,b.first+v.bufferOffset+n);}}
static void vkUpdateDescriptorSets(VkDevice,uint32_t n,const VkWriteDescriptorSet*w,uint32_t,const VkCopyDescriptorSet*){for(uint32_t i=0;i<n;++i)assert(w[i].pImageInfo->imageView!=VK_NULL_HANDLE);}
static bool CountedDecodeBcMip(VkFormat f,uint32_t w,uint32_t h,std::span<const uint8_t>d,std::vector<uint8_t>&o){++decodes;return DecodeBcMip(f,w,h,d,o);}
namespace vk {
struct Image{VkImage image{};VkImageView view{};};struct Buffer{VkBuffer buffer{};void*allocation{};void*mapped{};size_t size=0;};
static void ImageBarrier(VkCommandBuffer,VkImage,VkImageAspectFlags,VkPipelineStageFlags2,VkAccessFlags2,VkImageLayout,VkPipelineStageFlags2,VkAccessFlags2,VkImageLayout){}
struct Context{VkPhysicalDevice physical{};VkDevice device{};void*allocator{};bool memory_budget=false;
 bool CreateImage(Image&o,VkFormat f,VkExtent3D,VkImageUsageFlags,uint32_t levels,uint32_t layers,VkImageAspectFlags,bool){static uintptr_t serial=1;o.image=handle<VkImage>(serial++);o.view=handle<VkImageView>(serial++);records.emplace(o.image,RecordedImage{f,levels,layers,std::vector<std::vector<uint8_t>>(levels*layers)});return true;}
 void DestroyImage(Image&o){records.erase(o.image);o={};}
 bool CreateBuffer(Buffer&o,size_t size,VkBufferUsageFlags,bool){if(fail_buffer)return false;static uintptr_t serial=100000;o.buffer=handle<VkBuffer>(serial++);o.size=size;o.mapped=new uint8_t[size];o.allocation=o.mapped;buffers[o.buffer]={static_cast<uint8_t*>(o.mapped),size};return true;}
 void DestroyBuffer(Buffer&o){if(o.buffer){buffers.erase(o.buffer);delete[]static_cast<uint8_t*>(o.mapped);}o={};}
 bool Submit(const std::function<void(VkCommandBuffer)>&fn){++submissions;if(fail_submit)return false;fn(nullptr);return true;}
};}
struct TextureMip{uint32_t width,height;std::span<const uint8_t>data;};
struct TextureManager{static constexpr uint32_t kWhite=0,kMaxTextures=8192,kMaxCubeTextures=64;
 vk::Context context;vk::Context*ctx_=&context;VkSampler sampler_{},base_sampler_{};VkDescriptorSet set_{};
 std::vector<vk::Image>images_;std::vector<uint8_t>cube_;std::unordered_map<std::string,uint32_t>by_name_;std::unordered_map<uint32_t,uint32_t>cube_slots_;
 struct PendingUpload{VkImage image;std::vector<VkBufferImageCopy>regions;};vk::Buffer upload_staging_;size_t upload_used_=0;std::vector<PendingUpload>upload_pending_;uint32_t upload_batch_depth_=0;uint64_t fallback_bytes_=0;double upload_ms_=0;
 struct FormatSupport{VkFormatProperties properties{};VkImageFormatProperties image{};bool valid=false;};std::unordered_map<uint64_t,FormatSupport>format_support_;
 bool SupportsTexture(VkFormat,uint32_t,uint32_t,uint32_t,uint32_t,bool);void BeginUploadBatch();void EndUploadBatch();void FlushUploads();uint32_t Create(const std::string&,VkFormat,std::span<const TextureMip>,uint32_t=1,bool=false);
 ~TextureManager(){for(auto&i:images_)ctx_->DestroyImage(i);ctx_->DestroyBuffer(upload_staging_);}
};}
int main(){using namespace pt;std::vector<uint8_t>red={0,248,0,248,0,0,0,0},white(4,255);TextureMip bc{4,4,red},builtin{1,1,white};
 {native_bc=true;TextureManager m;m.Create("white",VK_FORMAT_R8G8B8A8_UNORM,{&builtin,1});auto index=m.Create("native",VK_FORMAT_BC1_RGBA_UNORM_BLOCK,{&bc,1});assert(index==1&&decodes==0);auto&r=records.at(m.images_[index].image);assert(r.format==VK_FORMAT_BC1_RGBA_UNORM_BLOCK&&r.bytes[0]==red);auto before=format_queries;assert(m.Create("native",VK_FORMAT_BC1_RGBA_UNORM_BLOCK,{&bc,1})==index&&format_queries==before&&decodes==0);}
 {native_bc=false;TextureManager m;m.Create("white",VK_FORMAT_R8G8B8A8_UNORM,{&builtin,1});std::array<TextureMip,18>cube;for(size_t f=0;f<6;++f)for(size_t l=0;l<3;++l)cube[f*3+l]={4u>>l,4u>>l,red};auto index=m.Create("cube",VK_FORMAT_BC1_RGBA_SRGB_BLOCK,cube,6,true);assert(index==1&&decodes==18);auto&r=records.at(m.images_[index].image);assert(r.format==VK_FORMAT_R8G8B8A8_SRGB&&r.levels==3&&r.layers==6);for(size_t f=0;f<6;++f)for(size_t l=0;l<3;++l){auto&d=r.bytes[f*3+l];assert(d.size()==(4u>>l)*(4u>>l)*4);for(size_t i=0;i<d.size();i+=4)assert(d[i]==255&&d[i+1]==0&&d[i+2]==0&&d[i+3]==255);}auto bad=bc;bad.width=0;assert(m.Create("bad",VK_FORMAT_BC1_RGBA_UNORM_BLOCK,{&bad,1})==0);auto before=decodes;m.fallback_bytes_=2ull*1024*1024*1024;assert(m.Create("bounded",VK_FORMAT_BC1_RGBA_UNORM_BLOCK,{&bc,1})==0&&before==decodes);m.fallback_bytes_=0;m.context.memory_budget=true;assert(m.Create("budget",VK_FORMAT_BC1_RGBA_UNORM_BLOCK,{&bc,1})==0&&before==decodes);}
 {native_bc=true;submissions=0;TextureManager m;m.BeginUploadBatch();std::vector<uint8_t>d(2048*1024*4,17);TextureMip large{2048,1024,d};for(int i=0;i<3;++i){d[0]=uint8_t(i);m.Create("batch"+std::to_string(i),VK_FORMAT_R8G8B8A8_UNORM,{&large,1});}assert(submissions==1&&m.upload_pending_.size()==1&&buffers.size()==1);m.EndUploadBatch();assert(submissions==2&&buffers.empty());for(int i=0;i<3;++i)assert(records.at(m.images_[i].image).bytes[0][0]==i);}
 {TextureManager m;submissions=0;m.BeginUploadBatch();m.BeginUploadBatch();for(int i=0;i<70;++i)m.Create("small"+std::to_string(i),VK_FORMAT_R8G8B8A8_UNORM,{&builtin,1});assert(submissions==1);m.EndUploadBatch();assert(submissions==1);m.EndUploadBatch();assert(submissions==2&&buffers.empty());}
 {TextureManager m;fail_buffer=true;assert(m.Create("allocation",VK_FORMAT_R8G8B8A8_UNORM,{&builtin,1})==0&&records.empty());fail_buffer=false;fail_submit=true;assert(m.Create("queue",VK_FORMAT_R8G8B8A8_UNORM,{&builtin,1})==0&&records.empty()&&buffers.empty());m.BeginUploadBatch();m.Create("queued",VK_FORMAT_R8G8B8A8_UNORM,{&builtin,1});bool threw=false;try{m.EndUploadBatch();}catch(const std::runtime_error&){threw=true;}assert(threw);fail_submit=false;m.FlushUploads();}
 assert(buffers.empty()&&records.empty());puts("Production uploads passed: zero Adreno conversion, byte equality, sRGB/mip/cube fallback, bounded staging, nested batches, caps and failures");}
