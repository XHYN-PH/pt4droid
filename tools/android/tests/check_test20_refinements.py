#!/usr/bin/env python3
"""Production policies, collision queries and render-target ownership without a phone."""
from pathlib import Path
import os,shlex,subprocess,tempfile,re
repo=Path(__file__).resolve().parents[3]
def extract(path,sig):
 s=(repo/path).read_text();a=s.index(sig);i=s.index('{',a)+1;depth=1
 while depth:depth+=(s[i]=='{')-(s[i]=='}');i+=1
 return s[a:i]
flags=shlex.split(os.environ.get('PT_TEST_CXX_FLAGS',''))
incs=['src','build/android/_deps/glm-src','build/android/_deps/volk-src','build/android/_deps/vulkan_headers-src/include']
compiler=[os.environ.get('CXX','g++'),'-std=c++20','-O1','-pthread','-ffunction-sections','-fdata-sections','-Wl,--gc-sections']+flags+['-I'+str(repo/p)for p in incs]
with tempfile.TemporaryDirectory()as folder:
 root=Path(folder);binary=root/'policies'
 subprocess.run(compiler+[str(repo/'tests/mobile_refinements20_test.cpp'),str(repo/'src/engine/physics/collision_world.cpp'),'-o',str(binary)],check=True,timeout=180)
 subprocess.run([str(binary)],check=True,timeout=60)
 # Execute actual bank ownership, target preservation and conditional upscale
 # allocation methods against counted image allocations; double frees fail.
 methods=[]
 for path,sigs in [('src/engine/render/scene_renderer.cpp',['std::vector<RenderTarget*> SceneRenderer::BankTargets(','std::vector<RenderTarget*> SceneRenderer::SharedTargets(','bool SceneRenderer::CreateTarget(','void SceneRenderer::DestroyTargets(','bool SceneRenderer::SwitchTargetBank(','void SceneRenderer::DropTargetBanks(']),('src/engine/render/upscale/scene_upscale.cpp',['bool SceneRenderer::CreateUpscaleTargets(','void SceneRenderer::DestroyUpscaleTargets('])]:
  methods += [extract(path,sig)for sig in sigs]
 names={}
 for text in methods:
  for name,index in re.findall(r'&([a-zA-Z][a-zA-Z0-9_]*_)(?:\[(\d+)\])?',text):names[name]=max(names.get(name,0),int(index)+1 if index else 0)
 # Some target uses use a direct member instead of &member syntax.
 names['refmap_color_']=0;names['rt_ao_images_']=2
 fields='\n'.join('RenderTarget '+n+('['+str(count)+']'if count else '')+';'for n,count in names.items())
 prefix=(repo/'tests/target_banks20_test.cpp.in').read_text().replace('/* TARGET_FIELDS */',fields)
 prefix=prefix.replace('/* SUBSURFACE_INLINE */','\n'.join(extract('src/engine/render/subsurface_pass.h',sig)for sig in ['TargetState TakeTargets()','void SwapTargets(']))
 subsurface=[extract('src/engine/render/subsurface_pass.cpp',sig)for sig in ['bool SubsurfacePass::Init(','void SubsurfacePass::Shutdown(','bool SubsurfacePass::CreateTargets(','void SubsurfacePass::DestroyTargets()','void SubsurfacePass::DestroyTargets(TargetState&','uint64_t SubsurfacePass::TargetBytes()','void SubsurfacePass::Record(']]
 source=root/'banks.cpp';source.write_text(prefix+'\nnamespace pt {\n'+'\n'.join(subsurface+methods)+'\n}\n');binary=root/'banks'
 subprocess.run(compiler+[str(source),'-o',str(binary)],check=True,timeout=120);subprocess.run([str(binary)],check=True,timeout=30)
 with tempfile.TemporaryDirectory()as case:
  t=Path(case);source=t/'filter.cpp';binary=t/'filter'
  body=extract('src/engine/render/vfx_pass.h','bool FilterSamplesScene() const')
  source.write_text('#include <vector>\n#include <cassert>\nnamespace vfx{enum class Layer{World,Flare,Screen};}struct Pass{struct Draw{int count;vfx::Layer layer;bool refract;};struct{std::vector<Draw>draws;}list_;'+body+'};int main(){Pass p;assert(!p.FilterSamplesScene());p.list_.draws={{1,vfx::Layer::World,true},{0,vfx::Layer::Screen,true},{1,vfx::Layer::Screen,false}};assert(!p.FilterSamplesScene());p.list_.draws.push_back({1,vfx::Layer::Screen,true});assert(p.FilterSamplesScene());}')
  subprocess.run(compiler+[str(source),'-o',str(binary)],check=True,timeout=60);subprocess.run([str(binary)],check=True,timeout=15)
print('Test20 production policies and target lifetime checks passed')
