#!/usr/bin/env python3
"""Exercise authored opaque visibility using actual model and pass selection code."""
from pathlib import Path
import os, re, shlex, subprocess, tempfile

repo = Path(__file__).resolve().parents[3]
def extract(text, signature):
    start = text.index(signature)
    end = text.index('{', start) + 1
    depth = 1
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    return text[start:end]

source = (repo / 'src/engine/render/model_cache.cpp').read_text()
frame = (repo / 'src/engine/render/scene_frame.cpp').read_text()
selection = re.search(r'if \((d\.sub->pass == wanted[^\n]+)\) \{', frame)[1]
assignment = re.search(r'sub.kind = static_cast<uint8_t>\(MeshPassKind[^\n]+', source)[0]
shadow = re.search(r'sub.shadow = \(f & 0x140\)[^\n]+', source)[0]
methods = '\n'.join(extract(source, sig) for sig in
                    ['RenderPass ClassifyMaterial(', 'uint32_t MaterialKindOf(', 'uint32_t MeshPassKind('])
fixture = r'''
#include <cassert>
#include <cstdint>
#include <string>
#include <cstdio>
#include "engine/render/gpu_types.h"
namespace pt {
enum class RenderPass:uint8_t {Opaque,Decal,Unlit,Transparent};
struct FmdlMaterial {std::string shader;};
METHODS
struct Sub {RenderPass pass;uint8_t kind=0;bool shadow=false;};
struct Draw {const Sub*sub;};
bool Visible(const Draw&,int){return true;}
bool MainVisible(const Sub&sub){Draw d{&sub};int view=0;auto wanted=RenderPass::Opaque;return SELECTION;}
Sub Load(const char*shader,uint32_t flags){
 FmdlMaterial m{shader};Sub sub;sub.pass=ClassifyMaterial(m);const uint32_t kind=MaterialKindOf(m);
 struct {uint32_t render_flags;}info{flags};ASSIGNMENT
 const uint32_t f=flags;SHADOW
 return sub;
}
}
int main(){using namespace pt;
 for(uint32_t flags:{0x200u,0x400u,0x410u,0x600u,0x680u}){
  auto proxy=Load("fox_3ddf_blin",flags);
  assert(!MainVisible(proxy));assert(proxy.kind==gpu::kKindShadowOnly);
  assert(proxy.shadow); // Geometry is retained for projected shadows.
 }
 for(uint32_t flags:{0u,0x10u,0x20u,0x80u,0x4080u,0x100050u}){
  auto visible=Load("fox_3ddf_blin",flags);assert(MainVisible(visible));
 }
 for(const char*shader:{"fox_3dfw_glass","fox_3dfw_constant","tpp_3dfw_constant","fox_3ddc_decal"}){
  auto ordinary=Load(shader,0),flagged=Load(shader,0x600);
  assert(ordinary.kind==flagged.kind&&ordinary.pass==flagged.pass);
 }
 auto authored=Load("fox_3dndw_shadow",0);assert(!MainVisible(authored)&&authored.shadow);
 // Camera/mirror G-buffer and object-velocity passes both require deferred
 // kind. Preserving pass/kind on forward surfaces keeps their existing route.
 puts("Mesh visibility: hidden opaque proxies stay out of the G-buffer while retaining shadow casting; ordinary, alpha-tested, glass, emissive and decal meshes retain their routes");
}
'''
for key, text in [('METHODS', methods), ('SELECTION', selection), ('ASSIGNMENT', assignment), ('SHADOW', shadow)]:
    fixture = fixture.replace(key, text)
with tempfile.TemporaryDirectory() as directory:
    root = Path(directory)
    cpp = root / 'visibility.cpp'
    cpp.write_text(fixture)
    cmd = [os.environ.get('CXX', 'g++'), '-std=c++20', '-O1']
    cmd += shlex.split(os.environ.get('PT_TEST_CXX_FLAGS', ''))
    cmd += ['-I' + str(repo / path) for path in ['src', 'build/android/_deps/glm-src']]
    subprocess.run(cmd + [str(cpp), '-o', str(root / 'checks')], check=True, timeout=90)
    subprocess.run([str(root / 'checks')], check=True, timeout=15)
