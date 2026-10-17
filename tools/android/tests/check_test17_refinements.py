#!/usr/bin/env python3
"""Check clipping/order/retirement and exact particle output against Test 16."""
from pathlib import Path
import os
import subprocess
import tempfile

repo = Path(__file__).resolve().parents[3]
compiler = [os.environ.get('CXX', 'g++'), '-std=c++20', '-O2', '-pthread',
            '-ffunction-sections', '-fdata-sections', '-Wl,--gc-sections']
includes = ['-I' + str(repo / 'src')] + ['-I' + str(repo / 'build/android/_deps' / name)
            for name in ['glm-src', 'volk-src', 'vulkan_headers-src/include']]
with tempfile.TemporaryDirectory() as directory:
    root = Path(directory)
    out = root / 'policies'
    subprocess.run(compiler + includes + [str(repo / 'tests/mobile_refinements17_test.cpp'), '-o', str(out)], check=True, timeout=120)
    subprocess.run([str(out)], check=True, timeout=30)
    baseline = root / 'baseline.cpp'
    baseline.write_bytes(subprocess.check_output(['git', 'show', 'a6305df:src/engine/vfx/vfx_system.cpp'], cwd=repo))
    headers = root / 'headers/engine/vfx'
    headers.mkdir(parents=True)
    (headers / 'vfx_system.h').write_bytes(subprocess.check_output(['git', 'show', 'a6305df:src/engine/vfx/vfx_system.h'], cwd=repo))
    # Exercise shared nonconstant expression graphs, changing age/random slots,
    # and world-position-dependent inputs in addition to the existing fixtures.
    fixture = (repo / 'tests/particle_build_comparison.cpp').read_text()
    fixture = fixture.replace(' FoldEffectConstants(*def);', '''
 def->exprs.resize(15); def->random_slots=1;
 auto& e=def->exprs;
 e[5].op=ExprOp::Random;e[5].slot=0;
 e[6].op=ExprOp::DragTime;e[6].mode=0;e[6].v0=glm::vec4(1,1,0,0);
 e[7].op=ExprOp::Multiply;e[7].in[0]=5;e[7].in[1]=6;
 e[8].op=ExprOp::Composition;e[8].in[0]=7;e[8].in[1]=7;e[8].v0=glm::vec4(.25f);e[8].v1=glm::vec4(.1f);
 e[9].op=ExprOp::CenterDistRate;e[9].in[0]=3;e[9].v0=glm::vec4(.2f);e[9].v1=glm::vec4(8);e[9].v2=glm::vec4(.2f,1,0,0);
 e[10].op=ExprOp::Multiply;e[10].in[0]=9;e[10].in[1]=8;
 e[11].op=ExprOp::Composition;e[11].in[0]=0;e[11].in[1]=5;
 e[12].op=ExprOp::TimeScale;e[12].in[0]=8;e[12].v0=glm::vec4(.5f);e[12].v1=glm::vec4(.1f,.9f,0,0);
 e[13].op=ExprOp::Keyframe;e[13].in[0]=12;e[13].time=TimeSource::LifeRatio;
 for(auto& curve:e[13].curves){curve.times={0,.25f,.5f,1};curve.values={0,.2f,.1f,.3f};curve.inv_span={4,4,2};}
 e[14].op=ExprOp::Composition;e[14].in[0]=13;e[14].in[1]=10;e[14].v0=glm::vec4(.2f);e[14].v1=glm::vec4(.3f);
 FoldEffectConstants(*def);''')
    fixture = fixture.replace('shape.attr[kPosition]=0;', 'shape.attr[kPosition]=11;')
    fixture = fixture.replace('shape.attr[kRotation]=2;', 'shape.attr[kRotation]=13;')
    fixture = fixture.replace('shape.attr[kColor]=3;', 'shape.attr[kColor]=14;')
    fixture = fixture.replace('state.particles.push_back(p);', 'state.particles.push_back(p);state.slots.push_back(glm::vec4(.01f*(rng()%100),.02f*(rng()%30),.03f*(rng()%20),.5f));')
    expressions = root / 'expressions.cpp'
    expressions.write_text(fixture)
    for source_fixture in [repo / 'tests/particle_build_comparison.cpp', repo / 'tests/particle_mesh_comparison.cpp', expressions]:
        outputs = []
        for name, source in [('baseline', baseline), ('production', repo / 'src/engine/vfx/vfx_system.cpp')]:
            binary = root / (name + source_fixture.stem)
            extra = ['-I' + str(root / 'headers')] if name == 'baseline' else []
            subprocess.run(compiler + extra + includes + [str(source_fixture), str(source),
                str(repo / 'src/engine/vfx/vfx_effect.cpp'), '-o', str(binary)], check=True, timeout=180)
            output = []
            for cull in ['0', '1']:
                output.append(subprocess.check_output([str(binary)], env={**os.environ, 'PT_VFX_CULL': cull}, text=True, timeout=45).strip())
            outputs.append(output)
        assert outputs[0] == outputs[1], (source_fixture.name, outputs)
        print(source_fixture.name, 'exact Test 16 payload/order match with culling off/on:', outputs[1])
