#pragma once
#include <volk.h>
#include <span>
#include <vector>
#include <cstdint>
namespace pt { VkFormat BcFallbackFormat(VkFormat format); bool DecodeBcMip(VkFormat format,uint32_t width,uint32_t height,std::span<const uint8_t> data,std::vector<uint8_t>& out); }
