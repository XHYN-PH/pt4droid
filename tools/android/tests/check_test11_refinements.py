#!/usr/bin/env python3
"""Exercise production diagnostics, preparation leases, VFX folding and shadow decisions."""
from pathlib import Path
import os, shlex, subprocess, tempfile
repo=Path(__file__).resolve().parents[3]
with tempfile.TemporaryDirectory() as directory:
    output=Path(directory)/'refinements'
    command=[os.environ.get('CXX','g++'),'-std=c++20','-O2','-pthread','-ffunction-sections','-fdata-sections','-Wl,--gc-sections']+shlex.split(os.environ.get('PT_TEST_CXX_FLAGS',''))
    command+=['-I'+str(repo/'src'),'-I'+str(repo/'build/android/_deps/glm-src'),'-I'+str(repo/'build/android/_deps/volk-src'),'-I'+str(repo/'build/android/_deps/vulkan_headers-src/include')]
    command+=[str(repo/'tests/mobile_refinements11_test.cpp'),str(repo/'src/engine/vfx/vfx_system.cpp'),str(repo/'src/engine/vfx/vfx_effect.cpp'),str(repo/'src/engine/assets/ftex.cpp'),'-o',str(output)]
    subprocess.run(command,check=True,timeout=180)
    subprocess.run([str(output)],check=True,timeout=45)
