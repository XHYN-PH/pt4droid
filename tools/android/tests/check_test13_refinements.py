#!/usr/bin/env python3
"""Focused Test 13 checks. Configure the Android build first to populate dependencies."""
from pathlib import Path
import os, shlex, subprocess, tempfile

repo=Path(__file__).resolve().parents[3]
flags=shlex.split(os.environ.get('PT_TEST_CXX_FLAGS',''))
compiler=[os.environ.get('CXX','g++'),'-std=c++20','-O1','-pthread','-ffunction-sections','-fdata-sections','-Wl,--gc-sections']+flags
includes=['-I'+str(repo/'src')]+['-I'+str(repo/'build/android/_deps'/name) for name in ['glm-src','volk-src','vulkan_headers-src/include']]
with tempfile.TemporaryDirectory() as directory:
    root=Path(directory)
    cases={
        'conversion':['tests/compat_texture_test.cpp','src/engine/assets/compatible_texture.cpp','src/engine/assets/bc_texture.cpp','src/engine/assets/ftex.cpp'],
        'probe':['tests/probe_transform_test.cpp'],
        'streaming-vfx':['tests/mobile_refinements11_test.cpp','src/engine/vfx/vfx_system.cpp','src/engine/vfx/vfx_effect.cpp','src/engine/assets/ftex.cpp'],
        'pacing-preparation':['tests/mobile_refinements12_test.cpp'],
    }
    for name,sources in cases.items():
        out=root/name
        subprocess.run(compiler+includes+[str(repo/source) for source in sources]+['-lz','-o',str(out)],check=True,timeout=150)
        subprocess.run([str(out)],check=True,timeout=30)
    source=(repo/'src/game/stage_manager.cpp').read_text();start=source.index('    const bool same=');end=source.index(';\n',start)+1
    text=(repo/'tests/collision_reuse_case.cpp.in').read_text().replace('/* PRODUCTION_CONDITION */',source[start:end])
    file=root/'collision.cpp';file.write_text(text);out=root/'collision'
    subprocess.run(compiler+includes+[str(file),'-o',str(out)],check=True,timeout=30)
    subprocess.run([str(out)],check=True,timeout=10)
for check in ['check_test13_uploads.py','check_graphics_menu_update.py','check_mobile_runtime_java.py']:
    subprocess.run([os.environ.get('PYTHON','python3'),str(Path(__file__).parent/check)],check=True,timeout=180)
print('Test 13 focused compatibility, residency, resource, streaming, VFX, collision and Android lifecycle checks passed')
