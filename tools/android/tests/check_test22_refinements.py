#!/usr/bin/env python3
"""Execute release policies and source-derived render-pass recordings."""
from pathlib import Path
import os,shlex,subprocess,tempfile
repo=Path(__file__).resolve().parents[3]
def extract(path,sig):
 s=(repo/path).read_text();a=s.index(sig);i=s.index('{',a)+1;d=1
 while d:d+=(s[i]=='{')-(s[i]=='}');i+=1
 return s[a:i]
flags=shlex.split(os.environ.get('PT_TEST_CXX_FLAGS',''))
c=[os.environ.get('CXX','g++'),'-std=c++20','-O1','-pthread']+flags+['-I'+str(repo/p)for p in ['src','build/android/_deps/glm-src','build/android/_deps/vulkan_headers-src/include']]
with tempfile.TemporaryDirectory()as directory:
 root=Path(directory);binary=root/'policies';subprocess.run(c+[str(repo/'tests/mobile_refinements22_test.cpp'),'-o',str(binary)],check=True,timeout=120);subprocess.run([str(binary)],check=True,timeout=30)
 fixture=(repo/'tests/release22_recording_test.cpp.in').read_text()
 methods=[extract('src/engine/render/scene_renderer.cpp','void SceneRenderer::ReleaseDisabledTargets('),extract('src/engine/render/scene_post.cpp','bool SceneRenderer::RecordDepthOfField(')]
 s=(repo/'src/engine/render/upscale/scene_upscale.cpp').read_text();a=s.index('    if(up_.kind==UpscalerKind::Sgsr2',s.index('void SceneRenderer::RecordUpscale('));b=s.index('    up_.reset = false;',a)
 fixture=fixture.replace('/* SGSR_DISPATCH */',s[a:b]).replace('/* METHODS */','\n'.join(methods))
 a=s.index('    const bool warp =',b);b=s.index('    push.f0 =',a);fixture=fixture.replace('/* RESOLVE_IDS */',s[a:b])
 s=(repo/'src/engine/render/scene_frame.cpp').read_text();a=s.index('    if (recorded > 0) {',s.index('void SceneRenderer::RecordEffects('));b=s.index('\n    UseTargets(cmd, {{&output, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL}',a)
 fixture=fixture.replace('/* EFFECT_COMPOSITE */',s[a:b])
 source=root/'recording.cpp';source.write_text(fixture);binary=root/'recording';subprocess.run(c+[str(source),'-o',str(binary)],check=True,timeout=120);subprocess.run([str(binary)],check=True,timeout=30)
