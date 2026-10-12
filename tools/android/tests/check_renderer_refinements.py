#!/usr/bin/env python3
"""Execute production refinements against independent references and edge cases."""
from pathlib import Path
import os, shlex, subprocess, tempfile

repo = Path(__file__).resolve().parents[3]
def extract(text, signature):
    start = text.index(signature); pos = text.index('{', start) + 1; depth = 1
    while depth:
        depth += (text[pos] == '{') - (text[pos] == '}'); pos += 1
    return text[start:pos]

with tempfile.TemporaryDirectory() as temp:
    t = Path(temp)
    text = (repo/'src/game/vfx_scene.cpp').read_text()
    production = t/'production.cpp'
    production.write_text('#include <algorithm>\n#include <cmath>\n#include "engine/core/stable_top.h"\n#include "engine/render/scene_lighting.h"\n#include "engine/vfx/vfx_system.h"\nnamespace pt::game {\n' + extract(text,'glm::vec3 ProbeAmbient(') + '\n' + extract(text,'vfx::LightBlock LightBlockAt(') + '\n}\n')
    includes = ['-I'+str(repo/'src'), '-I'+str(repo/'tests'), '-I'+str(repo/'build/android/_deps/glm-src'),
                '-I'+str(repo/'build/android/_deps/vulkan_headers-src/include')]
    compiler = [os.environ.get('CXX','g++')] + shlex.split(os.environ.get('PT_TEST_CXX_FLAGS',''))
    command = compiler + ['-std=c++20','-O2','-DVK_NO_PROTOTYPES'] + includes + [str(repo/'tests/mobile_renderer_refinement_test.cpp'),str(production),str(repo/'src/engine/render/light_cull.cpp'),'-o',str(t/'checks')]
    subprocess.run(command,check=True,timeout=120)
    subprocess.run([str(t/'checks')],check=True,timeout=45)
    frame = (repo/'src/engine/render/scene_frame.cpp').read_text()
    util = (repo/'src/engine/render/render_util.cpp').read_text()
    shadow = t/'shadow.cpp'
    shadow.write_text((repo/'tests/shadow_recording_test.cpp').read_text() + '\nnamespace pt {\n' +
                      extract(util,'void UseTargets(').replace('void UseTargets(', 'static void UseTargets(',1) + '\n' +
                      extract(frame,'void SceneRenderer::InvalidateShadowCache(') + '\n' +
                      extract(frame,'void SceneRenderer::RecordShadows(') + '\n}\n')
    command = compiler + ['-std=c++20','-O2','-DVK_NO_PROTOTYPES'] + includes + [str(shadow),'-o',str(t/'shadow')]
    subprocess.run(command,check=True,timeout=120)
    subprocess.run([str(t/'shadow')],check=True,timeout=15)
    subprocess.run([str(t/'shadow'),'fallback'],check=True,timeout=15)
