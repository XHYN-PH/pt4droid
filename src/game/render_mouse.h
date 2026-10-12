#pragma once
#include <algorithm>
#include "engine/render/camera.h"
#include "game/player.h"
namespace pt::game {
inline Camera RenderDeltaLook(Camera rendered, glm::vec2 consumed, glm::vec2 pending, float blend,
                              bool invert_x, bool invert_y) {
    // Unblend only the direct mouse/touch/gyro delta consumed by the last tick.
    // Gamepad rotation stays interpolated, even when it is used together with swipe or gyro.
    const glm::vec2 delta = pending + consumed * (1.0f - std::clamp(blend, 0.0f, 1.0f));
    rendered.yaw -= delta.x * (invert_x ? -1.0f : 1.0f);
    if (delta.y != 0.0f) rendered.pitch = Player::ClampPitch(rendered.pitch - delta.y * (invert_y ? -1.0f : 1.0f));
    return rendered;
}
inline Camera RenderMouseLook(Camera rendered, const Camera& current, glm::vec2 pending, bool invert_x, bool invert_y) {
    // Mouse deltas are already integrated in current, unlike smoothed pad look.
    // Blending these angles again makes the camera rewind whenever a tick consumes pending input.
    rendered.yaw = current.yaw - pending.x * (invert_x ? -1.0f : 1.0f);
    rendered.pitch = Player::ClampPitch(current.pitch - pending.y * (invert_y ? -1.0f : 1.0f));
    return rendered;
}
}
