#!/usr/bin/env python3
"""Run the production queue/retirement bodies against explicit fence states."""
from pathlib import Path
import os,shlex,subprocess,tempfile
repo=Path(__file__).resolve().parents[3]
def extract(s,signature):
 a=s.index(signature);i=s.index('{',a)+1;d=1
 while d:d+=(s[i]=='{')-(s[i]=='}');i+=1
 return s[a:i]
with tempfile.TemporaryDirectory() as directory:
 t=Path(directory);s=(repo/'src/engine/render/vk_context.cpp').read_text()
 body='\n'.join(extract(s,k)for k in ['void Context::RetireTextureUploads(','bool Context::SubmitTextureUpload('])
 p=t/'async.cpp';p.write_text((repo/'tests/async_texture_upload_test.cpp').read_text()+'\nnamespace pt::vk {\n'+body+'\n}\n')
 c=[os.environ.get('CXX','g++')]+shlex.split(os.environ.get('PT_TEST_CXX_FLAGS',''))
 subprocess.run(c+['-std=c++20','-O1','-DVK_NO_PROTOTYPES','-I'+str(repo/'build/android/_deps/vulkan_headers-src/include'),str(p),'-o',str(t/'check')],check=True,timeout=60)
 subprocess.run([str(t/'check')],check=True,timeout=15)
