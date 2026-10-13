#!/usr/bin/env python3
"""Compare material uploads and preparation selection against Test 8."""
from pathlib import Path
import os, shlex, subprocess, tempfile

repo = Path(__file__).resolve().parents[3]
def extract(text, signature):
    start = text.index(signature); pos = text.index('{', start) + 1; depth = 1
    while depth:
        depth += (text[pos] == '{') - (text[pos] == '}'); pos += 1
    return text[start:pos]

fixture = r'''
#include <algorithm>
#include <bit>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <string>
#include <string_view>
#include <vector>
#include <glm/glm.hpp>
#include "engine/render/gpu_types.h"
namespace pt {
@@MATERIAL@@;
@@GPU@@;
struct MaterialTextureRead { std::string path; bool raw=false; };
static std::vector<MaterialTextureRead> observed;
struct TextureManager {
 static constexpr uint32_t kWhite=0,kFlatNormal=1,kBlack=2,kGrey=4,kGreySrgb=5,kNoCube=UINT32_MAX;
 uint32_t CubeSlot(uint32_t value)const{return value>5?value%17:kNoCube;}
};
struct ModelCache {
 TextureManager textures_;
 uint32_t LoadTexture(const std::string& path,uint32_t fallback,bool raw=false){if(path.empty())return fallback;observed.push_back({path,raw});return 20+uint32_t(observed.size());}
 bool ViewReflection(const FmdlMaterial&){return true;}
 MaterialGpu BuildMaterial(const FmdlMaterial&,uint32_t);
};
@@HELPERS@@
@@BODY@@
}
int main(){using namespace pt;
 uint64_t hash=1469598103934665603ull;
 auto bytes=[&](const void* data,size_t n){for(size_t i=0;i<n;++i){hash^=static_cast<const uint8_t*>(data)[i];hash*=1099511628211ull;}};
 std::vector<std::string> shaders={"fox_3dfw_constant_srgb","fox_3dfw_constant","tpp_3dfw_constant","fox_3dfw_glass","fox_3dfw_glass_commonref","fox_3dfw_parallax","fox_3dndw_shadow","fox_3ddf_tempolary_albedoview","fox_3ddf_blin","fox_3ddf_layer","fox_3ddf_normal_wave_directivealpha","fox_3ddf_reflector","fox_3ddf_hair","fox_3ddf_lightcover","fox_3ddf_eye"};
 std::vector<std::string> slots={"Base_Tex_SRGB","Base_Tex2","Mask_Tex","NormalMap_Tex","SpecularMap_Tex","GlassReflection_Tex","Height_Tex","MatParamMap_Tex","SubNormalMap_Tex","SubNormalMask_Tex","Translucent_Tex_LIN","Layer_Tex","LayerMask_Tex","Shift_Tex","InternalNormalMap_Tex","CoverTranslucentMap_Tex","LensHeight_Tex","ViewReflection_Tex","unused"};
 for(int trial=0;trial<1200;++trial){FmdlMaterial m;m.shader=shaders[trial%shaders.size()];m.technique="fox3DDF_Blin_Translucent_4MT_MaskUV_DirectiveAlpha_NC";
  for(size_t i=0;i<slots.size();++i)if((trial>>(i%9))&1)m.textures.emplace_back(slots[i],trial%7==0?"":"/Assets/"+slots[i]+std::to_string(i));
  for(const char* name:{"SelfColor","Incidence_Color","MinRoughness","WindDir","WindOffset","WindAmplitude","WindAnimTime","WindRandAmplitude"})m.vectors.emplace_back(name,glm::vec4(.3f,.7f,.9f,.5f));
  ModelCache cache;observed.clear();MaterialGpu result=cache.BuildMaterial(m,trial%2?gpu::kMatAlphaTest:0);bytes(&result,sizeof(result));
  for(const auto& use:observed){bytes(use.path.data(),use.path.size());bytes(&use.raw,sizeof(use.raw));}
@@PLAN_CHECK@@
 }
 printf("%016llx\n",static_cast<unsigned long long>(hash));
}
'''
current = (repo/'src/engine/render/model_cache.cpp').read_text()
baseline = subprocess.check_output(['git','show','pt4droid-v0.2.1-test.8:src/engine/render/model_cache.cpp'],cwd=repo,text=True)
helpers = ['bool MaterialLegacy(', 'const std::string* FindTexture(', 'bool HasVector(', 'uint32_t MultiMaterialCount(', 'uint32_t MaterialKindOf(']
material = extract((repo/'src/engine/assets/fmdl.h').read_text(),'struct FmdlMaterial')
gpu = extract((repo/'src/engine/render/texture_manager.h').read_text(),'struct MaterialGpu')
compiler = [os.environ.get('CXX','g++'),'-std=c++20','-O2'] + shlex.split(os.environ.get('PT_TEST_CXX_FLAGS',''))
compiler += ['-I'+str(repo/'src'),'-I'+str(repo/'build/android/_deps/glm-src')]
with tempfile.TemporaryDirectory() as directory:
    root=Path(directory);outputs=[]
    for name,source in [('baseline',baseline),('production',current)]:
        body=extract(source,'MaterialGpu ModelCache::BuildMaterial(')
        check=''
        if name=='production':
            body=extract(source,'MaterialGpu BuildMaterialGpu(')+'\n'+body+'\n'+extract(source,'std::vector<MaterialTextureRead> MaterialTextureReads(')
            check='auto plan=MaterialTextureReads(m);assert(plan.size()==observed.size());for(size_t i=0;i<plan.size();++i)assert(plan[i].path==observed[i].path && plan[i].raw==observed[i].raw);'
        text=fixture.replace('@@MATERIAL@@',material).replace('@@GPU@@',gpu).replace('@@HELPERS@@','\n'.join(extract(source,s) for s in helpers)).replace('@@BODY@@',body).replace('@@PLAN_CHECK@@',check)
        cpp=root/(name+'.cpp');cpp.write_text(text);exe=root/name
        subprocess.run(compiler+[str(cpp),'-o',str(exe)],check=True,timeout=120)
        outputs.append([subprocess.check_output([str(exe)],text=True,env=dict(os.environ,**env)).strip() for env in [{},{'PT_MATERIAL_LEGACY':'1','PT_HAIR_LEGACY':'1'}]])
    assert outputs[0]==outputs[1],outputs
    print('2400 material cases: exact Test 8 GPU/texture-use output; preparation matches actual upload selection',outputs[1])
