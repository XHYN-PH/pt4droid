#include "engine/platform/android.h"
#ifdef __ANDROID__
#include <SDL3/SDL.h>
#include <SDL3/SDL_system.h>
#include <jni.h>
#include <atomic>
#include <cmath>
#include <mutex>
#include <fstream>
#include <cstdlib>
#include <algorithm>
namespace pt::android {
namespace {
std::atomic<float> scale{0.667f};
std::atomic<bool> scale_changed{false};
std::atomic<int> profile{-1}, bloom{6};
std::once_flag loaded;
std::mutex look_mutex;
float look_x=0.0f,look_y=0.0f;
std::filesystem::path PrivateFile(const char* name) {
    const char* root=SDL_GetAndroidInternalStoragePath();
    return std::filesystem::path(root?root:".") / name;
}
float ReadNumber(const std::filesystem::path& path,float fallback) {
    std::ifstream in(path);std::string text;
    if(!(in>>text)) return fallback;
    char* end=nullptr;float value=std::strtof(text.c_str(),&end);
    return end==text.c_str()+text.size() && std::isfinite(value) ? value : fallback;
}
void Load() {
    std::call_once(loaded,[]{
        scale=std::clamp(ReadNumber(PrivateFile("render_scale.txt"),ReadNumber(LogDir()/"render_scale.txt",0.667f)),0.25f,1.0f);
        profile=std::clamp(static_cast<int>(ReadNumber(PrivateFile("performance_profile.txt"),0)),0,2);
        bloom=ReadNumber(PrivateFile("bloom_pass_limit.txt"),6)>0?6:0;
    });
}
template<class Call> void Activity(const char* method,const char* signature,Call call) {
    auto* env=static_cast<JNIEnv*>(SDL_GetAndroidJNIEnv());
    auto activity=static_cast<jobject>(SDL_GetAndroidActivity());
    if(!env || !activity)return;
    jclass cls=env->GetObjectClass(activity);
    jmethodID id=env->GetStaticMethodID(cls,method,signature);
    if(id)call(env,cls,id);
    if(env->ExceptionCheck())env->ExceptionClear();
    env->DeleteLocalRef(cls);env->DeleteLocalRef(activity);
}
}
float RenderScale(){Load();return scale.load();}
void SetRenderScale(float value){Load();if(!std::isfinite(value))return;value=std::clamp(value,0.25f,1.0f);if(std::abs(scale.exchange(value)-value)>0.0001f)scale_changed=true;}
bool ConsumeRenderScaleChange(){Load();return scale_changed.exchange(false);}
int ConsumePerformanceProfile(){Load();return profile.exchange(-1);}
int BloomPassLimit(){Load();return bloom.load();}
void ConsumeLookDelta(float& dx,float& dy){std::lock_guard lock(look_mutex);dx=look_x;dy=look_y;look_x=look_y=0.0f;}
void SetTouchControlsMode(int mode){Activity("setTouchControlsMode","(I)V",[&](JNIEnv* e,jclass c,jmethodID m){e->CallStaticVoidMethod(c,m,std::clamp(mode,0,2));});}
int TouchControlsMode(int fallback){int value=fallback;Activity("getTouchControlsMode","()I",[&](JNIEnv* e,jclass c,jmethodID m){value=e->CallStaticIntMethod(c,m);});return std::clamp(value,0,2);}
void OpenMobileSettings(){Activity("openMobileSettings","()V",[](JNIEnv* e,jclass c,jmethodID m){e->CallStaticVoidMethod(c,m);});}
}
extern "C" JNIEXPORT jfloat JNICALL Java_com_ptport_app_PTActivity_nativeGetRenderScale(JNIEnv*,jclass){return pt::android::RenderScale();}
extern "C" JNIEXPORT void JNICALL Java_com_ptport_app_PTActivity_nativeSetRenderScale(JNIEnv*,jclass,jfloat value){pt::android::SetRenderScale(value);}
extern "C" JNIEXPORT void JNICALL Java_com_ptport_app_PTActivity_nativeSetPerformanceProfile(JNIEnv*,jclass,jint value){pt::android::Load();pt::android::profile=std::clamp(value,0,2);}
extern "C" JNIEXPORT void JNICALL Java_com_ptport_app_PTActivity_nativeSetBloomPassLimit(JNIEnv*,jclass,jint value){pt::android::Load();pt::android::bloom=value>0?6:0;}
extern "C" JNIEXPORT void JNICALL Java_com_ptport_app_TouchControls_nativeLookDelta(JNIEnv*,jclass,jfloat dx,jfloat dy){
    if(!std::isfinite(dx)||!std::isfinite(dy))return;
    std::lock_guard lock(pt::android::look_mutex);pt::android::look_x+=dx;pt::android::look_y+=dy;
}
extern "C" JNIEXPORT void JNICALL Java_com_ptport_app_TouchControls_nativeClearLook(JNIEnv*,jclass){std::lock_guard lock(pt::android::look_mutex);pt::android::look_x=pt::android::look_y=0.0f;}
#endif
