#include "engine/platform/android.h"
#ifdef __ANDROID__
#include <SDL3/SDL_system.h>
#include <jni.h>
#include <atomic>
#include <fstream>
#include <cstdlib>
#include <mutex>
#include <string>
namespace pt::android {
namespace {
std::mutex options_mutex;
MobilePerformanceOptions options;
std::once_flag first_load;
std::atomic<int> thermal_status{0};
void Reload() {
    MobilePerformanceOptions next;
    const char* root=SDL_GetAndroidInternalStoragePath();
    std::ifstream file(std::filesystem::path(root?root:".")/"mobile_performance.ini");
    std::string line;
    while(std::getline(file,line)) {
        auto at=line.find('=');if(at==std::string::npos)continue;
        const auto key=line.substr(0,at);char* end=nullptr;const auto text=line.substr(at+1);
        float value=std::strtof(text.c_str(),&end);
        if(end==text.c_str()||end!=text.c_str()+text.size()||!std::isfinite(value))continue;
        if(key=="frame_pacing")next.frame_pacing=value>0;
        else if(key=="mali_safe_lighting")next.mali_safe_lighting=value>0;
        else if(key=="mali_optimizations")next.mali_optimizations=value>0;
        else if(key=="sgsr")next.sgsr=value>0;
        else if(key=="sustained")next.sustained=value>0;
        else if(key=="invert_y")next.invert_y=value>0;
        else if(key=="sharpness")next.sharpness=std::clamp(value,0.f,2.f);
        else if(key=="mouse_sensitivity")next.mouse_sensitivity=std::clamp(value,.25f,3.f);
        else if(key=="controller_sensitivity")next.controller_sensitivity=std::clamp(value,.25f,3.f);
        else if(key=="compat_texture_size")next.compat_texture_size=value==512||value==1024||value==2048?int(value):0;
        else if(key=="ao_quality")next.ao_quality=value>0?1:0;
        else if(key=="reflection_quality")next.reflection_quality=value>0?1:0;
    }
    {std::lock_guard lock(options_mutex);options=next;}
}
}
MobilePerformanceOptions PerformanceOptions(){std::call_once(first_load,Reload);std::lock_guard lock(options_mutex);return options;}
int ThermalStatus(){return thermal_status.load();}
int EffectiveFrameLimit(int requested){
    auto o=PerformanceOptions();const int result=ThermalFrameLimit(requested,thermal_status.load(),o.sustained);
    return result;
}
}
extern "C" JNIEXPORT void JNICALL Java_com_ptport_app_PTActivity_nativeReloadPerformance(JNIEnv*,jclass){pt::android::Reload();}
extern "C" JNIEXPORT void JNICALL Java_com_ptport_app_PTActivity_nativeThermalStatus(JNIEnv*,jclass,jint value){pt::android::thermal_status=std::clamp(static_cast<int>(value),0,6);}
#endif
