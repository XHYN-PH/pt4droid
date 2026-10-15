#!/usr/bin/env python3
"""Execute production logging, draw-state cache, timing policy, presets and pending settings."""
from pathlib import Path
import os, subprocess, tempfile, shlex

repo=Path(__file__).resolve().parents[3]
def method(path, signature):
    text=(repo/path).read_text();a=text.index(signature);i=text.index('{',a)+1;depth=1
    while depth:
        if text[i]=='{':depth+=1
        elif text[i]=='}':depth-=1
        i+=1
    return text[a:i]

timing=method('src/engine/render/scene_renderer.cpp','bool SceneRenderer::EnsureTimingQueries(')
change=method('src/game/ui/pc_settings_page.cpp','void PcSettingsPage::Change(')
row=method('src/main.cpp','static pt::game::PcSettingRow Row(')
sections=method('src/main.cpp','std::vector<pt::game::PcSettingSection> GraphicsSections() const')
return_menu_action=method('src/main.cpp','if(id==kReturnToMenu)')
prefix=r'''#include <cassert>
#include <fstream>
#include <iostream>
#include <thread>
#include <atomic>
#include <algorithm>
#include "engine/core/log.h"
#include "engine/platform/os.h"
#include "engine/platform/mobile_driver_policy.h"
#include "engine/platform/graphics_presets.h"
#include "engine/platform/mobile_graphics.h"
#include "engine/render/mesh_draw_state.h"
#include "game/ui/pc_settings.h"
int formatted=0;
struct Cost {};
template<> struct std::formatter<Cost>:std::formatter<std::string_view> {
 auto format(const Cost&,std::format_context& context)const {formatted++;return std::formatter<std::string_view>::format("cost",context);}
};
namespace pt::os {FILE* OpenFile(const std::filesystem::path& p,const char* mode){return std::fopen(p.c_str(),mode);}}
int culls=0,fronts=0,vertices=0,indices=0,creates=0,fail_at=0;
VKAPI_ATTR void VKAPI_CALL Cull(VkCommandBuffer,VkCullModeFlags){culls++;}
VKAPI_ATTR void VKAPI_CALL Front(VkCommandBuffer,VkFrontFace){fronts++;}
VKAPI_ATTR void VKAPI_CALL Vertex(VkCommandBuffer,uint32_t first,uint32_t count,const VkBuffer*,const VkDeviceSize* offsets){assert(first==0 && count==1 && *offsets==0);vertices++;}
VKAPI_ATTR void VKAPI_CALL Index(VkCommandBuffer,VkBuffer,VkDeviceSize offset,VkIndexType type){assert(offset==0 && type==VK_INDEX_TYPE_UINT32);indices++;}
VKAPI_ATTR VkResult VKAPI_CALL Query(VkDevice,const VkQueryPoolCreateInfo* info,const VkAllocationCallbacks*,VkQueryPool* out){
 creates++;assert(info->queryType==VK_QUERY_TYPE_TIMESTAMP && (info->queryCount==12 || info->queryCount==4));
 *out=reinterpret_cast<VkQueryPool>(uintptr_t(creates));return fail_at==creates?VK_ERROR_OUT_OF_DEVICE_MEMORY:VK_SUCCESS;
}
PFN_vkCmdSetCullMode vkCmdSetCullMode=Cull;PFN_vkCmdSetFrontFace vkCmdSetFrontFace=Front;
PFN_vkCmdBindVertexBuffers vkCmdBindVertexBuffers=Vertex;PFN_vkCmdBindIndexBuffer vkCmdBindIndexBuffer=Index;
PFN_vkCreateQueryPool vkCreateQueryPool=Query;
struct MockContext{uint32_t timestamp_bits=64;};struct MockRenderer{MockContext ctx;MockContext& Context(){return ctx;}};
using pt::LogEnabled;
struct FrameSlot{VkQueryPool queries=VK_NULL_HANDLE,upscale_queries=VK_NULL_HANDLE;};
struct SceneRenderer {bool timing_failed_=false;MockRenderer data;MockRenderer* renderer_=&data;VkDevice device_=VK_NULL_HANDLE;static constexpr uint32_t kTimestamps=12;bool EnsureTimingQueries(FrameSlot& slot);};
struct AudioSink{void PostEvent(const char*,void*){}};struct Game{std::vector<int> steps;AudioSink* Audio(){return nullptr;}void RequestOptionsSave(){steps.push_back(1);}void RequestQuit(){steps.push_back(3);}};
constexpr const char* kCursorSound="cursor";constexpr const char* kChangeSound="change";
using pt::game::PcSettingRow;
struct PresetSource:pt::game::PcSettingsSource {
 struct Scene{bool RayTracingSupported(std::string&)const{return false;}};
 struct App{pt::AppSettings settings;Scene scene;}app_;
 pt::AppSettings& settings=app_.settings;PcSettingRow row;int set_calls=0;static constexpr int kGraphicsPreset=1;
 PresetSource(){settings.graphics.shadow_quality=0;Refresh();}
 void Refresh(){row=GraphicsSections()[0].rows[0];}
 std::vector<pt::game::PcSettingSection> LightingSections()const{return {pt::game::PcSettingSection{}};}
 std::vector<pt::game::PcSettingSection> TextureSections()const{return {pt::game::PcSettingSection{}};}
 std::vector<pt::game::PcSettingSection> EffectSections()const{return {pt::game::PcSettingSection{},pt::game::PcSettingSection{}};}
 ROW_METHOD
 SECTIONS_METHOD
 std::vector<pt::game::PcSettingSection> Sections()override{return {};}
 void Set(int,int value)override{pt::ApplyGraphicsPreset(settings,static_cast<pt::GraphicsPreset>(value),false);set_calls++;Refresh();}
 Game* game_=nullptr;static constexpr int kReturnToMenu=2;
 void Save(){game_->steps.push_back(2);}
 void Activate(int id)override{auto& game_=*this->game_;RETURN_MENU_ACTION}
};
struct PcSettingsPage {
 PresetSource data;PresetSource* source_=&data;int cursor_=0,preview_id_=-1,preview_value_=0;bool confirming_=false;float flash_frame_=0;std::vector<int> rows_{0};
 const PcSettingRow* CurrentRow(){return &data.row;}int ShownValue(const PcSettingRow& r){return preview_id_==r.id?preview_value_:r.value;}
 void SourceChanged(){}void Scroll(){}void Refresh(){data.Refresh();}void Change(Game&,int,bool);
};
'''
body=r'''
int main(int argc,char** argv){
 assert(argc==2);auto path=std::filesystem::path(argv[1]);assert(!pt::LogEnabled());
 pt::LogSetFile(path);pt::LogInfo("{}",Cost{});assert(!std::filesystem::exists(path) && formatted==0);
 pt::LogSetEnabled(true);pt::LogSetFile(path);pt::LogInfo("on {}",Cost{});assert(formatted==1 && std::filesystem::exists(path));
 pt::LogSetEnabled(false);const auto bytes=std::filesystem::file_size(path);pt::LogInfo("{}",Cost{});pt::LogWrite(pt::LogLevel::Error,"off");assert(formatted==1 && std::filesystem::file_size(path)==bytes);
 pt::LogSetEnabled(true);pt::LogSetFile(path,true);pt::LogInfo("reopened");pt::LogSetEnabled(false);std::ifstream in(path);std::string text((std::istreambuf_iterator<char>(in)),{});assert(text.find("on cost")!=std::string::npos && text.find("reopened")!=std::string::npos && text.find("off")==std::string::npos);
 pt::MeshDrawState state;VkCommandBuffer cmd=reinterpret_cast<VkCommandBuffer>(uintptr_t(1));VkBuffer v=reinterpret_cast<VkBuffer>(uintptr_t(1)),i=reinterpret_cast<VkBuffer>(uintptr_t(2));
 for(int n=0;n<10000;n++)state.Bind(cmd,v,i,VK_CULL_MODE_BACK_BIT,VK_FRONT_FACE_COUNTER_CLOCKWISE);
 assert(culls==1 && fronts==1 && vertices==1 && indices==1);
 state.Bind(cmd,reinterpret_cast<VkBuffer>(uintptr_t(3)),i,VK_CULL_MODE_BACK_BIT,VK_FRONT_FACE_COUNTER_CLOCKWISE);assert(vertices==2 && indices==1);
 state.Bind(cmd,v,reinterpret_cast<VkBuffer>(uintptr_t(4)),VK_CULL_MODE_NONE,VK_FRONT_FACE_CLOCKWISE);assert(culls==2 && fronts==2 && vertices==3 && indices==2);
 state.Reset();state.Bind(cmd,v,i,VK_CULL_MODE_NONE,VK_FRONT_FACE_CLOCKWISE);assert(culls==3 && fronts==3 && vertices==4 && indices==3);
 SceneRenderer scene;FrameSlot slot;
 for(int n=0;n<10000;n++)assert(!scene.EnsureTimingQueries(slot));assert(creates==0);
 pt::LogSetEnabled(true);scene.data.ctx.timestamp_bits=0;assert(!scene.EnsureTimingQueries(slot) && creates==0);scene.data.ctx.timestamp_bits=64;
 for(int n=0;n<10000;n++)assert(scene.EnsureTimingQueries(slot));assert(creates==2);
 pt::LogSetEnabled(false);assert(!scene.EnsureTimingQueries(slot) && creates==2);
 pt::LogSetEnabled(true);SceneRenderer failed;FrameSlot failed_slot;fail_at=creates+1;assert(!failed.EnsureTimingQueries(failed_slot) && failed_slot.queries==VK_NULL_HANDLE);const int stop=creates;assert(!failed.EnsureTimingQueries(failed_slot) && creates==stop);
 SceneRenderer partial;FrameSlot partial_slot;fail_at=creates+2;assert(!partial.EnsureTimingQueries(partial_slot) && partial_slot.queries && !partial_slot.upscale_queries);assert(!partial.EnsureTimingQueries(partial_slot));
 pt::LogSetEnabled(false);
 using pt::android::PreferBundledTurnip;
 for(auto name:{"Adreno (TM) 610","QUALCOMM Adreno 650","Adreno (TM) 730","Adreno 740","adreno 799"})assert(PreferBundledTurnip(name));
 for(auto name:{"Adreno 530","Adreno 830","Adreno 840","Mali-G715","Immortalis-G720","PowerVR","Adreno",""})assert(!PreferBundledTurnip(name));
 PcSettingsPage page;Game game;assert(page.data.row.value==4);page.Change(game,1,true);assert(page.data.row.value==0 && page.data.set_calls==1);page.Change(game,1,true);assert(page.data.row.value==1 && page.data.set_calls==2);
 for(int n=0;n<100;n++){page.Change(game,1,true);page.Change(game,-1,false);}assert(page.data.row.value==1);
 page.data.row.wrap=false;page.data.row.value=4;const int calls=page.data.set_calls;page.Change(game,1,true);assert(page.data.set_calls==calls);
 page.data.row.id=PresetSource::kReturnToMenu;page.data.row.action=true;page.data.game_=&game;
 page.Change(game,1,false);assert(game.steps.empty() && !page.confirming_);
 page.Change(game,1,true);assert(game.steps.empty() && page.confirming_);
 page.Change(game,1,true);assert((game.steps==std::vector<int>{1,2,3}) && !page.confirming_);
 auto graphics_path=path.parent_path()/"graphics.ini";
 pt::AppSettings settings;settings.audio.volume=1.7f;settings.progress.loops_reached=123;settings.progress.archive="keep";settings.input.mouse_sensitivity=2.5f;settings.display.width=2400;
 auto requested=settings;requested.audio.volume=.1f;requested.progress.archive="discard";requested.display.width=320;
 requested.graphics.shadow_quality=0;requested.graphics.anisotropy=16;requested.graphics.film_grain=.37f;requested.graphics.clarity=.81f;requested.graphics.enhanced_textures=true;
 requested.camera.third_person=true;requested.ray_tracing.shadows=2;requested.ray_tracing.contact_shadows=true;requested.display.fps_limit=120;requested.display.letterbox=2;requested.display.vsync=false;
 assert(pt::SaveAppSettings(graphics_path,requested));assert(pt::android::ApplyGameGraphicsOverrides(graphics_path,settings,true));
 assert(settings.graphics.shadow_quality==0 && settings.graphics.anisotropy==16 && settings.graphics.film_grain==.37f && settings.graphics.clarity==.81f && !settings.graphics.enhanced_textures);
 assert(settings.camera.third_person);
 assert(settings.ray_tracing.shadows==2 && settings.ray_tracing.contact_shadows && settings.display.fps_limit==120 && settings.display.letterbox==2 && !settings.display.vsync);
 assert(settings.audio.volume==1.7f && settings.progress.loops_reached==123 && settings.progress.archive=="keep" && settings.input.mouse_sensitivity==2.5f && settings.display.width==2400);
 assert(pt::android::ApplyGameGraphicsOverrides(graphics_path,settings,false));assert(settings.ray_tracing==pt::AppSettings::RayTracing{});
 assert(!pt::android::ApplyGameGraphicsOverrides(graphics_path.parent_path()/"missing.ini",settings,true));
 auto roundtrip=path.parent_path()/"roundtrip.ini";assert(pt::SaveAppSettings(roundtrip,settings));pt::AppSettings loaded;assert(pt::LoadAppSettings(roundtrip,loaded));assert(loaded.graphics==settings.graphics && loaded.progress==settings.progress);
 std::puts("Native checks passed: log Off/On and formatting; 10000 cached draws; lazy/unsupported/failed timing; 13 GPU policies; Custom to PS4 cycling; confirmed Return to menu");
}
'''
options='\n'.join(method('src/engine/platform/android_options.cpp',s) for s in ['std::filesystem::path PrivateFile(', 'float ReadNumber(', 'void Load()', 'int ConsumePerformanceProfile()', 'bool ConsumeGameGraphicsChanges()', 'std::filesystem::path GameGraphicsChangesFile()'])
options_prefix=r'''#include <cassert>
#include <atomic>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <cmath>
#include <mutex>
#include <cstdlib>
#include <iostream>
std::string storage;const char* SDL_GetAndroidInternalStoragePath(){return storage.c_str();}
std::filesystem::path LogDir(){return storage;}
std::atomic<float> scale{.667f};std::atomic<int> profile{-1},bloom{6};std::atomic<bool> graphics_changed{false};std::once_flag loaded;
'''
options_body=r'''
int main(int argc,char** argv){assert(argc==3);storage=argv[1];std::string mode=argv[2];auto pending=std::filesystem::path(storage)/"performance_profile_pending.txt";
 std::ofstream(std::filesystem::path(storage)/"performance_profile.txt")<<"1";
 if(mode=="pending")std::ofstream(pending)<<"3";if(mode=="corrupt")std::ofstream(pending)<<"1e30";if(mode=="nan")std::ofstream(pending)<<"nan";
 std::ofstream(std::filesystem::path(storage)/"render_scale.txt")<<"nan";
 const auto graphics=std::filesystem::path(storage)/"game_graphics_pending.ini",applying=std::filesystem::path(storage)/"game_graphics_applying.ini";
 if(mode=="graphics")std::ofstream(graphics)<<"first";if(mode=="recovered")std::ofstream(applying)<<"recovered";
 const int expected=(mode=="pending" || mode=="corrupt")?3:-1;
 assert(ConsumePerformanceProfile()==expected);assert(ConsumePerformanceProfile()==-1);assert(scale==.667f);
 if(expected>=0)assert(!std::filesystem::exists(pending));
 if(mode=="graphics" || mode=="recovered"){
  assert(ConsumeGameGraphicsChanges() && std::filesystem::exists(GameGraphicsChangesFile()));assert(!ConsumeGameGraphicsChanges());
  std::ofstream(graphics)<<"newer";graphics_changed=true;std::filesystem::remove(applying);
  assert(std::filesystem::exists(graphics));assert(ConsumeGameGraphicsChanges());std::ifstream input(GameGraphicsChangesFile());std::string text;input>>text;assert(text=="newer");
 }else assert(!ConsumeGameGraphicsChanges());
 std::puts("Pending profile check passed");}
'''
with tempfile.TemporaryDirectory() as tmp:
    t=Path(tmp);flags=[os.environ.get('CXX','g++')]+shlex.split(os.environ.get('PT_TEST_CXX_FLAGS',''))+['-std=c++20','-pthread','-I'+str(repo/'src')]
    for folder in ['vulkan_headers-src/include','volk-src','vma-src/include']:flags+=['-I'+str(repo/'build/android/_deps'/folder)]
    file=t/'native.cpp';file.write_text(prefix.replace('RETURN_MENU_ACTION',return_menu_action).replace('ROW_METHOD',row).replace('SECTIONS_METHOD',sections)+timing+'\n'+change+body)
    subprocess.run(flags+['-D__ANDROID__','-c',str(repo/'src/engine/platform/settings.cpp'),'-o',str(t/'settings.o')],check=True,timeout=60)
    subprocess.run(flags+[str(file),str(repo/'src/engine/core/log.cpp'),str(repo/'src/engine/platform/graphics_presets.cpp'),str(t/'settings.o'),'-o',str(t/'native')],check=True,timeout=60)
    env=os.environ.copy();env['PT_LOGS']='0'
    subprocess.run([str(t/'native'),str(t/'native.log')],check=True,env=env,timeout=15)
    file=t/'pending.cpp';file.write_text(options_prefix+options+options_body)
    subprocess.run(flags+[str(file),'-o',str(t/'pending')],check=True,timeout=30)
    for name in ['fresh','pending','corrupt','nan','graphics','recovered']:
        folder=t/('case-'+name);folder.mkdir();subprocess.run([str(t/'pending'),str(folder),name],check=True,timeout=5)
    print('Profile/graphics persistence checks passed: pending applied once; old override ignored; invalid values bounded; updates retained during application; interrupted graphics recovered')
