#pragma once
#include <cstdint>
namespace pt {
inline bool ProbeMapWorthBuilding(uint64_t covered_pixels, uint32_t probes) {
    // The direction map shades a padded row of 32 probes, 16x16 texels each.
    // Small/occluded views use direct SH evaluation instead of paying its
    // fixed render pass and texture traffic. The probe quality is preserved.
    const uint64_t map_pixels = 512ull * 16 * ((probes + 31ull) / 32);
    return probes > 0 && covered_pixels >= map_pixels * 4;
}
}
