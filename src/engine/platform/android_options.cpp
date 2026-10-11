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
#include "engine/core/log.h"
namespace pt::android {
namespace {
std::atomic<float> scale{0.667f};
std::atomic<bool> scale_changed{false};
std::atomic<bool> graphics_changed{false};
std::mutex graphics_mutex;
std::string graphics_snapshot;
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
        // Presets are one-shot actions. pt.ini keeps the result; do not overwrite later game-menu changes on restart.
        profile=static_cast<int>(std::clamp(ReadNumber(PrivateFile("performance_profile_pending.txt"),-1),-1.f,3.f));
        bloom=ReadNumber(PrivateFile("bloom_pass_limit.txt"),6)>0?6:0;
        std::error_code ec;
        graphics_changed=std::filesystem::is_regular_file(PrivateFile("game_graphics_pending.ini"),ec) ||
            std::filesystem::is_regular_file(PrivateFile("game_graphics_applying.ini"),ec);
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
int ConsumePerformanceProfile(){Load();const int value=profile.exchange(-1);if(value>=0){std::error_code ec;std::filesystem::remove(PrivateFile("performance_profile_pending.txt"),ec);}return value;}
int BloomPassLimit(){Load();return bloom.load();}
void PublishGameGraphicsSettings(std::string text){std::lock_guard lock(graphics_mutex);graphics_snapshot=std::move(text);}
bool ConsumeGameGraphicsChanges(){
    Load();if(!graphics_changed.exchange(false))return false;
    std::error_code ec;
    const auto pending=PrivateFile("game_graphics_pending.ini"),applying=PrivateFile("game_graphics_applying.ini");
    if(std::filesystem::is_regular_file(pending,ec)){std::filesystem::rename(pending,applying,ec);if(ec)return false;}
    ec.clear();
    return std::filesystem::is_regular_file(applying,ec);
}
std::filesystem::path GameGraphicsChangesFile(){return PrivateFile("game_graphics_applying.ini");}
void ConsumeLookDelta(float& dx,float& dy){std::lock_guard lock(look_mutex);dx=look_x;dy=look_y;look_x=look_y=0.0f;}
void SetTouchControlsMode(int mode){Activity("setTouchControlsMode","(I)V",[&](JNIEnv* e,jclass c,jmethodID m){e->CallStaticVoidMethod(c,m,std::clamp(mode,0,2));});}
int TouchControlsMode(int fallback){int value=fallback;Activity("getTouchControlsMode","()I",[&](JNIEnv* e,jclass c,jmethodID m){value=e->CallStaticIntMethod(c,m);});return std::clamp(value,0,2);}
void OpenMobileSettings(){Activity("openMobileSettings","()V",[](JNIEnv* e,jclass c,jmethodID m){e->CallStaticVoidMethod(c,m);});}
void GameGraphicsPresetSelected(int preset){profile=-1;std::error_code ec;std::filesystem::remove(PrivateFile("performance_profile_pending.txt"),ec);Activity("gameGraphicsPresetSelected","(I)V",[&](JNIEnv* e,jclass c,jmethodID m){e->CallStaticVoidMethod(c,m,preset);});}
void GameGraphicsSettingsFailed(const std::string& text){Activity("gameGraphicsSettingsFailed","(Ljava/lang/String;)V",[&](JNIEnv* e,jclass c,jmethodID m){jstring value=e->NewStringUTF(text.c_str());e->CallStaticVoidMethod(c,m,value);e->DeleteLocalRef(value);});}
}
extern "C" JNIEXPORT jfloat JNICALL Java_com_ptport_app_PTActivity_nativeGetRenderScale(JNIEnv*,jclass){return pt::android::RenderScale();}
extern "C" JNIEXPORT void JNICALL Java_com_ptport_app_PTActivity_nativeSetRenderScale(JNIEnv*,jclass,jfloat value){pt::android::SetRenderScale(value);}
extern "C" JNIEXPORT void JNICALL Java_com_ptport_app_PTActivity_nativeSetPerformanceProfile(JNIEnv*,jclass,jint value){pt::android::Load();if(value>0)pt::android::graphics_changed=false;pt::android::profile=std::clamp(value,0,3);}
extern "C" JNIEXPORT void JNICALL Java_com_ptport_app_PTActivity_nativeSetBloomPassLimit(JNIEnv*,jclass,jint value){pt::android::Load();pt::android::bloom=value>0?6:0;}
extern "C" JNIEXPORT jstring JNICALL Java_com_ptport_app_PTActivity_nativeGetGameGraphicsSettings(JNIEnv* env,jclass){std::lock_guard lock(pt::android::graphics_mutex);return env->NewStringUTF(pt::android::graphics_snapshot.c_str());}
extern "C" JNIEXPORT void JNICALL Java_com_ptport_app_PTActivity_nativeApplyGameGraphicsSettings(JNIEnv*,jclass){pt::android::Load();pt::android::profile=-1;pt::android::graphics_changed=true;}
extern "C" JNIEXPORT void JNICALL Java_com_ptport_app_PTActivity_nativeSetLoggingEnabled(JNIEnv*,jclass,jboolean enabled){
    const bool was=pt::LogEnabled();pt::LogSetEnabled(enabled==JNI_TRUE);
    if(enabled==JNI_TRUE && !was){pt::LogSetFile(pt::android::LogDir()/"pt.log",true);pt::android::InstallCrashLog(pt::android::LogDir()/"pt-crash.txt");}
}
extern "C" JNIEXPORT void JNICALL Java_com_ptport_app_TouchControls_nativeLookDelta(JNIEnv*,jclass,jfloat dx,jfloat dy){
    if(!std::isfinite(dx)||!std::isfinite(dy))return;
    std::lock_guard lock(pt::android::look_mutex);pt::android::look_x+=dx;pt::android::look_y+=dy;
}
extern "C" JNIEXPORT void JNICALL Java_com_ptport_app_TouchControls_nativeClearLook(JNIEnv*,jclass){std::lock_guard lock(pt::android::look_mutex);pt::android::look_x=pt::android::look_y=0.0f;}
#endif
