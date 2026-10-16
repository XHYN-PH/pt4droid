#include "engine/core/pathcode.h"
#include "engine/render/texture_manager.h"

#include <stb_image.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstring>
#include <format>
#include <stdexcept>
#include <bit>
#include "engine/assets/bc_texture.h"
#include "engine/render/gpu_types.h"
#include "engine/platform/android.h"
#include <SDL3/SDL_system.h>

#include "engine/assets/ftex.h"
#include "engine/assets/enhanced_textures.h"
#include "engine/core/log.h"
#include "engine/fs/mods.h"

namespace pt {
namespace {

// A mod's PNG (docs/modding.md) as RGBA8 levels down to 1 x 1, each the average of four texels of the one above (in linear
// light for a colour texture). Uncompressed, so loading never stalls on an encoder.
bool ModImageLevels(const std::vector<uint8_t>& png, bool srgb, std::vector<std::vector<uint8_t>>& levels, uint32_t& width,
                    uint32_t& height, std::string& error) {
    int w = 0, h = 0, channels = 0;
    stbi_uc* pixels = stbi_load_from_memory(png.data(), static_cast<int>(png.size()), &w, &h, &channels, 4);
    if (!pixels) {
        error = stbi_failure_reason() ? stbi_failure_reason() : "not a PNG";
        return false;
    }
    if (w <= 0 || h <= 0 || w > 8192 || h > 8192) {
        stbi_image_free(pixels);
        error = std::format("{} x {} is outside 1 to 8192 px", w, h);
        return false;
    }
    width = static_cast<uint32_t>(w);
    height = static_cast<uint32_t>(h);
    levels.clear();
    levels.emplace_back(pixels, pixels + size_t(width) * height * 4);
    stbi_image_free(pixels);
    float to_linear[256];
    for (int i = 0; i < 256; ++i) {
        const float f = i / 255.0f;
        to_linear[i] = srgb ? (f <= 0.04045f ? f / 12.92f : std::pow((f + 0.055f) / 1.055f, 2.4f)) : f;
    }
    uint32_t lw = width, lh = height;
    while (lw > 1 || lh > 1) {
        const uint32_t nw = std::max(1u, lw / 2), nh = std::max(1u, lh / 2);
        const std::vector<uint8_t>& above = levels.back();
        std::vector<uint8_t> next(size_t(nw) * nh * 4);
        for (uint32_t y = 0; y < nh; ++y) {
            for (uint32_t x = 0; x < nw; ++x) {
                for (uint32_t c = 0; c < 4; ++c) {
                    float sum = 0.0f;
                    for (uint32_t dy = 0; dy < 2; ++dy) {
                        for (uint32_t dx = 0; dx < 2; ++dx) {
                            const uint8_t v = above[(size_t(std::min(y * 2 + dy, lh - 1)) * lw + std::min(x * 2 + dx, lw - 1)) * 4 + c];
                            sum += c < 3 ? to_linear[v] : v / 255.0f;
                        }
                    }
                    float f = sum * 0.25f;
                    if (srgb && c < 3) {
                        f = f <= 0.0031308f ? f * 12.92f : 1.055f * std::pow(f, 1.0f / 2.4f) - 0.055f;
                    }
                    next[(size_t(y) * nw + x) * 4 + c] = static_cast<uint8_t>(std::clamp(std::lround(f * 255.0f), 0l, 255l));
                }
            }
        }
        levels.push_back(std::move(next));
        lw = nw;
        lh = nh;
    }
    return true;
}

}

uint32_t FormatBlockBytes(VkFormat format, bool& compressed) {
    compressed = true;
    switch (format) {
    case VK_FORMAT_BC1_RGB_UNORM_BLOCK:
    case VK_FORMAT_BC1_RGB_SRGB_BLOCK:
    case VK_FORMAT_BC1_RGBA_UNORM_BLOCK:
    case VK_FORMAT_BC1_RGBA_SRGB_BLOCK:
    case VK_FORMAT_BC4_UNORM_BLOCK:
    case VK_FORMAT_BC4_SNORM_BLOCK: return 8;
    case VK_FORMAT_BC2_UNORM_BLOCK:
    case VK_FORMAT_BC2_SRGB_BLOCK:
    case VK_FORMAT_BC3_UNORM_BLOCK:
    case VK_FORMAT_BC3_SRGB_BLOCK:
    case VK_FORMAT_BC5_UNORM_BLOCK:
    case VK_FORMAT_BC5_SNORM_BLOCK:
    case VK_FORMAT_BC6H_UFLOAT_BLOCK:
    case VK_FORMAT_BC6H_SFLOAT_BLOCK:
    case VK_FORMAT_BC7_UNORM_BLOCK:
    case VK_FORMAT_BC7_SRGB_BLOCK: return 16;
    default: break;
    }
    compressed = false;
    switch (format) {
    case VK_FORMAT_R8_SNORM:
    case VK_FORMAT_R8_UNORM: return 1;
    case VK_FORMAT_R8G8_SNORM:
    case VK_FORMAT_R8G8_UNORM: return 2;
    case VK_FORMAT_R16_SFLOAT: return 2;
    case VK_FORMAT_R8G8B8A8_UNORM:
    case VK_FORMAT_R8G8B8A8_SRGB:
    case VK_FORMAT_B8G8R8A8_UNORM:
    case VK_FORMAT_B8G8R8A8_SRGB:
    case VK_FORMAT_R32_SFLOAT: return 4;
    case VK_FORMAT_R16G16B16A16_SFLOAT: return 8;
    case VK_FORMAT_R32G32B32A32_SFLOAT: return 16;
    default: return 4;
    }
}

bool TextureManager::Init(vk::Context& ctx) {
    ctx_ = &ctx;
    std::array<bool,16> native{};
    for(size_t i=0;i<native.size();++i)native[i]=SupportsTexture(VkFormat(VK_FORMAT_BC1_RGB_UNORM_BLOCK+i),4,4,1,1,false);
    std::filesystem::path cache;
    uint32_t max_side=0;
#ifdef __ANDROID__
    const char* cache_root=SDL_GetAndroidCachePath();
    cache=std::filesystem::path(cache_root?cache_root:".")/"textures-bc-v1";
    max_side=android::PerformanceOptions().compat_texture_size;
#else
    std::error_code cache_error;cache=std::filesystem::temp_directory_path(cache_error)/"pt4droid-bc-v1";
#endif
    ConfigureBcConversion(native,cache,max_side);
    VkPhysicalDeviceMemoryProperties memory{};vkGetPhysicalDeviceMemoryProperties(ctx.physical,&memory);
    uint64_t heap=0;for(uint32_t i=0;i<memory.memoryHeapCount;++i)if(memory.memoryHeaps[i].flags&VK_MEMORY_HEAP_DEVICE_LOCAL_BIT)heap=std::max(heap,memory.memoryHeaps[i].size);
    residency_limit_=std::clamp<uint64_t>(heap/4,256ull*1024*1024,1536ull*1024*1024);

    base_sampler_ = CreateSampler(0);
    sampler_ = base_sampler_;

    VkDescriptorSetLayoutBinding bindings[3]{};
    bindings[0] = {0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, kMaxTextures,
                   VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT, nullptr};
    bindings[1] = {1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1,
                   VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_COMPUTE_BIT, nullptr};
    bindings[2] = {2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, kMaxCubeTextures, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr};
    const VkDescriptorBindingFlags bindless = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT |
                                             VK_DESCRIPTOR_BINDING_UPDATE_UNUSED_WHILE_PENDING_BIT;
    VkDescriptorBindingFlags binding_flags[3] = {bindless, VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT, bindless};
    VkDescriptorSetLayoutBindingFlagsCreateInfo flags_info{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO};
    flags_info.bindingCount = 3;
    flags_info.pBindingFlags = binding_flags;
    VkDescriptorSetLayoutCreateInfo layout_info{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    layout_info.pNext = &flags_info;
    layout_info.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;
    layout_info.bindingCount = 3;
    layout_info.pBindings = bindings;
    if (!vk::Check(vkCreateDescriptorSetLayout(ctx.device, &layout_info, nullptr, &set_layout_), "texture set layout")) {
        return false;
    }
    VkDescriptorPoolSize sizes[2] = {{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, kMaxTextures + kMaxCubeTextures},
                                     {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1}};
    VkDescriptorPoolCreateInfo pool_info{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;
    pool_info.maxSets = 1;
    pool_info.poolSizeCount = 2;
    pool_info.pPoolSizes = sizes;
    vkCreateDescriptorPool(ctx.device, &pool_info, nullptr, &pool_);
    VkDescriptorSetAllocateInfo alloc{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
    alloc.descriptorPool = pool_;
    alloc.descriptorSetCount = 1;
    alloc.pSetLayouts = &set_layout_;
    if (!vk::Check(vkAllocateDescriptorSets(ctx.device, &alloc, &set_), "texture set")) {
        return false;
    }
    ctx.CreateBuffer(material_buffer_, sizeof(MaterialGpu) * kMaxMaterials, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, true);
    VkDescriptorBufferInfo buffer_info{material_buffer_.buffer, 0, VK_WHOLE_SIZE};
    VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
    write.dstSet = set_;
    write.dstBinding = 1;
    write.descriptorCount = 1;
    write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    write.pBufferInfo = &buffer_info;
    vkUpdateDescriptorSets(ctx.device, 1, &write, 0, nullptr);

    const uint8_t white[4] = {255, 255, 255, 255};
    const uint8_t flat[4] = {128, 128, 255, 255};
    const uint8_t black[4] = {0, 0, 0, 255};
    const TextureMip white_mip{1, 1, white};
    const TextureMip flat_mip{1, 1, flat};
    const TextureMip black_mip{1, 1, black};
    Create("builtin:white", VK_FORMAT_R8G8B8A8_UNORM, {&white_mip, 1});
    Create("builtin:flat_normal", VK_FORMAT_R8G8B8A8_UNORM, {&flat_mip, 1});
    Create("builtin:black", VK_FORMAT_R8G8B8A8_UNORM, {&black_mip, 1});
    const TextureMip black_faces[6] = {black_mip, black_mip, black_mip, black_mip, black_mip, black_mip};
    Create("builtin:black_cube", VK_FORMAT_R8G8B8A8_UNORM, black_faces, 6, true);
    const uint8_t grey[4] = {128, 128, 128, 255};
    const TextureMip grey_mip{1, 1, grey};
    Create("builtin:grey", VK_FORMAT_R8G8B8A8_UNORM, {&grey_mip, 1});
    Create("builtin:grey_srgb", VK_FORMAT_R8G8B8A8_SRGB, {&grey_mip, 1});
    AddMaterial(MaterialGpu{});
    FlushMaterials();
    return true;
}

TextureManager::~TextureManager() { StopDecodeWorkers(); }

void TextureManager::StopDecodeWorkers() {
    if (decode_queue_) {
        { std::lock_guard lock(decode_queue_->mutex); decode_queue_->stop = true; decode_queue_->jobs.clear(); }
        decode_queue_->ready.notify_all();
    }
    for (std::future<void>& worker : decode_workers_) {
        worker.wait();
    }
    decode_workers_.clear();
    decode_queue_.reset();
}

void TextureManager::Shutdown() {
    StopEnhancedWorker();
    StopDecodeWorkers();
    worker_prepared_.clear();
    read_stats_ = {};
    converted_images_.clear();texture_use_.clear();failed_textures_.clear();
    decoding_.clear();
    prepared_.clear();
    pending_preparations_.clear();
    prepared_variants_.clear();
    decode_order_.clear();
    if (!ctx_) {
        return;
    }
    FlushUploads();ctx_->RetireTextureUploads(true);ctx_->DestroyBuffer(upload_staging_);upload_pending_.clear();upload_used_=0;upload_batch_depth_=0;fallback_bytes_=0;format_support_.clear();
    for (vk::Image& image : images_) {
        ctx_->DestroyImage(image);
    }
    images_.clear();
    cube_.clear();
    by_name_.clear();
    cube_slots_.clear();
    fox_sources_.clear();
    enhanced_images_.clear();
    enhanced_bytes_ = 0;
    enhanced_qar_ = nullptr;
    enhanced_enabled_ = false;
    ctx_->DestroyBuffer(material_buffer_);
    vkDestroyDescriptorPool(ctx_->device, pool_, nullptr);
    vkDestroyDescriptorSetLayout(ctx_->device, set_layout_, nullptr);
    if (sampler_ != base_sampler_) {
        vkDestroySampler(ctx_->device, sampler_, nullptr);
    }
    vkDestroySampler(ctx_->device, base_sampler_, nullptr);
    sampler_ = VK_NULL_HANDLE;
    base_sampler_ = VK_NULL_HANDLE;
    anisotropy_ = 0;
    ctx_ = nullptr;
}

uint32_t TextureManager::CubeSlot(uint32_t texture) const {
    auto it = cube_slots_.find(texture);
    return it == cube_slots_.end() ? kNoCube : it->second;
}

VkSampler TextureManager::CreateSampler(int anisotropy) const {
    VkSamplerCreateInfo sampler_info{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
    sampler_info.magFilter = VK_FILTER_LINEAR;
    sampler_info.minFilter = VK_FILTER_LINEAR;
    sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_info.anisotropyEnable = anisotropy > 1 ? VK_TRUE : VK_FALSE;
    sampler_info.maxAnisotropy = anisotropy > 1 ? static_cast<float>(anisotropy) : 1.0f;
    sampler_info.maxLod = VK_LOD_CLAMP_NONE;
    VkSampler sampler = VK_NULL_HANDLE;
    vk::Check(vkCreateSampler(ctx_->device, &sampler_info, nullptr, &sampler), "texture sampler");
    return sampler;
}

int TextureManager::SetAnisotropy(int level) {
    if (!ctx_) {
        return 0;
    }
    const int limit = static_cast<int>(ctx_->properties.limits.maxSamplerAnisotropy);
    level = level > 1 ? std::min(level, limit) : 0;
    if (level == anisotropy_) {
        return anisotropy_;
    }
    const VkSampler sampler = level > 1 ? CreateSampler(level) : base_sampler_;
    if (!sampler) {
        return anisotropy_;
    }
    // every texture descriptor takes the new sampler; frames in flight still read the old ones
    vkDeviceWaitIdle(ctx_->device);
    std::vector<VkDescriptorImageInfo> infos(images_.size());
    for (size_t i = 0; i < images_.size(); ++i) {
        uint32_t shown = static_cast<uint32_t>(i);
        if (enhanced_enabled_) {
            if (auto it = enhanced_images_.find(shown); it != enhanced_images_.end()) shown = it->second;
        }
        infos[i] = {sampler, cube_[i] ? images_[kWhite].view : (images_[shown].view ? images_[shown].view : images_[kWhite].view), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    }
    if (!infos.empty()) {
        VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
        write.dstSet = set_;
        write.dstBinding = 0;
        write.descriptorCount = static_cast<uint32_t>(infos.size());
        write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        write.pImageInfo = infos.data();
        vkUpdateDescriptorSets(ctx_->device, 1, &write, 0, nullptr);
    }
    if (sampler_ != base_sampler_) {
        vkDestroySampler(ctx_->device, sampler_, nullptr);
    }
    sampler_ = sampler;
    anisotropy_ = level;
    LogInfo("textures: anisotropic filtering {}", level > 1 ? std::format("{}x", level) : std::string("off (trilinear)"));
    return anisotropy_;
}

uint32_t TextureManager::Find(const std::string& name) const {
    auto it = by_name_.find(name);
    return it == by_name_.end() ? kWhite : it->second;
}

bool TextureManager::SupportsTexture(VkFormat format,uint32_t width,uint32_t height,uint32_t levels,uint32_t layers,bool cube){
    const uint64_t key=uint64_t(format)*2+cube;auto[it,inserted]=format_support_.try_emplace(key);auto& support=it->second;
    if(inserted){vkGetPhysicalDeviceFormatProperties(ctx_->physical,format,&support.properties);const auto required=VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT | VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT | VK_FORMAT_FEATURE_TRANSFER_DST_BIT;
        support.valid=(support.properties.optimalTilingFeatures & required)==required && vkGetPhysicalDeviceImageFormatProperties(ctx_->physical,format,VK_IMAGE_TYPE_2D,VK_IMAGE_TILING_OPTIMAL,VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,cube?VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT:0,&support.image)==VK_SUCCESS;}
    return support.valid && width<=support.image.maxExtent.width && height<=support.image.maxExtent.height && levels<=support.image.maxMipLevels && layers<=support.image.maxArrayLayers;
}
void TextureManager::BeginUploadBatch(){++upload_batch_depth_;}
void TextureManager::EndUploadBatch(){if(upload_batch_depth_ && --upload_batch_depth_==0){FlushUploads();ctx_->DestroyBuffer(upload_staging_);}}
void TextureManager::FlushUploads(){
    if(upload_pending_.empty())return;const auto started=std::chrono::steady_clock::now();vmaFlushAllocation(ctx_->allocator,upload_staging_.allocation,0,upload_used_);
    if(!ctx_->SubmitTextureUpload([&](VkCommandBuffer cmd){for(const auto& upload:upload_pending_){
        vk::ImageBarrier(cmd,upload.image,VK_IMAGE_ASPECT_COLOR_BIT,VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,0,VK_IMAGE_LAYOUT_UNDEFINED,VK_PIPELINE_STAGE_2_TRANSFER_BIT,VK_ACCESS_2_TRANSFER_WRITE_BIT,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        vkCmdCopyBufferToImage(cmd,upload_staging_.buffer,upload.image,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,uint32_t(upload.regions.size()),upload.regions.data());
        vk::ImageBarrier(cmd,upload.image,VK_IMAGE_ASPECT_COLOR_BIT,VK_PIPELINE_STAGE_2_TRANSFER_BIT,VK_ACCESS_2_TRANSFER_WRITE_BIT,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,VK_ACCESS_2_SHADER_SAMPLED_READ_BIT,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }}, upload_staging_))throw std::runtime_error("The driver could not submit the texture upload batch");
    upload_ms_+=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-started).count();upload_pending_.clear();upload_used_=0;
}

uint32_t TextureManager::Create(const std::string& name, VkFormat format, std::span<const TextureMip> mips, uint32_t layers, bool cube, std::shared_ptr<const BcMipChain> prepared) {
    if (auto it = by_name_.find(name); it != by_name_.end()) {
        return it->second;
    }
    if (failed_textures_.contains(name)) return kWhite;
    if (mips.empty() || !layers || mips.size()%layers || (cube && layers!=6) || images_.size() >= kMaxTextures) {
        return kWhite;
    }
    vk::Image image;
    uint32_t mip_count = static_cast<uint32_t>(mips.size()) / layers;
    bool compressed=false;const uint32_t block_bytes=FormatBlockBytes(format,compressed);
    if(!mips[0].width || !mips[0].height || mips[0].width>16384 || mips[0].height>16384 || mip_count>std::bit_width(std::max(mips[0].width,mips[0].height)) || (cube && mips[0].width!=mips[0].height))return kWhite;
    for(size_t i=0;i<mips.size();++i){const uint32_t level=uint32_t(i)%mip_count,w=std::max(1u,mips[0].width>>level),h=std::max(1u,mips[0].height>>level);const auto& mip=mips[i];const uint64_t bytes=compressed?uint64_t((w+3)/4)*((h+3)/4)*block_bytes:uint64_t(w)*h*block_bytes;
        if(mip.width!=w || mip.height!=h || mip.data.size()!=bytes){LogError("textures: {} invalid face/mip {} size",name,i);return kWhite;}}
    uint64_t converted_bytes=0;std::vector<TextureMip> compatible;
    if(!SupportsTexture(format,mips[0].width,mips[0].height,mip_count,layers,cube)){
        const VkFormat fallback=BcFallbackFormat(format);
        if(fallback==VK_FORMAT_UNDEFINED){failed_textures_[name]="unsupported format";LogError("textures: {} unsupported format {}",name,int(format));return kWhite;}
        if(!prepared){std::vector<BcSourceMip> source;for(const auto& mip:mips)source.push_back({mip.width,mip.height,mip.data});prepared=PrepareBcMips(format,source,layers);}
        if(!prepared || !SupportsTexture(fallback,prepared->width,prepared->height,prepared->levels,layers,cube)){
            failed_textures_[name]="invalid or unsupported conversion";LogError("textures: {} conversion failed for format {}",name,int(format));return kWhite;
        }
        converted_bytes=prepared->bytes;mip_count=prepared->levels;
        for(uint32_t face=0;face<layers;++face)for(uint32_t l=0;l<mip_count;++l)compatible.push_back({std::max(1u,prepared->width>>l),std::max(1u,prepared->height>>l),prepared->mips[face*mip_count+l]});
        // Protect current-frame references. During stage loading unused cached
        // images can be released; their stable descriptor slots are retained.
        EvictConverted(converted_bytes);
        if(fallback_bytes_+converted_bytes>residency_limit_ && !restoring_ && !prepared->cache_file.empty()){
            const uint32_t slot=uint32_t(images_.size());
            images_.push_back({});cube_.push_back(cube?1:0);by_name_[name]=slot;
            if(cube && cube_slots_.size()<kMaxCubeTextures)cube_slots_[slot]=uint32_t(cube_slots_.size());
            converted_images_[slot]={prepared->cache_file,converted_bytes,texture_epoch_,fallback,false};
            WriteResidentDescriptor(slot);return slot;
        }
        format=fallback;mips=compatible;
    }
    if (!ctx_->CreateImage(image, format, {mips[0].width, mips[0].height, 1}, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                           mip_count, layers, VK_IMAGE_ASPECT_COLOR_BIT, cube)) {
        return kWhite;
    }
    VkDeviceSize total = 0;
    for (const TextureMip& mip : mips) {
        total += (mip.data.size() + 15) & ~VkDeviceSize(15);
    }
    constexpr VkDeviceSize batch_limit=16ull*1024*1024;
    bool batched=upload_batch_depth_ && total<=batch_limit;
    if(batched && (upload_used_+total>batch_limit || upload_pending_.size()>=64))FlushUploads();
    if(batched && !upload_staging_.buffer)batched=ctx_->CreateBuffer(upload_staging_,batch_limit,VK_BUFFER_USAGE_TRANSFER_SRC_BIT,true);
    vk::Buffer staging;
    if(batched)staging=upload_staging_;else{FlushUploads();if(!ctx_->CreateBuffer(staging,total,VK_BUFFER_USAGE_TRANSFER_SRC_BIT,true)){ctx_->DestroyImage(image);return kWhite;}}
    std::vector<VkBufferImageCopy> regions;
    VkDeviceSize offset=batched?upload_used_:0;
    for (uint32_t layer = 0; layer < layers; ++layer) {
        for (uint32_t level = 0; level < mip_count; ++level) {
            const TextureMip& mip = mips[layer * mip_count + level];
            std::memcpy(static_cast<uint8_t*>(staging.mapped) + offset, mip.data.data(), mip.data.size());
            VkBufferImageCopy region{};
            region.bufferOffset = offset;
            region.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, level, layer, 1};
            region.imageExtent = {mip.width, mip.height, 1};
            regions.push_back(region);
            offset += (mip.data.size() + 15) & ~VkDeviceSize(15);
        }
    }
    if(batched){upload_pending_.push_back({image.image,std::move(regions)});upload_used_=offset;}else{
    vmaFlushAllocation(ctx_->allocator, staging.allocation, 0, total);
    const bool submitted=ctx_->SubmitTextureUpload([&](VkCommandBuffer cmd) {
        vk::ImageBarrier(cmd, image.image, VK_IMAGE_ASPECT_COLOR_BIT, VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT, 0, VK_IMAGE_LAYOUT_UNDEFINED,
                         VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        vkCmdCopyBufferToImage(cmd, staging.buffer, image.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, static_cast<uint32_t>(regions.size()),
                               regions.data());
        vk::ImageBarrier(cmd, image.image, VK_IMAGE_ASPECT_COLOR_BIT, VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT,
                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_SAMPLED_READ_BIT,
                         VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }, staging);
    ctx_->DestroyBuffer(staging);if(!submitted){ctx_->DestroyImage(image);return kWhite;}
    }
    fallback_bytes_+=converted_bytes;
    const uint32_t index = static_cast<uint32_t>(images_.size());
    const bool cube_slot = cube && cube_slots_.size() < kMaxCubeTextures;
    VkDescriptorImageInfo image_info{sampler_, image.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    if (cube && !images_.empty()) {
        image_info.imageView = images_[kWhite].view;
    }
    VkDescriptorImageInfo cube_info{base_sampler_, image.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    VkWriteDescriptorSet writes[2]{};
    writes[0] = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
    writes[0].dstSet = set_;
    writes[0].dstBinding = 0;
    writes[0].dstArrayElement = index;
    writes[0].descriptorCount = 1;
    writes[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writes[0].pImageInfo = &image_info;
    writes[1] = writes[0];
    writes[1].dstBinding = 2;
    writes[1].dstArrayElement = static_cast<uint32_t>(cube_slots_.size());
    writes[1].pImageInfo = &cube_info;
    vkUpdateDescriptorSets(ctx_->device, cube_slot ? 2 : 1, writes, 0, nullptr);
    if (cube_slot) {
        const uint32_t slot = static_cast<uint32_t>(cube_slots_.size());
        cube_slots_[index] = slot;
    }
    images_.push_back(image);
    cube_.push_back(cube && index > 0 ? 1 : 0);
    by_name_[name] = index;
    if(converted_bytes)converted_images_[index]={prepared->cache_file,converted_bytes,texture_epoch_,format,false};
    return index;
}

void TextureManager::WriteResidentDescriptor(uint32_t index) {
    if(index>=images_.size() || images_.size()<4)return;
    const VkImageView resident=images_[index].view;
    VkDescriptorImageInfo info{sampler_,cube_[index]?images_[kWhite].view:(resident?resident:images_[kWhite].view),VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    VkWriteDescriptorSet writes[2]{};writes[0]={VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
    writes[0].dstSet=set_;writes[0].dstBinding=0;writes[0].dstArrayElement=index;writes[0].descriptorCount=1;
    writes[0].descriptorType=VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;writes[0].pImageInfo=&info;
    VkDescriptorImageInfo cube_info{base_sampler_,resident?resident:images_[3].view,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    uint32_t count=1;
    if(auto it=cube_slots_.find(index);it!=cube_slots_.end()){writes[1]=writes[0];writes[1].dstBinding=2;writes[1].dstArrayElement=it->second;writes[1].pImageInfo=&cube_info;count=2;}
    vkUpdateDescriptorSets(ctx_->device,count,writes,0,nullptr);
}

void TextureManager::Touch(uint32_t texture) {
    if(converted_images_.empty())return;
    // Material redirection may change the texture sampled by a slot.
    texture_use_.insert(texture);
    if(auto it=redirects_.find(texture);it!=redirects_.end())texture_use_.insert(it->second);
    if(enhanced_enabled_)if(auto it=enhanced_images_.find(texture);it!=enhanced_images_.end())texture_use_.insert(it->second);
}
void TextureManager::TouchCube(uint32_t slot) {
    if(converted_images_.empty())return;
    for(const auto& [index,cube_slot]:cube_slots_)if(slot==cube_slot){Touch(index);break;}
}
void TextureManager::TouchMaterial(uint32_t index,bool alpha_only) {
    if(index>=materials_.size() || converted_images_.empty())return;
    const auto& m=materials_[index];Touch(m.albedo);
    if(alpha_only)return;
    Touch(m.normal);Touch(m.specular);Touch(m.aux1);Touch(m.aux2);
    if(m.kind==gpu::kKindGlass)TouchCube(m.aux0);else Touch(m.aux0);
    if(m.flags&gpu::kMatEye)Touch(std::bit_cast<uint32_t>(m.extra.x));
}

void TextureManager::EvictConverted(uint64_t needed) {
    if(fallback_bytes_+needed<=residency_limit_)return;
    std::vector<uint32_t> candidates;
    for(const auto& [index,entry]:converted_images_)
        if(images_[index].image && !texture_use_.contains(index) && !entry.file.empty())candidates.push_back(index);
    std::sort(candidates.begin(),candidates.end(),[&](uint32_t a,uint32_t b){return converted_images_.at(a).last_use<converted_images_.at(b).last_use;});
    if(candidates.empty())return;
    // A descriptor may still be read by the previous frame or by a queued
    // upload. Wait once for this reclamation batch, before changing any slot.
    FlushUploads();ctx_->RetireTextureUploads(true);vkDeviceWaitIdle(ctx_->device);
    for(uint32_t index:candidates){
        if(fallback_bytes_+needed<=residency_limit_)break;
        auto& entry=converted_images_.at(index);
        ctx_->DestroyImage(images_[index]);fallback_bytes_-=entry.bytes;WriteResidentDescriptor(index);
    }
}

void TextureManager::FinishTextureUse() {
    if(converted_images_.empty())return;
    ++texture_epoch_;
    if(texture_epoch_%300==0)LogInfo("texture residency: {} converted slots, {} of {} MiB live; compatibility size {}",converted_images_.size(),fallback_bytes_/1024/1024,residency_limit_/1024/1024,
#ifdef __ANDROID__
        android::PerformanceOptions().compat_texture_size
#else
        0
#endif
    );
    for(uint32_t index:texture_use_)if(auto it=converted_images_.find(index);it!=converted_images_.end())it->second.last_use=texture_epoch_;
    uint32_t restored=0;const auto started=std::chrono::steady_clock::now();
    for(uint32_t index:texture_use_){
        auto it=converted_images_.find(index);if(it==converted_images_.end() || images_[index].image || it->second.failed)continue;
        const ConvertedImage entry=it->second;
        EvictConverted(entry.bytes);
        if(fallback_bytes_+entry.bytes>residency_limit_){
            if(!residency_warning_){LogWarn("textures: active converted textures exceed {} MiB; select a smaller Compatibility texture size and restart",residency_limit_/1024/1024);residency_warning_=true;}
            continue;
        }
        auto payload=ReadBcCache(entry.file);
        if(!payload){it->second.failed=true;LogError("textures: conversion backing is unreadable for slot {}",index);continue;}
        std::vector<TextureMip> mips;for(uint32_t face=0;face<payload->layers;++face)for(uint32_t l=0;l<payload->levels;++l)mips.push_back({std::max(1u,payload->width>>l),std::max(1u,payload->height>>l),payload->mips[face*payload->levels+l]});
        const std::string temporary="builtin:residency-"+std::to_string(index);
        // Restore through the existing validated upload path. Its temporary
        // descriptor is never referenced by a material or submitted draw.
        restoring_=true;uint32_t uploaded=kWhite;
        try {uploaded=Create(temporary,entry.format,mips,payload->layers,cube_[index]!=0);}
        catch(...){restoring_=false;throw;}
        restoring_=false;
        if(uploaded==kWhite)continue;
        images_[index]=images_[uploaded];images_.pop_back();cube_.pop_back();cube_slots_.erase(uploaded);by_name_.erase(temporary);
        fallback_bytes_+=entry.bytes;WriteResidentDescriptor(index);++restored;
    }
    if(restored)LogDebug("textures: restored {} converted images, {} MiB live, {:.2f} ms",restored,fallback_bytes_/1024/1024,std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-started).count());
    // Keep the current set protected throughout the next frame's asset setup;
    // clear it lazily on the first Touch of the following rendering epoch.
}


uint32_t TextureManager::LoadFox(const QarArchive& qar, const std::string& path, bool* ok, bool raw) {
    const std::string stem = FtexStem(path);
    const std::string key = raw ? stem + "#raw" : stem;
    if (auto it = by_name_.find(key); it != by_name_.end()) {
        if (ok) {
            *ok = true;
        }
        return it->second;
    }
    if (mods::Active()) {
        if (const auto png = mods::ReadOverride(stem + ".png")) {
            const uint32_t index = LoadModImage(qar, key, stem, *png, raw);
            if (index != kWhite) {
                if (ok) {
                    *ok = true;
                }
                return index;
            }
        }
    }
    if (failed_textures_.contains(key)) {if(ok)*ok=false;return kWhite;}
    FtexTexture owned;
    std::shared_ptr<const FtexTexture> prepared;
    bool read = false;
    const bool scheduled=prepared_.contains(stem)||pending_preparations_.contains(stem)||decoding_.contains(stem);
    if (auto it = prepared_.find(stem); it != prepared_.end()) {
        prepared = it->second;
        read = prepared != nullptr;
        ++read_stats_.prepared;
    } else if (auto it = pending_preparations_.find(stem); it != pending_preparations_.end()) {
        it->second.prioritize(stem);
        const bool ready=it->second.future.wait_for(std::chrono::seconds(0))==std::future_status::ready;
        const auto started=std::chrono::steady_clock::now();
        auto result=it->second.future.get();
        read_stats_.stream_wait_ms+=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-started).count();
        read_stats_.stream_ready+=ready;
        prepared=std::move(result.texture); pending_preparations_.erase(it); read=prepared!=nullptr;
        if(read){prepared_.emplace(stem,prepared);++read_stats_.prepared;}
        else if(result.failure==TexturePreparationFailure::Budget)++read_stats_.budget_fallback;
        else ++read_stats_.read_fallback;
    } else if (auto it = decoding_.find(stem); it != decoding_.end()) {
        prepared = it->second.get();
        decoding_.erase(it);
        read = prepared != nullptr;
        if (read) {
            prepared_.emplace(stem, prepared);
            worker_prepared_.insert(stem);
            ++read_stats_.ahead;
        }
    }
    if (!read) {
        const auto started = std::chrono::steady_clock::now();
        ++read_stats_.direct;
        read_stats_.unprepared+=!scheduled;
        read = LoadFtex(qar, stem, owned);
        read_ms_ += std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - started).count();
    }
    const FtexTexture& ftex = prepared ? *prepared : owned;
    if(!read){const bool header_exists=qar.Contains(PathCode64(stem+".ftex"));failed_textures_[key]=header_exists?"invalid header or missing texture streams":"missing FTEX header";LogWarn("textures: {} {}",key,failed_textures_[key]);}
    if (!read || ftex.Format() == VK_FORMAT_UNDEFINED || ftex.depth > 1) {
        if (ok) {
            *ok = false;
        }
        return kWhite;
    }
    uint32_t first_level = 0;
    while (first_level < ftex.mip_count && ftex.mips[first_level].empty()) {
        ++first_level;
    }
    std::vector<TextureMip> mips;
    for (uint32_t face = 0; face < ftex.faces; ++face) {
        for (uint32_t level = first_level; level < ftex.mip_count; ++level) {
            const auto& data = ftex.mips[face * ftex.mip_count + level];
            mips.push_back({ftex.MipWidth(level), ftex.MipHeight(level), data});
        }
    }
    bool compressed = false;
    FormatBlockBytes(ftex.Format(), compressed);
    if (compressed) {
        for (TextureMip& mip : mips) {
            mip.width = std::max(mip.width, 1u);
            mip.height = std::max(mip.height, 1u);
        }
    }
    const auto upload_started = std::chrono::steady_clock::now();
    const double upload_before=upload_ms_;
    const uint32_t index = Create(key, ftex.Format(raw), mips, ftex.faces, ftex.faces == 6, ftex.compatible);
    if (auto it = prepared_variants_.find(stem); it != prepared_variants_.end()) {
        const uint8_t resident = uint8_t(by_name_.contains(stem)) | (uint8_t(by_name_.contains(stem + "#raw")) << 1);
        if ((resident & it->second) == it->second) {
            prepared_.erase(stem);
            prepared_variants_.erase(it);
        }
    }
    // A worker result can serve either upload order without rereading or changing
    // flags. Reclaim its budget after both variants are resident.
    if (worker_prepared_.contains(stem) && by_name_.contains(stem) && by_name_.contains(stem + "#raw")) {
        worker_prepared_.erase(stem);
        prepared_.erase(stem);
    }
    upload_ms_ = upload_before+std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - upload_started).count();
    if (index != kWhite && !raw && EnhancedTextureEligible(stem, ftex)) {
        fox_sources_[index] = stem;
        if (enhanced_enabled_) LoadEnhancedTexture(index, stem, &ftex);
    }
    if (ok) {
        *ok = index != kWhite;
    }
    return index;
}

void TextureManager::DecodeAhead(const QarArchive& qar, const std::vector<std::string>& paths) {
    if (mods::Active() || paths.empty()) return;
    const bool new_reads = std::any_of(paths.begin(), paths.end(), [&](const std::string& path) {
        const std::string stem = FtexStem(path);
        return !stem.empty() && !by_name_.contains(stem) && !prepared_.contains(stem) && !decoding_.contains(stem);
    });
    if (!new_reads) return;
    // Uploaded payloads may serve raw/sRGB within a preparation set. A new
    // set releases that cache so old CPU copies cannot starve the bounded pool.
    for (const auto& stem : worker_prepared_) prepared_.erase(stem);
    worker_prepared_.clear();
    if (!decode_queue_) {
        decode_queue_ = std::make_shared<DecodeQueue>();
        for (int i = 0; i < 2; ++i) {
            decode_workers_.push_back(std::async(std::launch::async, [queue = decode_queue_] {
                for (;;) {
                    DecodeJob job;
                    {
                        std::unique_lock lock(queue->mutex);
                        queue->ready.wait(lock, [&] { return queue->stop || !queue->jobs.empty(); });
                        if (queue->stop) return;
                        job = std::move(queue->jobs.front());
                        queue->jobs.pop_front();
                    }
                    // The lease lasts exactly as long as the immutable payload.
                    auto lease = std::make_shared<TextureReadBudget>(96ull * 1024 * 1024, queue->budget);
                    auto ftex = std::shared_ptr<FtexTexture>(new FtexTexture,
                        [budget = queue->budget, lease](FtexTexture* value) { delete value; });
                    if (!LoadFtex(*job.qar, job.stem, *ftex, lease.get())) ftex.reset();
                    job.promise.set_value(std::move(ftex));
                }
            }));
        }
    }
    {
        std::lock_guard lock(decode_queue_->mutex);
        for (const std::string& path : paths) {
            std::string stem = FtexStem(path);
            if (stem.empty() || by_name_.contains(stem) || prepared_.contains(stem) || decoding_.contains(stem)) continue;
            DecodeJob job;
            job.qar = &qar;
            job.stem = stem;
            decoding_.emplace(stem, job.promise.get_future().share());
            decode_order_.push_back(stem);
            decode_queue_->jobs.push_back(std::move(job));
        }
    }
    decode_queue_->ready.notify_all();
}

bool TextureManager::AdoptPending(const std::string& path, std::shared_future<PreparedTextureResult> future,
                                 std::function<void(const std::string&)> prioritize) {
    const std::string stem=FtexStem(path);
    if (mods::Active() || prepared_.contains(stem) || pending_preparations_.contains(stem)) return false;
    return pending_preparations_.emplace(stem,PendingPreparation{std::move(future),std::move(prioritize)}).second;
}

bool TextureManager::AdoptDecoded(const std::string& path, std::shared_ptr<FtexTexture> decoded) {
    std::string stem = FtexStem(path);
    if (mods::Active() || stem.empty() || !decoded || prepared_.contains(stem) || decoding_.contains(stem)) {
        return false;
    }
    prepared_.emplace(std::move(stem), std::move(decoded));
    return true;
}

void TextureManager::DropDecoded(const std::vector<std::string>& paths) {
    for (const std::string& path : paths) {
        prepared_.erase(FtexStem(path));
        pending_preparations_.erase(FtexStem(path));
        prepared_variants_.erase(FtexStem(path));
    }
}

std::unordered_set<std::string> TextureManager::Names() const {
    std::unordered_set<std::string> names;
    for (const auto& [name, index] : by_name_) {
        names.insert(name);
    }
    return names;
}

uint32_t TextureManager::PumpDecoded(const QarArchive& qar, uint32_t count) {
    uint32_t uploaded = 0;
    while (uploaded < count && !decode_order_.empty()) {
        auto it = decoding_.find(decode_order_.front());
        if (it == decoding_.end()) {
            decode_order_.pop_front();
            continue;
        }
        if (it->second.wait_for(std::chrono::seconds(0)) != std::future_status::ready) {
            break;
        }
        const std::string stem = decode_order_.front();
        decode_order_.pop_front();
        if (!it->second.get()) { decoding_.erase(it); continue; }
        LoadFox(qar, stem);
        ++uploaded;
    }
    return uploaded;
}

uint32_t TextureManager::LoadModImage(const QarArchive& qar, const std::string& key, const std::string& stem, const std::vector<uint8_t>& png,
                                      bool raw) {
    // the game's texture, when there is one, says whether the picture is colour (sRGB) and whether a PNG can stand in for it
    FtexTexture original;
    const bool has_original = LoadFtex(qar, stem, original);
    if (has_original && (original.faces != 1 || original.depth > 1)) {
        LogWarn("mods: {}.png ignored: a cube or volume texture is replaced by a .ftex only", stem);
        return kWhite;
    }
    const bool srgb = !raw && (!has_original || original.Srgb());
    std::vector<std::vector<uint8_t>> levels;
    uint32_t width = 0, height = 0;
    std::string error;
    if (!ModImageLevels(png, srgb, levels, width, height, error)) {
        LogWarn("mods: {}.png ignored: {}", stem, error);
        return kWhite;
    }
    std::vector<TextureMip> mips;
    for (uint32_t level = 0; level < levels.size(); ++level) {
        mips.push_back({std::max(1u, width >> level), std::max(1u, height >> level), levels[level]});
    }
    const uint32_t index = Create(key, srgb ? VK_FORMAT_R8G8B8A8_SRGB : VK_FORMAT_R8G8B8A8_UNORM, mips);
    if (index != kWhite) {
        LogInfo("mods: texture {} from a {} x {} PNG", stem, width, height);
    }
    return index;
}

void TextureManager::ConfigureEnhancedTextures(const QarArchive& qar, const std::filesystem::path& cache, uint64_t model) {
    if(enhanced_qar_ == &qar && enhanced_cache_ == cache && enhanced_model_ == model) return;
    SetEnhancedTextures(false);
    enhanced_qar_ = &qar;
    enhanced_cache_ = cache;
    enhanced_model_ = model;
    enhanced_images_.clear();
    enhanced_bytes_ = 0;
}

void TextureManager::LoadEnhancedTexture(uint32_t index, const std::string& path, const FtexTexture* source) {
    if (!enhanced_qar_ || !enhanced_model_ || enhanced_images_.contains(index)) return;
    FtexTexture loaded, cached;
    if (!source) {
        if (!LoadFtex(*enhanced_qar_, path, loaded)) return;
        source = &loaded;
    }
    const uint64_t key = EnhancedTextureKey(*source, enhanced_model_);
    if (!ReadTextureCache(EnhancedCacheFile(enhanced_cache_, path), key, cached)) return;
    // twice the source's size, or the source's size where the capped mode reduced the 2x result (a 2048 source capped at 2048)
    const bool doubled = cached.width == source->width * 2 && cached.height == source->height * 2;
    const bool kept = cached.width == source->width && cached.height == source->height;
    if (!(doubled || kept) || cached.Srgb() != source->Srgb()) return;
    std::vector<TextureMip> mips;
    uint64_t bytes = 0;
    for (uint32_t level = 0; level < cached.mip_count; ++level) {
        mips.push_back({cached.MipWidth(level), cached.MipHeight(level), cached.mips[level]});
        bytes += cached.mips[level].size();
    }
    const uint32_t replacement = Create(path + std::format("#enhanced-{:016x}", key), cached.Format(), mips);
    if (replacement == kWhite) return;
    enhanced_images_[index] = replacement;
    enhanced_bytes_ += bytes;
    if (enhanced_enabled_) {
        vkDeviceWaitIdle(ctx_->device);
        UpdateTextureDescriptor(index);
    }
}

void TextureManager::UpdateTextureDescriptor(uint32_t index) {
    uint32_t shown = index;
    if (enhanced_enabled_) {
        if (auto it = enhanced_images_.find(index); it != enhanced_images_.end()) shown = it->second;
    }
    VkDescriptorImageInfo info{sampler_, images_[shown].view ? images_[shown].view : images_[kWhite].view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
    write.dstSet = set_; write.dstBinding = 0; write.dstArrayElement = index;
    write.descriptorCount = 1; write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER; write.pImageInfo = &info;
    vkUpdateDescriptorSets(ctx_->device, 1, &write, 0, nullptr);
}

void TextureManager::StopEnhancedWorker() {
    if (enhanced_queue_) enhanced_queue_->stop = true;
    if (enhanced_worker_.valid()) enhanced_worker_.wait();
    enhanced_worker_ = {};
    enhanced_queue_.reset();
}

void TextureManager::SetEnhancedTextures(bool enabled) {
    if (!ctx_ || enabled == enhanced_enabled_) return;
    StopEnhancedWorker();
    if (enabled && enhanced_qar_ && enhanced_model_) {
        std::vector<std::pair<uint32_t, std::string>> todo;
        for (const auto& [index, path] : fox_sources_) {
            if (!enhanced_images_.contains(index)) todo.emplace_back(index, path);
        }
        auto queue = std::make_shared<EnhancedQueue>();
        queue->total = todo.size();
        enhanced_queue_ = queue;
        const QarArchive* qar = enhanced_qar_;
        const std::filesystem::path cache = enhanced_cache_;
        const uint64_t model = enhanced_model_;
        enhanced_worker_ = std::async(std::launch::async, [queue, todo = std::move(todo), qar, cache, model] {
            for (const auto& [index, path] : todo) {
                if (queue->stop) break;
                FtexTexture source;
                EnhancedPrepared prepared;
                if (LoadFtex(*qar, path, source)) {
                    prepared.key = EnhancedTextureKey(source, model);
                    if (ReadTextureCache(EnhancedCacheFile(cache, path), prepared.key, prepared.cached)) {
                        const FtexTexture& c = prepared.cached;
                        const bool doubled = c.width == source.width * 2 && c.height == source.height * 2;
                        const bool kept = c.width == source.width && c.height == source.height;
                        if ((doubled || kept) && c.Srgb() == source.Srgb()) {
                            prepared.index = index;
                            prepared.path = path;
                            std::lock_guard lock(queue->mutex);
                            queue->ready.push_back(std::move(prepared));
                        }
                    }
                }
                ++queue->done;
            }
        });
    }
    vkDeviceWaitIdle(ctx_->device);
    enhanced_enabled_ = enabled;
    for (const auto& [index, replacement] : enhanced_images_) UpdateTextureDescriptor(index);
    LogInfo("textures: enhanced textures {}, {} loaded replacements ({:.0f} MB of texture data)", enabled ? "on" : "off", enhanced_images_.size(),
            static_cast<double>(enhanced_bytes_) / (1024.0 * 1024.0));
}

void TextureManager::PumpEnhancedTextures(double budget_ms) {
    if (!enhanced_queue_ || !ctx_) return;
    const auto start = std::chrono::steady_clock::now();
    std::vector<uint32_t> swapped;
    for (;;) {
        EnhancedPrepared prepared;
        {
            std::lock_guard lock(enhanced_queue_->mutex);
            if (enhanced_queue_->ready.empty()) break;
            prepared = std::move(enhanced_queue_->ready.front());
            enhanced_queue_->ready.pop_front();
        }
        if (!enhanced_images_.contains(prepared.index)) {
            std::vector<TextureMip> mips;
            uint64_t bytes = 0;
            for (uint32_t level = 0; level < prepared.cached.mip_count; ++level) {
                mips.push_back({prepared.cached.MipWidth(level), prepared.cached.MipHeight(level), prepared.cached.mips[level]});
                bytes += prepared.cached.mips[level].size();
            }
            const uint32_t replacement = Create(prepared.path + std::format("#enhanced-{:016x}", prepared.key), prepared.cached.Format(), mips);
            if (replacement != kWhite) {
                enhanced_images_[prepared.index] = replacement;
                enhanced_bytes_ += bytes;
                swapped.push_back(prepared.index);
            }
        }
        if (std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count() >= budget_ms) break;
    }
    if (!swapped.empty() && enhanced_enabled_) {
        vkDeviceWaitIdle(ctx_->device);
        for (const uint32_t index : swapped) UpdateTextureDescriptor(index);
    }
    bool finished = false;
    {
        std::lock_guard lock(enhanced_queue_->mutex);
        finished = enhanced_queue_->ready.empty() && enhanced_queue_->done.load() >= enhanced_queue_->total;
    }
    if (finished) {
        StopEnhancedWorker();
        LogInfo("textures: enhanced textures ready, {} replacements ({:.0f} MB of texture data)", enhanced_images_.size(),
                static_cast<double>(enhanced_bytes_) / (1024.0 * 1024.0));
    }
}

uint32_t TextureManager::AddMaterial(const MaterialGpu& material) {
    materials_.push_back(material);
    materials_dirty_ = true;
    return static_cast<uint32_t>(materials_.size() - 1);
}

void TextureManager::UpdateMaterial(uint32_t index, const MaterialGpu& material) {
    if (index < materials_.size()) {
        materials_[index] = material;
        materials_dirty_ = true;
    }
}

void TextureManager::RedirectTexture(uint32_t texture, uint32_t shown) {
    if (shown == texture) {
        materials_dirty_ = redirects_.erase(texture) > 0 || materials_dirty_;
        return;
    }
    redirects_[texture] = shown;
    materials_dirty_ = true;
}

std::array<uint64_t,3> TextureManager::ShadowAlphaState(uint32_t index) const {
    if(index>=materials_.size())return {};
    const auto& m=materials_[index];if(!(m.flags & gpu::kMatAlphaTest))return {m.flags,0,0};
    uint32_t albedo=m.albedo;if(auto it=redirects_.find(albedo);it!=redirects_.end())albedo=it->second;
    uint32_t shown=albedo;if(enhanced_enabled_)if(auto it=enhanced_images_.find(albedo);it!=enhanced_images_.end())shown=it->second;
    return {uint64_t(m.flags)|(uint64_t(anisotropy_)<<32),albedo,shown};
}

void TextureManager::FlushMaterials() {
    FlushUploads();
    if (!materials_dirty_) {
        return;
    }
    const size_t count = std::min<size_t>(materials_.size(), kMaxMaterials);
    if (redirects_.empty()) {
        std::memcpy(material_buffer_.mapped, materials_.data(), count * sizeof(MaterialGpu));
    } else {
        auto* out = static_cast<MaterialGpu*>(material_buffer_.mapped);
        for (size_t i = 0; i < count; ++i) {
            out[i] = materials_[i];
            for (uint32_t* slot : {&out[i].albedo, &out[i].normal, &out[i].specular}) {
                if (auto it = redirects_.find(*slot); it != redirects_.end()) {
                    *slot = it->second;
                }
            }
        }
    }
    vmaFlushAllocation(ctx_->allocator, material_buffer_.allocation, 0, count * sizeof(MaterialGpu));
    materials_dirty_ = false;
}

}
