#!/usr/bin/env python3
"""Compile and execute production mobile kernels, surface policy and presets."""
from pathlib import Path
import os,tempfile,subprocess
repo=Path(__file__).resolve().parents[3]
def extract(path,signature):
 s=(repo/path).read_text();a=s.index(signature);start=s.index('{',a);i=start+1;depth=1
 while depth:
  if s[i]=='{':depth+=1
  elif s[i]=='}':depth-=1
  i+=1
 return s[a:i]
policy=extract('src/engine/render/vk_context.cpp','bool Context::SuboptimalNeedsRecreate()')
preset=extract('src/main.cpp','void ApplyMobilePreset(')
prefix=r'''#include <cstdio>
#include <cassert>
#include <cmath>
#include <algorithm>
#include "engine/render/mobile_bloom.h"
#include "engine/platform/settings.h"
#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>
struct SDL_Window{}; SDL_Window window; void* native=reinterpret_cast<void*>(1);int width=2400,height=1080;
constexpr int SDL_PROP_WINDOW_ANDROID_WINDOW_POINTER=1;
int SDL_GetWindowProperties(SDL_Window*){return 1;}void* SDL_GetPointerProperty(int,int,void*){return native;}
void SDL_GetWindowSizeInPixels(SDL_Window*,int* w,int* h){*w=width;*h=height;}
VkSurfaceCapabilitiesKHR caps{};
VkResult vkGetPhysicalDeviceSurfaceCapabilitiesKHR(VkPhysicalDevice,VkSurfaceKHR,VkSurfaceCapabilitiesKHR* out){*out=caps;return VK_SUCCESS;}
template<class... Args>void LogInfo(const char*,Args...){}
struct Context{
 SDL_Window* window_=&window;VkSurfaceKHR surface=reinterpret_cast<VkSurfaceKHR>(1);VkPhysicalDevice physical{};void* android_native_window_=native;
 VkSurfaceCapabilitiesKHR surface_caps_{};VkExtent2D window_extent_{2400,1080};bool suboptimal_logged_=false;
 bool MatchesWindowSize(uint32_t w,uint32_t h){return window_extent_.width==w && window_extent_.height==h;}
 bool SuboptimalNeedsRecreate();
};
namespace pt{using ::LogInfo;namespace android{float scale=.667f;float RenderScale(){return scale;}void SetRenderScale(float v){scale=v;}}}
struct MockContext{void* device=nullptr;};struct MockTextures{int anisotropy=-1;void SetAnisotropy(int v){anisotropy=v;}};
struct MockRenderer{MockContext ctx;bool vsync=true;void SetVsync(bool v){vsync=v;}MockContext& Context(){return ctx;}};
struct App{pt::AppSettings settings;MockRenderer renderer;MockTextures textures;int graphics_calls=0;};
void ApplyGraphicsSettings(App& app){app.graphics_calls++;}
'''
body=r'''
int main(){
 for(int requested=1;requested<=64;requested++)for(int count=1;count<=requested;count++){
  double actual=0,expected=0,energy=0;
  for(int j=0;j<requested;j++)expected+=(j+.5)*(j+.5);
  for(int i=0;i<count;i++){auto s=pt::MobileBloomStep(requested,count,i);assert(std::isfinite(s.radius) && s.radius>0);actual+=s.radius*s.radius;energy+=s.fraction;if(count==requested)assert(std::abs(s.radius-(i+.5))<1e-6);}
  assert(std::abs(actual-expected)<expected*1e-6);assert(std::abs(energy-1)<1e-6);
 }
 Context c;c.surface_caps_=caps;for(int i=0;i<10000;i++)assert(!c.SuboptimalNeedsRecreate());
 width=1200;assert(c.SuboptimalNeedsRecreate());width=2400;
 native=reinterpret_cast<void*>(2);assert(c.SuboptimalNeedsRecreate());native=reinterpret_cast<void*>(1);
 caps.currentTransform=VK_SURFACE_TRANSFORM_ROTATE_90_BIT_KHR;assert(c.SuboptimalNeedsRecreate());caps={};
 caps.currentExtent={1080,2400};assert(c.SuboptimalNeedsRecreate());caps={};
 c.android_native_window_=nullptr;assert(c.SuboptimalNeedsRecreate());c.android_native_window_=native;
 c.surface=VK_NULL_HANDLE;assert(c.SuboptimalNeedsRecreate());
 App app;app.settings.display.fps_limit=30;app.settings.graphics.shadow_quality=3;app.settings.ray_tracing.shadows=2;
 ApplyMobilePreset(app,1);assert(app.settings.display.fps_limit==0 && !app.settings.display.vsync && !app.renderer.vsync);assert(app.settings.graphics.shadow_quality==1 && app.settings.graphics.texture_detail==0);assert(!app.settings.graphics.bloom && !app.settings.graphics.reflections && !app.settings.graphics.ambient_occlusion);assert(app.settings.ray_tracing.shadows==0 && pt::android::scale==.5f);assert(app.textures.anisotropy==-1);
 app.renderer.ctx.device=reinterpret_cast<void*>(1);ApplyMobilePreset(app,2);assert(app.settings.display.fps_limit==60 && app.settings.graphics.bloom && app.settings.graphics.shadow_quality==2);assert(app.textures.anisotropy==2);
 app.settings.display.fps_limit=144;app.settings.graphics.shadow_quality=3;ApplyMobilePreset(app,0);assert(app.settings.display.fps_limit==144 && app.settings.graphics.shadow_quality==3);
 std::puts("Render checks passed: 2080 bloom variance/energy cases; 10000 stable SUBOPTIMAL frames; resize, rotation, window and surface loss; preset behavior");
}
'''
with tempfile.TemporaryDirectory() as tmp:
 t=Path(tmp);file=t/'render.cpp';file.write_text(prefix+policy+'\n'+preset+body)
 compiler=os.environ.get('CXX','g++');extra=os.environ.get('PT_TEST_CXX_FLAGS','')
 import shlex
 args=[compiler]+shlex.split(extra)+['-std=c++20','-D__ANDROID__','-I'+str(repo/'src'),'-I'+str(next((repo/'build/android/_deps').glob('*/include/vulkan/vulkan.h')).parents[1]),str(file),'-o',str(t/'check')]
 subprocess.run(args,check=True);subprocess.run([str(t/'check')],check=True)
