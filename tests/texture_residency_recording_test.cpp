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
#include "engine/assets/compatible_texture.h"
#include <unordered_set>
#include <memory>
namespace pt {
template<class...T>void LogWarn(const char*,T...){}template<class...T>void LogDebug(const char*,T...){}
static VkResult vkDeviceWaitIdle(VkDevice){return VK_SUCCESS;}
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
struct Context{void RetireTextureUploads(bool){}
VkPhysicalDevice physical{};VkDevice device{};void*allocator{};bool memory_budget=false;
 bool CreateImage(Image&o,VkFormat f,VkExtent3D,VkImageUsageFlags,uint32_t levels,uint32_t layers,VkImageAspectFlags,bool){static uintptr_t serial=1;o.image=handle<VkImage>(serial++);o.view=handle<VkImageView>(serial++);records.emplace(o.image,RecordedImage{f,levels,layers,std::vector<std::vector<uint8_t>>(levels*layers)});return true;}
 void DestroyImage(Image&o){records.erase(o.image);o={};}
 bool CreateBuffer(Buffer&o,size_t size,VkBufferUsageFlags,bool){if(fail_buffer)return false;static uintptr_t serial=100000;o.buffer=handle<VkBuffer>(serial++);o.size=size;o.mapped=new uint8_t[size];o.allocation=o.mapped;buffers[o.buffer]={static_cast<uint8_t*>(o.mapped),size};return true;}
 void DestroyBuffer(Buffer&o){if(o.buffer){buffers.erase(o.buffer);delete[]static_cast<uint8_t*>(o.mapped);}o={};}
 bool SubmitTextureUpload(const std::function<void(VkCommandBuffer)>& fn,Buffer&){return Submit(fn);}
 bool Submit(const std::function<void(VkCommandBuffer)>&fn){++submissions;if(fail_submit)return false;fn(nullptr);return true;}
};}
struct TextureMip{uint32_t width,height;std::span<const uint8_t>data;};
struct TextureManager{static constexpr uint32_t kWhite=0,kMaxTextures=8192,kMaxCubeTextures=64;
 vk::Context context;vk::Context*ctx_=&context;VkSampler sampler_{},base_sampler_{};VkDescriptorSet set_{};
 std::vector<vk::Image>images_;std::vector<uint8_t>cube_;std::unordered_map<std::string,uint32_t>by_name_;std::unordered_map<uint32_t,uint32_t>cube_slots_;
 struct PendingUpload{VkImage image;std::vector<VkBufferImageCopy>regions;};vk::Buffer upload_staging_;size_t upload_used_=0;std::vector<PendingUpload>upload_pending_;uint32_t upload_batch_depth_=0;uint64_t fallback_bytes_=0;double upload_ms_=0;
 struct FormatSupport{VkFormatProperties properties{};VkImageFormatProperties image{};bool valid=false;};std::unordered_map<uint64_t,FormatSupport>format_support_;
 struct ConvertedImage {std::filesystem::path file;uint64_t bytes=0,last_use=0;VkFormat format=VK_FORMAT_UNDEFINED;bool failed=false;};
 std::unordered_map<uint32_t,ConvertedImage> converted_images_;std::unordered_set<uint32_t> texture_use_;
 std::unordered_map<std::string,std::string>failed_textures_;std::unordered_map<uint32_t,uint32_t>redirects_,enhanced_images_;
 bool enhanced_enabled_=false,restoring_=false,residency_warning_=false;uint64_t texture_epoch_=1,residency_limit_=1024;
 void WriteResidentDescriptor(uint32_t);void Touch(uint32_t);void TouchCube(uint32_t);void EvictConverted(uint64_t);void FinishTextureUse();
 bool SupportsTexture(VkFormat,uint32_t,uint32_t,uint32_t,uint32_t,bool);void BeginUploadBatch();void EndUploadBatch();void FlushUploads();uint32_t Create(const std::string&,VkFormat,std::span<const TextureMip>,uint32_t=1,bool=false,std::shared_ptr<const BcMipChain> = {});
 ~TextureManager(){for(auto&i:images_)ctx_->DestroyImage(i);ctx_->DestroyBuffer(upload_staging_);}
};}

int main(){using namespace pt;
 const auto cache=std::filesystem::temp_directory_path()/"pt13-residency-recording";std::filesystem::remove_all(cache);
 std::array<bool,16> native{};ConfigureBcConversion(native,cache,0);native_bc=false;
 std::vector<uint8_t> white(4,255),red{0,248,0,248,0,0,0,0};TextureMip builtin{1,1,white},source{4,4,red};
 {
  TextureManager m;for(int i=0;i<4;++i)m.Create("builtin"+std::to_string(i),VK_FORMAT_R8G8B8A8_UNORM,{&builtin,1});
  m.residency_limit_=64;
  auto a=m.Create("a",VK_FORMAT_BC1_RGBA_UNORM_BLOCK,{&source,1});assert(a==4 && m.fallback_bytes_==64 && m.images_[a].image);
  auto b=m.Create("b",VK_FORMAT_BC1_RGBA_UNORM_BLOCK,{&source,1});assert(b==5 && !m.images_[a].image && m.images_[b].image && m.fallback_bytes_==64);
  m.texture_use_={a};m.FinishTextureUse();assert(m.images_[a].image && !m.images_[b].image && m.images_.size()==6 && m.fallback_bytes_==64);
  const auto image=m.images_[a].image;auto payload=records.at(image).bytes;
  m.texture_use_={a,b};m.FinishTextureUse();assert(m.images_[a].image==image && !m.images_[b].image && m.fallback_bytes_==64);
  m.texture_use_={b};m.FinishTextureUse();assert(!m.images_[a].image && m.images_[b].image && records.at(m.images_[b].image).bytes==payload);
  // Cached handle, name and material descriptor indices survive many evictions.
  for(int i=0;i<12;++i){m.texture_use_={i%2?a:b};m.FinishTextureUse();assert(m.images_.size()==6 && m.cube_slots_.empty() && m.by_name_.at("a")==a && m.by_name_.at("b")==b && m.fallback_bytes_==64);}
  // Failed backing is reported once; an unrelated live texture is retained.
  const uint32_t live=m.images_[a].image?a:b,dead=live==a?b:a;
  std::filesystem::remove(m.converted_images_.at(dead).file);m.texture_use_={live,dead};m.residency_limit_=128;m.FinishTextureUse();assert(m.converted_images_.at(dead).failed && m.images_[live].image);
 }
 assert(records.empty() && buffers.empty());std::filesystem::remove_all(cache);
 // Native compressed bytes remain exactly as supplied; compatibility limit is ignored.
 native.fill(true);ConfigureBcConversion(native,cache,512);native_bc=true;
 {TextureManager m;m.Create("white",VK_FORMAT_R8G8B8A8_UNORM,{&builtin,1});auto i=m.Create("native",VK_FORMAT_BC1_RGBA_UNORM_BLOCK,{&source,1});assert(i==1 && m.converted_images_.empty() && m.fallback_bytes_==0 && records.at(m.images_[i].image).bytes[0]==red);}
 assert(records.empty() && buffers.empty());puts("Production residency checks passed: live protection, budget, stable slots, eviction/restoration bytes, failure isolation and untouched native uploads");
}
