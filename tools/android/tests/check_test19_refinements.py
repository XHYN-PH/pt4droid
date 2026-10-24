#!/usr/bin/env python3
"""Execute production candidate policies and injected rendering failures without a phone."""
from pathlib import Path
import os,shlex,subprocess,tempfile
repo=Path(__file__).resolve().parents[3]
def extract(path,signature):
 s=(repo/path).read_text();a=s.index(signature);i=s.index('{',a)+1;d=1
 while d:d+=(s[i]=='{')-(s[i]=='}');i+=1
 return s[a:i]
incs=['src','build/android/_deps/glm-src','build/android/_deps/volk-src','build/android/_deps/vulkan_headers-src/include','build/android/_deps/vma-src/include']
c=[os.environ.get('CXX','g++'),'-std=c++20','-O1','-pthread']+shlex.split(os.environ.get('PT_TEST_CXX_FLAGS',''))+['-I'+str(repo/p)for p in incs]
with tempfile.TemporaryDirectory()as directory:
 root=Path(directory)
 def run(name,text):
  source=root/(name+'.cpp');source.write_text(text);binary=root/name
  subprocess.run(c+[str(source),'-o',str(binary)],check=True,timeout=120);subprocess.run([str(binary)],check=True,timeout=30)
 subprocess.run(c+[str(repo/'tests/mobile_candidate_test.cpp'),'-o',str(root/'policies')],check=True,timeout=120);subprocess.run([str(root/'policies')],check=True,timeout=30)
 prefix=r"""
#include <memory>
#include <cassert>
#include <cstdio>
#include "engine/render/mesh.h"
#include "engine/core/parallel_for.h"
namespace pt {
template<class...T>void LogError(const char*,T...){}
struct MockContext {bool ray_query=false;unsigned fail_create=0,creates=0,destroys=0,uploads=0;bool fail_upload=false;
 bool CreateBuffer(vk::Buffer& b,VkDeviceSize size,VkBufferUsageFlags,bool){if(++creates==fail_create)return false;b.buffer=(VkBuffer)1;b.size=size;return true;}
 void DestroyBuffer(vk::Buffer& b){if(b.buffer)++destroys;b={};}
 bool UploadMesh(vk::Buffer& a,const void*,VkDeviceSize,vk::Buffer& b,const void*,VkDeviceSize){assert(a.buffer&&b.buffer);++uploads;return !fail_upload;}
};
struct MockRenderer{MockContext context;MockContext& Context(){return context;}};
struct SceneRenderer{MockRenderer*renderer_;std::unique_ptr<GpuMesh>Upload(const MeshData&);};
"""
 main=r"""
}
int main(){pt::MeshData data;data.vertices.resize(3);data.indices={0,1,2};pt::MockRenderer r;pt::SceneRenderer s{&r};
 for(unsigned fail:{1u,2u}){r.context={};r.context.fail_create=fail;assert(!s.Upload(data));assert(r.context.uploads==0&&r.context.destroys==fail-1);}
 r.context={};r.context.fail_upload=true;assert(!s.Upload(data));assert(r.context.destroys==2&&r.context.uploads==1);
 r.context={};assert(s.Upload(data));assert(r.context.uploads==1&&r.context.destroys==0);
 puts("Mesh failure recording: each allocation failure and submission failure cleans up, success uses one batched upload");}
"""
 run('mesh',prefix+extract('src/engine/render/scene_renderer.cpp','std::unique_ptr<GpuMesh> SceneRenderer::Upload(')+main)
 prefix=r"""
#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>
#include <vector>
#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <cstdio>
#include <cassert>
namespace pt {
namespace vfx{enum class BlendMode:uint8_t{Alpha};}
namespace vk{int attempts=0;bool fail=true;bool Check(VkResult r,const char*){return r==VK_SUCCESS;}
 VkResult CreateGraphicsPipelinesCached(VkDevice,uint32_t,const VkGraphicsPipelineCreateInfo*,const VkAllocationCallbacks*,VkPipeline*p){++attempts;if(fail)return VK_ERROR_OUT_OF_HOST_MEMORY;*p=(VkPipeline)1;return VK_SUCCESS;}}
void vkDestroyPipeline(VkDevice,VkPipeline,const VkAllocationCallbacks*){}
VkPipelineColorBlendAttachmentState BlendState(vfx::BlendMode,bool,bool){return {};}
struct Renderer{struct ContextType{void CheckDeviceLost(VkResult,const char*){}}context;ContextType&Context(){return context;}};
struct VfxPass {
"""
 key=extract('src/engine/render/vfx_pass.h','struct PipelineKey')+';'
 fields=r"""
 VkDevice device_=VK_NULL_HANDLE;VkShaderModule vertex_=VK_NULL_HANDLE,fragment_=VK_NULL_HANDLE;VkPipelineLayout layout_=VK_NULL_HANDLE;
 std::vector<std::pair<PipelineKey,VkPipeline>>pipelines_;struct PipelineFailure{PipelineKey key;uint64_t retry;uint32_t attempts;};std::vector<PipelineFailure>pipeline_failures_;
 uint64_t generation_=1;Renderer renderer;Renderer*renderer_=&renderer;VkPipeline Pipeline(const PipelineKey&);
};
"""
 main=r"""
}
int main(){pt::VfxPass pass;pt::VfxPass::PipelineKey key;assert(!pass.Pipeline(key));assert(!pass.Pipeline(key)&&pt::vk::attempts==1&&pass.pipelines_.empty());
 pt::vk::fail=false;pass.generation_=31;assert(pass.Pipeline(key));assert(pass.Pipeline(key)&&pt::vk::attempts==2&&pass.pipelines_.size()==1&&pass.pipeline_failures_.empty());
 puts("VFX failure recording: transient failure is not cached, retries are delayed, successful variants are reused");}
"""
 run('pipeline',prefix+key+fields+extract('src/engine/render/vfx_pass.cpp','VkPipeline VfxPass::Pipeline(')+main)
 prefix=r"""
#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>
#include <array>
#include <vector>
#include <unordered_map>
#include <cstdint>
#include <cstdio>
#include <cassert>
#include "engine/render/shadow_cache.h"
namespace pt {
namespace gpu{constexpr uint32_t kMatAlphaTest=1;}
VkImageView last=VK_NULL_HANDLE;void vkUpdateDescriptorSets(VkDevice,uint32_t,const VkWriteDescriptorSet*w,uint32_t,const VkCopyDescriptorSet*){last=w[0].pImageInfo->imageView;}
struct TextureManager {struct Material{uint32_t flags=1,albedo=4;};std::vector<Material>materials_{Material{}};
 std::unordered_map<uint32_t,uint32_t>redirects_,enhanced_images_,cube_slots_;std::unordered_map<uint32_t,uint64_t>texture_generations_;
 bool enhanced_enabled_=false;uint32_t anisotropy_=2;struct Image{VkImageView view=VK_NULL_HANDLE;};std::vector<Image>images_{5};std::vector<uint8_t>cube_{0,0,0,0,0};
 struct Context{VkDevice device=VK_NULL_HANDLE;}context;Context*ctx_=&context;VkDescriptorSet set_=VK_NULL_HANDLE;VkSampler sampler_=VK_NULL_HANDLE,base_sampler_=VK_NULL_HANDLE;
 static constexpr uint32_t kWhite=0;std::array<uint64_t,3>ShadowAlphaState(uint32_t)const;void WriteResidentDescriptor(uint32_t);
};
"""
 main=r"""
}
int main(){pt::TextureManager t;t.images_[0].view=(VkImageView)1;t.WriteResidentDescriptor(4);auto before=t.ShadowAlphaState(0);auto fallback=pt::last;
 pt::ShadowMapState state;state.casters.emplace_back();state.casters[0].alpha=before;pt::ShadowTileCache cache;cache.Recorded(state);
 t.images_[4].view=(VkImageView)9;t.WriteResidentDescriptor(4);auto after=t.ShadowAlphaState(0);state.casters[0].alpha=after;
 assert(fallback!=pt::last&&before!=after&&!cache.Reuse(state));t.materials_[0].flags=0;auto opaque=t.ShadowAlphaState(0);t.WriteResidentDescriptor(4);assert(t.ShadowAlphaState(0)==opaque);
 puts("Alpha shadow recording: descriptor restoration invalidates alpha tiles and leaves opaque dependencies unchanged");}
"""
 run('alpha',prefix+extract('src/engine/render/texture_manager.cpp','std::array<uint64_t,3> TextureManager::ShadowAlphaState(')+extract('src/engine/render/texture_manager.cpp','void TextureManager::WriteResidentDescriptor(')+main)
 run('gpu_boundaries',(repo/'tests/candidate_gpu_boundaries.cpp').read_text()+'\nnamespace pt::vk {\n'+
     extract('src/engine/render/vk_context.cpp','bool Context::UploadMesh(')+'\n}\nnamespace pt {\n'+
     extract('src/engine/render/renderer.cpp','void Renderer::PollScreenshots(')+'\n'+
     extract('src/engine/render/renderer.cpp','bool Renderer::QueueScreenshot(')+'\n}\n')
print('Candidate policies and production error-path regressions passed')
