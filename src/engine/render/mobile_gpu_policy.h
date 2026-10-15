#pragma once
#include <cstdint>
namespace pt {
// ARM's Vulkan vendor ID is also reported by the Mali system/PanVK drivers.
// A Turnip/Qualcomm device never selects these shader or atlas alternatives.
inline bool UseMaliLighting(uint32_t vendor, bool enabled) { return enabled && vendor == 0x13B5u; }
}
