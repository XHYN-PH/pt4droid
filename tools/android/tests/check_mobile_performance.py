#!/usr/bin/env python3
"""Execute the production performance parser, JNI updates, thermal limits and static-library log gate."""
from pathlib import Path
import os, subprocess, tempfile

repo=Path(__file__).resolve().parents[3]
with tempfile.TemporaryDirectory() as temporary:
    root=Path(temporary)
    stubs={
        'engine/platform/android.h':'''#pragma once
#include <filesystem>
#include "engine/platform/mobile_performance.h"
namespace pt::android {MobilePerformanceOptions PerformanceOptions();int EffectiveFrameLimit(int);}
''',
        'SDL3/SDL_system.h':'''#pragma once
const char* SDL_GetAndroidInternalStoragePath();''',
        'jni.h':'''#pragma once
#define JNIEXPORT
#define JNICALL
struct JNIEnv {};using jclass=void*;using jint=int;''',
        'android/log.h':'''#pragma once''',
        'engine/core/log.h':'''#pragma once
namespace pt {bool LogEnabled();}''',
    }
    for name,content in stubs.items():
        file=root/name;file.parent.mkdir(parents=True,exist_ok=True);file.write_text(content)
    source=root/'check.cpp'
    source.write_text(r'''#include <cassert>
#include <fstream>
#include <string>
#include <thread>
#include <cstdarg>
#include <cstdio>
#include <atomic>
#include "engine/platform/android.h"
#include "engine/platform/mobile_driver_policy.h"
#include "engine/platform/mobile_frame_pacing.h"
#include <cstring>
std::string storage;const char* SDL_GetAndroidInternalStoragePath(){return storage.c_str();}
extern "C" void Java_com_ptport_app_PTActivity_nativeReloadPerformance(void*,void*);
extern "C" void Java_com_ptport_app_PTActivity_nativeThermalStatus(void*,void*,int);
extern "C" int __wrap___android_log_print(int,const char*,const char*,...);
extern "C" int __wrap___android_log_vprint(int,const char*,const char*,va_list);
extern "C" int __wrap___android_log_write(int,const char*,const char*);
bool enabled=false;int log_calls=0;
namespace pt {bool LogEnabled(){return enabled;}}
extern "C" int __real___android_log_vprint(int,const char*,const char*,va_list){return ++log_calls;}
extern "C" int __real___android_log_write(int,const char*,const char*){return ++log_calls;}
void config(const char* text){std::ofstream(std::filesystem::path(storage)/"mobile_performance.ini")<<text;Java_com_ptport_app_PTActivity_nativeReloadPerformance(nullptr,nullptr);}
int main(int argc,char** argv){
 auto extensions=pt::android::FramePacingExtensions(VK_NULL_HANDLE,0,nullptr,[](auto,uint32_t,auto,uint32_t* count,char** names){
  if(!names){*count=2;return;}assert(*count==2&&names[0]&&names[1]);std::strcpy(names[0],"VK_GOOGLE_display_timing");std::strcpy(names[1],"VK_KHR_swapchain");
 });
 assert(extensions.size()==2&&std::string(extensions[0].data())=="VK_GOOGLE_display_timing");
 assert(argc==2);storage=argv[1];auto defaults=pt::android::PerformanceOptions();
 assert(!defaults.frame_pacing&&!defaults.sgsr&&!defaults.sustained&&!defaults.invert_y);
 assert(defaults.mouse_sensitivity==1&&defaults.controller_sensitivity==1);
 defaults.controller_sensitivity=.5f;assert(pt::MobileControllerSensitivity(1,defaults,false)==.5f);
 assert(pt::MobileControllerSensitivity(2,defaults,true)==2);defaults.controller_sensitivity=1;
 auto touch=pt::CombineMobileLook(0,0,100,50,0,0,.003f,.0015f,defaults);
 auto changed=defaults;changed.mouse_sensitivity=3;
 auto same_touch=pt::CombineMobileLook(0,0,100,50,0,0,.015f,.0015f,changed);
 assert(touch.x==same_touch.x&&touch.y==same_touch.y);
 auto gyro=pt::CombineMobileLook(0,0,0,0,.2f,.1f,.015f,.0015f,changed);
 assert(gyro.x==.2f&&gyro.y==.1f);changed.invert_y=true;
 auto inverted=pt::CombineMobileLook(10,20,30,40,.2f,.1f,.003f,.0015f,changed);
 changed.invert_y=false;auto normal=pt::CombineMobileLook(10,20,30,40,.2f,.1f,.003f,.0015f,changed);
 assert(normal.x==inverted.x&&normal.y==-inverted.y);
 for(int requested:{0,20,30,60,90,120,144})for(int status=0;status<=6;++status){
  Java_com_ptport_app_PTActivity_nativeThermalStatus(nullptr,nullptr,status);
  assert(pt::android::EffectiveFrameLimit(requested)==requested);
 }
 config("sustained=1\nframe_pacing=1\nsgsr=1\ninvert_y=1\nsharpness=90\nmouse_sensitivity=-50\ncontroller_sensitivity=80\nao_quality=1\nreflection_quality=1\n");
 auto o=pt::android::PerformanceOptions();assert(o.frame_pacing&&o.sgsr&&o.sustained&&o.invert_y);
 assert(o.sharpness==2&&o.mouse_sensitivity==.25f&&o.controller_sensitivity==3&&o.ao_quality==1&&o.reflection_quality==1);
 for(int requested:{0,20,30,60,90,120,144})for(int status=-2;status<=9;++status){
  Java_com_ptport_app_PTActivity_nativeThermalStatus(nullptr,nullptr,status);
  int expected=status<2?requested:status>=4?20:status>=3?30:45;
  if(status>=2&&requested>0)expected=std::min(requested,expected);
  assert(pt::android::EffectiveFrameLimit(requested)==expected);
 }
 config("sharpness=nan\nmouse_sensitivity=inf\ncontroller_sensitivity=1xyz\nsharpness=\nmouse_sensitivity=\n");
 o=pt::android::PerformanceOptions();assert(o.sharpness==1&&o.mouse_sensitivity==1&&o.controller_sensitivity==1&&!o.sustained);
 assert(pt::android::IsMaliGpu("Mali-G615 MC6")&&pt::android::IsMaliGpu("Immortalis-G720")&&!pt::android::IsMaliGpu("Adreno 750"));
 __wrap___android_log_print(4,"test","off %d",1);__wrap___android_log_write(4,"test","off");assert(log_calls==0);
 enabled=true;__wrap___android_log_print(4,"test","on %d",1);__wrap___android_log_write(4,"test","on");assert(log_calls==2);
 enabled=false;__wrap___android_log_write(4,"test","off again");assert(log_calls==2);
 std::atomic<bool> stop=false;
 std::thread reader([&]{while(!stop){auto value=pt::android::PerformanceOptions();assert(value.mouse_sensitivity>=.25f&&value.mouse_sensitivity<=3);}});
 for(int i=0;i<1000;++i)config(i%2?"mouse_sensitivity=.5":"mouse_sensitivity=2");stop=true;reader.join();
 puts("Mobile performance checks passed: safe defaults, JNI reloads, corrupt values, 133 thermal cases, GPU policy, logging and 1000 concurrent settings reloads");
}
''')
    command=[os.environ.get('CXX','g++'),'-std=c++20','-pthread','-D__ANDROID__','-I'+str(root),'-I'+str(repo/'src'),'-I'+str(repo/'build/android/_deps/vulkan_headers-src/include'),str(source),str(repo/'src/engine/platform/android_performance.cpp'),str(repo/'src/engine/platform/android_library_log.cpp'),'-o',str(root/'check')]
    subprocess.run(command,check=True,timeout=60)
    subprocess.run([str(root/'check'),str(root)],check=True,timeout=20)
