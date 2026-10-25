from pathlib import Path
import subprocess,tempfile
repo=Path(__file__).resolve().parents[3]
s=(repo/'src/game/game_objects.cpp').read_text()
def extract(start):
 a=s.index(start);i=s.index('{',a)+1;depth=1
 while depth:depth+=(s[i]=='{')-(s[i]=='}');i+=1
 return s[a:i]
stage=extract('    if (command.id == "StageLight")')
enable=extract('void GameObjects::SetGimmickEnabled(')
sync=extract('void SyncCeilLamp(')
fixture=r'''
#include <cassert>
#include <array>
#include <cstdint>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <vector>
#include <cstdio>
#include <functional>
enum class GimmickType{Baby,Ocho,CeilLamp,Freezer,Bag};
constexpr uint64_t kStrCode64Mask=0xffffffffffffffffull;
uint64_t StrCode64(const char*name){return std::hash<std::string>{}(name);}
struct Gimmick{GimmickType type=GimmickType::CeilLamp;bool lights[3]={};bool stage_light_set=false,stage_light=false,stage_light_red=false,freezer_strong=false,shown=false,enabled=false;float freezer_timer=0;std::set<uint64_t>hidden_meshes;std::vector<uint32_t>sound_handles;int hidden_views=0;};
struct Cmd{std::string id="StageLight";std::map<std::string,bool>args;std::optional<bool>Bool(const char*k)const{auto p=args.find(k);return p==args.end()?std::nullopt:std::optional(p->second);}};
struct Mock{Mock&Demos(){return *this;}Mock&Gimmicks(){return *this;}void StopSounds(GimmickType){}Mock*Audio(){return nullptr;}void StopPlayingId(uint32_t,float){}};
SYNC
struct GameObjects{Mock game_;std::array<Gimmick,5>gimmicks_;void SetGimmickEnabled(GimmickType,bool);bool Stage(Gimmick&gimmick,const Cmd&command){STAGE return false;}};
ENABLE
int main(){GameObjects o;auto&g=o.gimmicks_[size_t(GimmickType::CeilLamp)];
 o.SetGimmickEnabled(GimmickType::CeilLamp,true);assert(g.lights[0]&&g.lights[1]&&!g.lights[2]);
 const auto unrelated=StrCode64("unrelated");g.hidden_meshes.insert(unrelated);
 for(bool on:{false,true})for(bool red:{false,true}){
  Cmd command;command.args={{"switch",on},{"isRed",red}};o.Stage(g,command);
  for(int repeat=0;repeat<3;repeat++){
   o.SetGimmickEnabled(GimmickType::CeilLamp,true);
   assert(g.lights[0]&&g.lights[1]==(on&&!red)&&g.lights[2]==(on&&red));
   assert(g.hidden_meshes.contains(StrCode64("MESH_off"))==on);
   assert(g.hidden_meshes.contains(StrCode64("MESH_on_ST"))==(!on||red));
   assert(g.hidden_meshes.contains(StrCode64("MESH_on_red_ST"))==(!on||!red));
   assert(g.hidden_meshes.contains(unrelated));
   o.SetGimmickEnabled(GimmickType::CeilLamp,false);assert(!g.lights[0]&&!g.lights[1]&&!g.lights[2]);
  }
 }
 // StageLight selects white/red effects; it must not restart the independently expired glass effect.
 o.SetGimmickEnabled(GimmickType::CeilLamp,true);g.lights[0]=false;
 Cmd white;white.args={{"switch",true},{"isRed",false}};o.Stage(g,white);assert(!g.lights[0]&&g.lights[1]&&!g.lights[2]);
 o.SetGimmickEnabled(GimmickType::CeilLamp,false);
 // Stage commands while disabled select the model without activating hidden light effects.
 Cmd command;command.args={{"switch",true},{"isRed",true}};o.Stage(g,command);assert(!g.lights[1]&&!g.lights[2]);
 o.SetGimmickEnabled(GimmickType::CeilLamp,true);assert(!g.lights[1]&&g.lights[2]);
 puts("Lamp state sequences passed: initial white, repeated enable, red/off, disable/re-enable, hidden groups preserved.");
}
'''.replace('SYNC',sync).replace('STAGE',stage).replace('ENABLE',enable)
with tempfile.TemporaryDirectory()as t:
 p=Path(t);(p/'state.cpp').write_text(fixture)
 subprocess.run(['g++','-std=c++20','-O1',str(p/'state.cpp'),'-o',str(p/'check')],check=True)
 subprocess.run([str(p/'check')],check=True)
