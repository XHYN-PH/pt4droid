#!/usr/bin/env python3
"""Fault-inject parallel dispatch allocation and shadow initialization failures."""
from pathlib import Path
import os, resource, shlex, subprocess, tempfile
repo=Path(__file__).resolve().parents[3]
resource.setrlimit(resource.RLIMIT_CORE,(0,0))
source=(repo/'src/engine/render/scene_renderer.cpp').read_text()
a=source.index('bool SceneRenderer::EnsureShadowTarget(');i=source.index('{',a)+1;depth=1
while depth:depth+=(source[i]=='{')-(source[i]=='}');i+=1
method=source[a:i]
shadow=r'''
#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <vector>
#include <utility>
#include <initializer_list>
VkResult vkDeviceWaitIdle(VkDevice){return VK_SUCCESS;}
void vkCmdEndRendering(VkCommandBuffer){}
namespace pt {
constexpr VkFormat kDepthFormat=VK_FORMAT_D32_SFLOAT;
struct Image{VkImage image=VK_NULL_HANDLE;};
struct RenderTarget{Image image;VkExtent2D size{};bool Valid()const{return image.image!=VK_NULL_HANDLE;}VkExtent2D Extent()const{return size;}};
struct Context{bool fail_submit=false;unsigned submits=0;std::vector<VkImage>destroyed;
 template<class F>bool Submit(F f){++submits;if(fail_submit)return false;f(VK_NULL_HANDLE);return true;}
 void DestroyImage(Image& image){if(image.image)destroyed.push_back(image.image);image={};}
};
struct Renderer{pt::Context ctx;pt::Context& Context(){return ctx;}};
void UseTargets(VkCommandBuffer,std::initializer_list<std::pair<RenderTarget*,VkImageLayout>>){}
void BeginPass(VkCommandBuffer,VkExtent2D,std::initializer_list<int>,RenderTarget*,bool,bool){}
template<class...T>void LogInfo(const char*,T...){}
struct SceneRenderer{Renderer*renderer_;VkDevice device_=VK_NULL_HANDLE;struct{int shadow_quality=2;}graphics;
 RenderTarget shadow_,shadow_static_;bool shadow_static_unavailable_=true,fail_create=false;unsigned created=0,invalidations=0,writes=0;VkExtent2D extent_{2400,1080};
 bool CreateTarget(RenderTarget& target,VkFormat,VkExtent2D size,VkImageUsageFlags,VkImageAspectFlags){++created;if(fail_create)return false;target.image.image=(VkImage)uintptr_t(100+created);target.size=size;return true;}
 void InvalidateShadowCache(){++invalidations;}void WriteImageDescriptors(){++writes;}bool EnsureShadowTarget();
};
METHOD
}
int main(){
 using namespace pt;const auto old=(VkImage)uintptr_t(1),cached=(VkImage)uintptr_t(2);
 Renderer r;SceneRenderer s{&r};s.shadow_={{old},{2048,2048}};s.shadow_static_={{cached},{2048,2048}};
 r.ctx.fail_submit=true;assert(s.EnsureShadowTarget());assert(s.shadow_.image.image==old);assert(s.shadow_static_.image.image==cached);assert(s.shadow_static_unavailable_);
 assert(s.writes==0&&s.invalidations==0&&r.ctx.destroyed.size()==1&&r.ctx.destroyed[0]!=(VkImage)old);
 Renderer initial;initial.ctx.fail_submit=true;SceneRenderer fresh{&initial};assert(!fresh.EnsureShadowTarget());assert(!fresh.shadow_.Valid());assert(initial.ctx.destroyed.size()==1);
 s.fail_create=true;unsigned submissions=r.ctx.submits;assert(s.EnsureShadowTarget());assert(s.shadow_.image.image==old&&r.ctx.submits==submissions);
 s.fail_create=false;r.ctx.fail_submit=false;assert(s.EnsureShadowTarget());assert(s.shadow_.Valid()&&s.shadow_.image.image!=old&&!s.shadow_static_.Valid());assert(s.writes==1&&s.invalidations==1&&!s.shadow_static_unavailable_);
 unsigned creates=s.created;assert(s.EnsureShadowTarget());assert(s.created==creates&&s.writes==1);
 puts("Shadow failure recovery: failed initialization preserves the old atlas/cache, releases replacement images, initial failure is reported, retry succeeds and unchanged size is reused");
}
'''.replace('METHOD',method)
compiler=[os.environ.get('CXX','g++'),'-std=c++20','-O1','-pthread']+shlex.split(os.environ.get('PT_TEST_CXX_FLAGS',''))+['-I'+str(repo/'src'),'-I'+str(repo/'build/android/_deps/vulkan_headers-src/include')]
with tempfile.TemporaryDirectory()as folder:
 t=Path(folder);cpp=t/'shadow.cpp';cpp.write_text(shadow);results=[]
 for name,path in [('parallel',repo/'tests/parallel_allocation_failure_test.cpp'),('shadow',cpp)]:
  binary=t/name;subprocess.run(compiler+[str(path),'-o',str(binary)],check=True,timeout=120)
  r=subprocess.run([str(binary)],timeout=30);results.append(r.returncode)
  print(name,'passed'if r.returncode==0 else'FAILED',flush=True)
 raise SystemExit(int(any(results)))
