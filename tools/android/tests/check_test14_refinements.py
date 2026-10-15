#!/usr/bin/env python3
"""Run real policies and compare mesh particles against the Test 13 implementation."""
from pathlib import Path
import os, shlex, subprocess, tempfile

repo=Path(__file__).resolve().parents[3]
flags=shlex.split(os.environ.get('PT_TEST_CXX_FLAGS',''))
compiler=[os.environ.get('CXX','g++'),'-std=c++20','-O2','-pthread','-ffunction-sections','-fdata-sections','-Wl,--gc-sections']+flags
includes=['-I'+str(repo/'src'),'-I'+str(repo/'build/android/_deps/glm-src')]
with tempfile.TemporaryDirectory() as directory:
    root=Path(directory)
    out=root/'policies'
    subprocess.run(compiler+includes+[str(repo/'tests/mobile_refinements14_test.cpp'),'-o',str(out)],check=True,timeout=90)
    subprocess.run([str(out)],check=True,timeout=20)
    baseline=root/'baseline.cpp'
    baseline.write_bytes(subprocess.check_output(['git','show','4791923:src/engine/vfx/vfx_system.cpp'],cwd=repo))
    headers=root/'headers/engine/vfx';headers.mkdir(parents=True)
    (headers/'vfx_system.h').write_bytes(subprocess.check_output(['git','show','4791923:src/engine/vfx/vfx_system.h'],cwd=repo))
    results={}
    for name,source in [('baseline',baseline),('production',repo/'src/engine/vfx/vfx_system.cpp')]:
        active=compiler+(['-I'+str(root/'headers')] if name=='baseline' else [])+includes
        output=root/name
        subprocess.run(active+[str(repo/'tests/particle_mesh_comparison.cpp'),str(source),str(repo/'src/engine/vfx/vfx_effect.cpp'),'-o',str(output)],check=True,timeout=150)
        results[name]=[]
        for cull in ['0','1']:
            env=os.environ.copy();env['PT_VFX_CULL']=cull
            results[name].append(subprocess.check_output([str(output)],env=env,text=True,timeout=30).strip())
    assert results['baseline']==results['production'],results
    print('1500 mixed mesh/sprite/liquid/lit particle cases: exact Test 13 payload and draw order with culling on/off',results['production'])
for name in ['check_graphics_menu_update.py','check_menu_exit.py','check_settings_actions.py']:
    subprocess.run([os.environ.get('PYTHON','python3'),str(Path(__file__).parent/name)],check=True,timeout=180)
