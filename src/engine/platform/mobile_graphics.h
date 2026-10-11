#pragma once
#include "engine/platform/settings.h"

namespace pt::android {
// Merge only menu-owned graphics/display fields, preserving audio, input, saves and progress.
inline bool ApplyGameGraphicsOverrides(const std::filesystem::path& path, AppSettings& settings, bool ray_queries) {
    AppSettings requested = settings;
    if (!LoadAppSettings(path, requested)) return false;
    settings.graphics = requested.graphics;
    settings.graphics.enhanced_textures = false;
    settings.ray_tracing = ray_queries ? requested.ray_tracing : AppSettings::RayTracing{};
    settings.display.vsync = requested.display.vsync;
    settings.display.fps_limit = requested.display.fps_limit;
    settings.display.letterbox = requested.display.letterbox;
    settings.display.hdr = false;
    return true;
}
}
