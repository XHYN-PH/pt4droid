#!/usr/bin/env python3
"""Focused policies, real VFX output, Android lifecycle, and Mali shader checks."""
from pathlib import Path
import os, shlex, subprocess, tempfile

repo=Path(__file__).resolve().parents[3]
with tempfile.TemporaryDirectory() as directory:
    output=Path(directory)/'checks'
    compiler=[os.environ.get('CXX','g++'),'-std=c++20','-O2','-pthread']+shlex.split(os.environ.get('PT_TEST_CXX_FLAGS',''))
    subprocess.run(compiler+['-I'+str(repo/'src'),'-I'+str(repo/'build/android/_deps/glm-src'),str(repo/'tests/mobile_refinements12_test.cpp'),'-o',str(output)],check=True,timeout=60)
    subprocess.run([str(output)],check=True,timeout=20)
for check in ['check_test11_refinements.py','check_graphics_menu_update.py','check_mobile_runtime_java.py']:
    subprocess.run([os.environ.get('PYTHON','python3'),str(Path(__file__).parent/check)],check=True,timeout=180)
glslc=Path(os.environ['GLSLC'])
shaders=list((repo/'build/android/shaders').glob('*.spv'))
assert shaders, 'Build pt_shaders before this check'
for path in shaders:
    subprocess.run([str(glslc.parent/'spirv-val'),'--target-env','vulkan1.3',str(path)],check=True,capture_output=True)
for name in ['mesh_clip.vert','shadow_clip.vert','gbuffer_clip.frag','forward_clip.frag','shadow_clip.frag']:
    text=subprocess.check_output([str(glslc.parent/'spirv-dis'),str(repo/'build/android/shaders'/(name+'.spv'))],text=True)
    assert 'OpCapability ClipDistance' not in text and 'BuiltIn ClipDistance' not in text, name
for name in ['mesh.vert','shadow.vert']:
    text=subprocess.check_output([str(glslc.parent/'spirv-dis'),str(repo/'build/android/shaders'/(name+'.spv'))],text=True)
    assert 'OpCapability ClipDistance' in text, name
print(f'Test 12: {len(shaders)} Vulkan 1.3 shader modules validated; native and Mali clipping variants checked')
