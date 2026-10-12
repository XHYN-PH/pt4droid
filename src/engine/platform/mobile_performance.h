#pragma once
#include <algorithm>
#include <cmath>

namespace pt {
inline int ThermalFrameLimit(int requested,int status,bool enabled) {
    if(!enabled||status<2)return requested;
    const int ceiling=status>=4?20:status>=3?30:45;
    return requested>0?std::min(requested,ceiling):ceiling;
}
struct MobilePerformanceOptions {
    bool frame_pacing=false,sgsr=false,sustained=false,invert_y=false;
    float sharpness=1.f,mouse_sensitivity=1.f,controller_sensitivity=1.f;
    int ao_quality=0,reflection_quality=0;
};
struct MobileLookDelta {float x,y;};
inline float MobileControllerSensitivity(float baseline,const MobilePerformanceOptions& options,bool virtual_input){
    return virtual_input?baseline:baseline*options.controller_sensitivity;
}
inline MobileLookDelta CombineMobileLook(float mx,float my,float tx,float ty,float gx,float gy,
                                        float mouse_scale,float touch_scale,const MobilePerformanceOptions& options){
    MobileLookDelta result{mx*options.mouse_sensitivity*mouse_scale+tx*touch_scale+gx,
                           my*options.mouse_sensitivity*mouse_scale+ty*touch_scale+gy};
    if(options.invert_y)result.y=-result.y;
    return result;
}
}
