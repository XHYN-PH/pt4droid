#include <bits/stdc++.h>
#include <format>
#define private public
#include "engine/vfx/vfx_system.h"
#undef private
#include "engine/assets/texture_preparation_stream.h"
#include "engine/render/frame_diagnostics.h"
#include "engine/platform/mobile_performance.h"
#include "engine/core/log.h"
namespace pt { bool LogEnabled(){return false;} void LogWrite(LogLevel,std::string_view){} }
int main(){using namespace pt;using namespace pt::vfx;
 for(int fps:{0,30,60,144,240})for(int status=0;status<=6;++status)for(bool sustained:{false,true})assert(ThermalFrameLimit(fps,status,sustained)==fps);
 FrameDiagnostics timing;timing.Add(16,std::array<float,8>{1,2,3,4,5,6,7,8});assert(timing.Mean(5)==6&&timing.Mean(6)==7&&timing.Mean(7)==8);
 std::mt19937 random(114);System system;System::Instance inst;ViewInfo view;
 for(int trial=0;trial<80;++trial){
  EffectDef def;def.all_frame=120;def.exprs.resize(30);
  for(int i=0;i<30;++i){auto&e=def.exprs[i];e.v0=glm::vec4(.37f);e.v1=glm::vec4(.73f);e.in[0]=i?int(random()%i):-1;e.in[1]=i?int(random()%i):-1;
   e.op=std::array{ExprOp::Const,ExprOp::DragTime,ExprOp::Random,ExprOp::UniformVelocity,ExprOp::UniformAccel,ExprOp::UniformVelocityTime,ExprOp::Composition,ExprOp::Multiply,ExprOp::Oscillate,ExprOp::Pass,ExprOp::TimeScale,ExprOp::CameraFollow,ExprOp::CenterScroll,ExprOp::CameraAngle,ExprOp::Receive}[random()%15];
   e.mode=random()%3;e.slot=0;e.flags=random()%2;
  }
  FoldEffectConstants(def);std::vector<glm::vec4> values(def.exprs.size());std::vector<uint32_t> ready(def.exprs.size());glm::vec4 slot(.4f);
  System::EvalContext ctx;ctx.def=&def;ctx.inst=&inst;ctx.view=&view;ctx.effect_time=.7f;ctx.effect_ratio=.35f;ctx.slots=&slot;ctx.frame_values=values.data();ctx.frame_ready=ready.data();
  for(int particle=0;particle<100;++particle){ctx.age=particle/117.f;ctx.ratio=particle/107.f;ctx.life=1.31f;ctx.world[3]=glm::vec4(particle*.031f,0,0,1);slot.x=particle*.17f;
   auto original=ctx;original.frame_values=nullptr;original.frame_ready=nullptr;
   for(int i=0;i<30;++i){auto a=system.Eval(original,i),b=system.Eval(ctx,i);assert(std::memcmp(&a,&b,sizeof(a))==0);}
  }
 }
 // Exercise actual per-shape generations, including wrap and an effect switch.
 auto epoch_def=std::make_shared<EffectDef>();epoch_def->all_frame=120;epoch_def->exprs.resize(1);epoch_def->frame_uniform={1};
 epoch_def->exprs[0].op=ExprOp::DragTime;epoch_def->exprs[0].mode=2;epoch_def->exprs[0].v0=glm::vec4(1);
 System::Instance epoch_inst;epoch_inst.def=epoch_def;epoch_inst.frame=60;
 ShapeDef shape;shape.kind=ShapeKind::PointLight;shape.attr[kColor]=0;
 System::ShapeState state;state.particles.emplace_back();RenderList output;std::vector<LightOut> lights;
 system.frame_epoch_=UINT32_MAX;system.frame_ready_={1};system.frame_values_={glm::vec4(99)};
 system.BuildShape(epoch_inst,glm::mat4(1),shape,state,view,{},{},{},output,lights);
 assert(system.frame_epoch_==1&&lights.size()==1&&lights[0].color==glm::vec3(1));
 epoch_inst.frame=120;lights.clear();system.BuildShape(epoch_inst,glm::mat4(1),shape,state,view,{},{},{},output,lights);
 assert(system.frame_epoch_==2&&lights[0].color==glm::vec3(2));
 epoch_inst.def=std::make_shared<EffectDef>(*epoch_def);epoch_inst.frame=30;lights.clear();
 system.BuildShape(epoch_inst,glm::mat4(1),shape,state,view,{},{},{},output,lights);assert(lights[0].color==glm::vec3(.5f));
 // Reused particle expressions invalidate at particle boundaries and epoch
 // wrap, independently of the frame-uniform cache.
 auto shared=std::make_shared<EffectDef>();shared->exprs.resize(2);
 shared->exprs[0].op=ExprOp::DragTime;shared->exprs[0].mode=0;shared->exprs[0].v0=glm::vec4(1);
 shared->exprs[1].op=ExprOp::Composition;shared->exprs[1].in[0]=0;shared->exprs[1].in[1]=0;
 FoldEffectConstants(*shared);assert(shared->has_particle_shared&&shared->particle_shared[0]);
 epoch_inst.def=shared;shape.attr[kColor]=1;state.particles.resize(2);
 state.particles[0].age=.5f;state.particles[1].age=.25f;
 system.particle_epoch_=UINT32_MAX;system.particle_ready_={1,1};system.particle_values_={glm::vec4(99),glm::vec4(99)};
 lights.clear();system.BuildShape(epoch_inst,glm::mat4(1),shape,state,view,{},{},{},output,lights);
 assert(system.particle_epoch_==2&&lights.size()==2&&lights[0].color==glm::vec3(1)&&lights[1].color==glm::vec3(.5f));
 state.particles[0].age=.125f;state.particles[1].age=.75f;lights.clear();
 system.BuildShape(epoch_inst,glm::mat4(1),shape,state,view,{},{},{},output,lights);
 assert(lights.size()==2&&lights[0].color==glm::vec3(.25f)&&lights[1].color==glm::vec3(1.5f));
 // A slow texture cannot hold a completed unrelated texture hostage. Use
 // explicit barriers instead of relying on scheduling or sleep thresholds.
 auto budget=std::make_shared<TextureReadBudget>(16);
 std::mutex mutex;std::condition_variable condition;bool release=false,entered=false;
 TexturePreparationStream stream(budget,[&](const std::string& path,FtexTexture& tex,TextureReadBudget* lease){
  if(path=="slow"){std::unique_lock lock(mutex);entered=true;condition.notify_all();condition.wait(lock,[&]{return release;});}
  if(path=="bad")return false;
  if(!lease->Reserve(8))return false;tex.mips={{uint8_t(path[0])}};return true;
 });
 auto slow=stream.Enqueue("slow");{std::unique_lock lock(mutex);condition.wait(lock,[&]{return entered;});}
 auto fast=stream.Enqueue("fast");assert(fast.wait_for(std::chrono::seconds(2))==std::future_status::ready);assert(slow.wait_for(std::chrono::seconds(0))!=std::future_status::ready);
 auto result=fast.get();assert(result.texture&&budget->Used()==8);fast={};result.texture.reset();
 auto drain=stream.Enqueue("bad");drain.wait();drain={};assert(budget->Used()==0);
 auto other=stream.Enqueue("other");auto kept=other.get().texture;other={};assert(budget->Used()==8);
 auto fill=stream.Enqueue("fill");auto second=fill.get().texture;fill={};assert(budget->Used()==16);
 auto refused=stream.Enqueue("full");stream.Prioritize("full");assert(refused.get().failure==TexturePreparationFailure::Budget);refused={};
 auto bad=stream.Enqueue("bad");assert(bad.get().failure==TexturePreparationFailure::Read);bad={};
 second.reset();kept.reset();assert(budget->Used()==0);
 {std::lock_guard lock(mutex);release=true;}condition.notify_all();assert(slow.get().texture);slow={};stream.Stop();assert(budget->Used()==0);
 // Both readers blocked: prioritize the next job, cancel queued futures and join running readers.
 std::vector<std::string> order;bool unblock_one=false,unblock_all=false;
 TexturePreparationStream priority(budget,[&](const std::string& path,FtexTexture&,TextureReadBudget*){
  std::unique_lock lock(mutex);order.push_back(path);condition.notify_all();
  condition.wait(lock,[&]{return unblock_all||(path=="one"&&unblock_one);});return true;
 });
 auto one=priority.Enqueue("one"),two=priority.Enqueue("two");
 {std::unique_lock lock(mutex);condition.wait(lock,[&]{return order.size()==2;});}
 auto a=priority.Enqueue("a"),b=priority.Enqueue("b"),needed=priority.Enqueue("needed");priority.Prioritize("needed");
 {std::lock_guard lock(mutex);unblock_one=true;}condition.notify_all();
 {std::unique_lock lock(mutex);condition.wait(lock,[&]{return order.size()==3;});assert(order[2]=="needed");}
 auto stopping=std::async(std::launch::async,[&]{priority.Stop();});
 assert(a.get().failure==TexturePreparationFailure::Cancelled&&b.get().failure==TexturePreparationFailure::Cancelled);
 {std::lock_guard lock(mutex);unblock_all=true;}condition.notify_all();stopping.get();
 assert(one.get().texture&&two.get().failure==TexturePreparationFailure::Cancelled&&needed.get().failure==TexturePreparationFailure::Cancelled);
 puts("Test 11: 240000 bit-exact cached/live VFX evaluations; generation wrap/effect switches; individual readiness, budget reclaim/fallbacks; split diagnostics and sustained FPS preservation passed");
}
