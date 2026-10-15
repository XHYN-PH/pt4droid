#pragma once
#include "engine/assets/ftex.h"
#include <array>
#include <filesystem>
#include <memory>
#include <span>

namespace pt {
struct BcMipChain {
    VkFormat format = VK_FORMAT_UNDEFINED;
    uint32_t width = 0, height = 0, levels = 0, layers = 1;
    std::vector<std::vector<uint8_t>> mips;
    std::filesystem::path cache_file;
    uint64_t bytes = 0;
};
struct BcSourceMip { uint32_t width, height; std::span<const uint8_t> bytes; };
// Installed once, before loading starts. Workers use an immutable snapshot;
// the supported/native path does not hash, convert, or access the disk cache.
void ConfigureBcConversion(std::array<bool, 16> native, std::filesystem::path cache, uint32_t max_side);
void PrepareBcTexture(FtexTexture& texture, TextureReadBudget* budget);
std::shared_ptr<const BcMipChain> PrepareBcMips(VkFormat format, std::span<const BcSourceMip> mips,
                                              uint32_t layers, TextureReadBudget* budget = nullptr);
std::shared_ptr<const BcMipChain> ReadBcCache(const std::filesystem::path& file);
}
