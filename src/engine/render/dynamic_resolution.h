#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
namespace pt {
inline float DynamicFrameBudget(int requested,float refresh) {
    const float fps=requested>0?float(requested):(std::isfinite(refresh)&&refresh>0?refresh:60.f);
    const float interval=1000.f/std::clamp(fps,10.f,240.f);
    return std::max(1.f,interval-std::max(.5f,interval*.08f));
}
inline float DynamicFloorScale(float configured,float chosen) {
    return std::min(std::clamp(configured,.5f,1.f),std::clamp(chosen,.25f,1.f));
}
class DynamicResolution {
    uint64_t sample_=0;float filtered_=0;unsigned hot_=0,cool_=0,cooldown_=0;bool reduced_=false;
public:
    bool Observe(uint64_t sample,float ms,float budget,bool enabled){
        if(!enabled){reduced_=false;hot_=cool_=cooldown_=0;return false;}
        if(!sample||sample==sample_||!std::isfinite(ms)||ms<=0)return reduced_;
        sample_=sample;filtered_=filtered_?filtered_*.9f+ms*.1f:ms;
        if(cooldown_){--cooldown_;return reduced_;}
        hot_=filtered_>budget*1.04f?hot_+1:0;cool_=filtered_<budget*.72f?cool_+1:0;
        if(!reduced_&&hot_>=8){reduced_=true;hot_=cool_=0;cooldown_=90;}
        else if(reduced_&&cool_>=120){reduced_=false;hot_=cool_=0;cooldown_=120;}
        return reduced_;
    }
    void Reset(){*this={};}
};
}
