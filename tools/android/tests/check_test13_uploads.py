#!/usr/bin/env python3
"""Run production upload/residency and renderer-resource methods against a recording Vulkan boundary."""
from pathlib import Path
import os, shlex, subprocess, tempfile

repo=Path(__file__).resolve().parents[3]
def extract(source,signature):
    start=source.index(signature);at=source.index('{',start)+1;depth=1
    while depth:
        depth+=(source[at]=='{')-(source[at]=='}');at+=1
    return source[start:at]
with tempfile.TemporaryDirectory() as directory:
    root=Path(directory)
    source=(repo/'src/engine/render/texture_manager.cpp').read_text()
    names=['uint32_t FormatBlockBytes(','bool TextureManager::SupportsTexture(',
        'void TextureManager::BeginUploadBatch(','void TextureManager::EndUploadBatch(',
        'void TextureManager::FlushUploads(','uint32_t TextureManager::Create(',
        'void TextureManager::WriteResidentDescriptor(','void TextureManager::Touch(',
        'void TextureManager::TouchCube(','void TextureManager::EvictConverted(',
        'void TextureManager::FinishTextureUse(']
    body='\nnamespace pt {\n'+'\n'.join(extract(source,name) for name in names)+'\n}\n'
    fixture=(repo/'tests/texture_residency_recording_test.cpp').read_text()
    includes=['-I'+str(repo/'src'),'-I'+str(repo/'build/android/_deps/volk-src'),'-I'+str(repo/'build/android/_deps/vulkan_headers-src/include')]
    compiler=[os.environ.get('CXX','g++'),'-std=c++20','-O1','-pthread','-DVK_NO_PROTOTYPES','-ffunction-sections','-fdata-sections','-Wl,--gc-sections']+shlex.split(os.environ.get('PT_TEST_CXX_FLAGS',''))
    sources=[str(repo/'src/engine/assets'/name) for name in ['compatible_texture.cpp','bc_texture.cpp','ftex.cpp']]
    for name,text in [('residency',fixture+body)]:
        file=root/(name+'.cpp');file.write_text(text);out=root/name
        subprocess.run(compiler+includes+[str(file)]+sources+['-lz','-o',str(out)],check=True,timeout=120)
        subprocess.run([str(out)],check=True,timeout=30)
    prefix=fixture.split('int main(){')[0]
    prefix=prefix.replace('uint32_t levels,uint32_t layers,VkImageAspectFlags,bool)',
        'uint32_t levels=1,uint32_t layers=1,VkImageAspectFlags=VK_IMAGE_ASPECT_COLOR_BIT,bool=false)')
    prefix=prefix.replace('VkAccessFlags2,VkImageLayout){}','VkAccessFlags2,VkImageLayout,uint32_t=1,uint32_t=1){}')
    prefix+='''
namespace pt {
struct TestRenderer {vk::Context context;vk::Context& Context(){return context;}};
struct SceneRenderer {TestRenderer owner;TestRenderer* renderer_=&owner;VkDevice device_{};
 bool UploadImage(vk::Image&,VkFormat,VkExtent3D,const void*,size_t);
 bool UploadImageMips(vk::Image&,VkFormat,VkExtent3D,const std::vector<std::vector<uint8_t>>&);
};}
int main(){using namespace pt;
 auto cache=std::filesystem::temp_directory_path()/"pt13-resource-recording";std::filesystem::remove_all(cache);
 std::array<bool,16> native{};ConfigureBcConversion(native,cache,0);native_bc=false;
 SceneRenderer scene;vk::Image image;std::vector<uint8_t> white(4,255),red{0,248,0,248,0,0,0,0};
 assert(scene.UploadImage(image,VK_FORMAT_R8G8B8A8_UNORM,{1,1,1},white.data(),white.size()));
 std::vector<std::vector<uint8_t>> mips(3,red);
 assert(scene.UploadImageMips(image,VK_FORMAT_BC1_RGBA_SRGB_BLOCK,{4,4,1},mips));
 assert(records.at(image.image).format==VK_FORMAT_R8G8B8A8_SRGB && records.at(image.image).levels==3);
 for(const auto& level:records.at(image.image).bytes)for(size_t i=0;i<level.size();i+=4)assert(level[i]==255 && level[i+1]==0 && level[i+2]==0 && level[i+3]==255);
 auto original=image.image;auto payload=records.at(original).bytes;
 fail_buffer=true;assert(!scene.UploadImageMips(image,VK_FORMAT_BC1_RGBA_SRGB_BLOCK,{4,4,1},mips));fail_buffer=false;
 assert(image.image==original && records.at(image.image).bytes==payload && records.size()==1);
 fail_submit=true;assert(!scene.UploadImageMips(image,VK_FORMAT_BC1_RGBA_SRGB_BLOCK,{4,4,1},mips));fail_submit=false;
 assert(image.image==original && records.at(image.image).bytes==payload && records.size()==1 && buffers.empty());
 // R8 tail levels need aligned staging offsets, even when a level is one byte.
 std::vector<std::vector<uint8_t>> channel{{1,2,3,4},{5,6},{7}};
 assert(scene.UploadImageMips(image,VK_FORMAT_R8_UNORM,{4,1,1},channel));assert(records.at(image.image).bytes==channel);
 scene.owner.context.DestroyImage(image);assert(records.empty() && buffers.empty());std::filesystem::remove_all(cache);
 puts("Renderer resource checks passed: grain/mask conversion, channels/mips, staging alignment and retained fallback on failures");
}
'''
    renderer=(repo/'src/engine/render/scene_renderer.cpp').read_text()
    text=prefix+body+'\nnamespace pt {\n'+extract(renderer,'bool SceneRenderer::UploadImage(')+'\n'+extract(renderer,'bool SceneRenderer::UploadImageMips(')+'\n}\n'
    file=root/'resources.cpp';file.write_text(text);out=root/'resources'
    subprocess.run(compiler+includes+[str(file)]+sources+['-lz','-o',str(out)],check=True,timeout=120)
    subprocess.run([str(out)],check=True,timeout=30)
