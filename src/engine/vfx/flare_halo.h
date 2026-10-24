#pragma once
#include <algorithm>
#include <cmath>
namespace pt::vfx {
inline float FlareHaloAttenuation(float span,float strength) {
    if(!std::isfinite(span)||!std::isfinite(strength))return 1.f;
    float t=std::clamp((std::abs(span)-.12f)/.48f,0.f,1.f);t=t*t*(3.f-2.f*t);
    return 1.f-(1.f-std::clamp(strength,0.f,1.f))*t;
}
}
