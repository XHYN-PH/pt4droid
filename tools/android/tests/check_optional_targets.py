#!/usr/bin/env python3
from pathlib import Path
import os,shlex,subprocess,tempfile
repo=Path(__file__).resolve().parents[3];source=(repo/'src/engine/render/scene_renderer.cpp').read_text();start=source.index('bool SceneRenderer::EnsureOptionalTargets(');pos=source.index('{',start)+1;depth=1
while depth:depth+=(source[pos]=='{')-(source[pos]=='}');pos+=1
with tempfile.TemporaryDirectory() as directory:
 root=Path(directory);p=root/'targets.cpp';p.write_text((repo/'tests/optional_targets_test.cpp').read_text()+'\nnamespace pt{\n'+source[start:pos]+'\n}\n');flags=shlex.split(os.environ.get('PT_TEST_CXX_FLAGS',''))
 subprocess.run([os.environ.get('CXX','g++'),'-std=c++20','-O2','-DVK_NO_PROTOTYPES','-I'+str(repo/'build/android/_deps/vulkan_headers-src/include')]+flags+[str(p),'-o',str(root/'checks')],check=True,timeout=60);subprocess.run([str(root/'checks')],check=True,timeout=20)
