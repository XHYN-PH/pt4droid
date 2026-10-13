#!/usr/bin/env python3
from pathlib import Path
import os,shlex,subprocess,tempfile
repo=Path(__file__).resolve().parents[3];compiler=[os.environ.get('CXX','g++'),'-std=c++20','-O2','-ffunction-sections','-fdata-sections','-Wl,--gc-sections']+shlex.split(os.environ.get('PT_TEST_CXX_FLAGS',''))+['-I'+str(repo/'src'),'-I'+str(repo/'build/android/_deps/glm-src')]
with tempfile.TemporaryDirectory() as directory:
 root=Path(directory);baseline=root/'baseline.cpp';baseline.write_bytes(subprocess.check_output(['git','show','pt4droid-v0.2.1-test.7:src/engine/vfx/vfx_system.cpp'],cwd=repo));results=[]
 for name,source in [('baseline',baseline),('production',repo/'src/engine/vfx/vfx_system.cpp')]:
  out=root/name;subprocess.run(compiler+[str(repo/'tests/particle_build_comparison.cpp'),str(source),str(repo/'src/engine/vfx/vfx_effect.cpp'),'-o',str(out)],check=True,timeout=120);env=os.environ.copy();env['PT_VFX_CULL']='0';results.append(subprocess.check_output([str(out)],env=env,text=True,timeout=20).strip())
 assert results[0]==results[1],results
 print('1500 mixed particle shapes: exact payload and alpha-order baseline match',results[0])
