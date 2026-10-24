#!/usr/bin/env python3
"""Run actual texture upload batches against the current residency recording boundary."""
from pathlib import Path
import os,shlex,subprocess,tempfile
repo=Path(__file__).resolve().parents[3]
def extract(s,signature):
 a=s.index(signature);i=s.index('{',a)+1;d=1
 while d:d+=(s[i]=='{')-(s[i]=='}');i+=1
 return s[a:i]
with tempfile.TemporaryDirectory() as directory:
 root=Path(directory);s=(repo/'src/engine/render/texture_manager.cpp').read_text()
 signatures=['uint32_t FormatBlockBytes(','bool TextureManager::SupportsTexture(','void TextureManager::BeginUploadBatch(','void TextureManager::EndUploadBatch(','void TextureManager::FlushUploads(','uint32_t TextureManager::Create(','void TextureManager::WriteResidentDescriptor(','void TextureManager::EvictConverted(']
 body='\n'.join(extract(s,k)for k in signatures)
 boundary=(repo/'tests/texture_residency_recording_test.cpp').read_text().split('int main(){')[0]
 p=root/'upload.cpp';p.write_text(boundary+(repo/'tests/texture_upload_recording_test.cpp').read_text()+'\nnamespace pt{\n'+body+'\n}\n')
 includes=['-I'+str(repo/'src'),'-I'+str(repo/'build/android/_deps/volk-src'),'-I'+str(repo/'build/android/_deps/vulkan_headers-src/include')]
 c=[os.environ.get('CXX','g++')]+shlex.split(os.environ.get('PT_TEST_CXX_FLAGS',''))
 sources=[str(repo/'src/engine/assets'/n)for n in ['bc_texture.cpp','compatible_texture.cpp','ftex.cpp']]
 subprocess.run(c+['-std=c++20','-O1','-pthread','-DVK_NO_PROTOTYPES','-ffunction-sections','-fdata-sections','-Wl,--gc-sections']+includes+[str(p)]+sources+['-lz','-o',str(root/'checks')],check=True,timeout=120)
 subprocess.run([str(root/'checks')],check=True,timeout=30)
