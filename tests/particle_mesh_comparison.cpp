#include <bits/stdc++.h>
#include <format>
#define private public
#include "engine/vfx/vfx_system.h"
#undef private
#include "engine/core/log.h"
namespace pt{bool LogEnabled(){return false;}void LogWrite(LogLevel,std::string_view){}}
int main(){using namespace pt::vfx;std::mt19937 rng(13077);uint64_t hash=1469598103934665603ull;auto bytes=[&](const void*d,size_t n){for(size_t i=0;i<n;++i){hash^=static_cast<const uint8_t*>(d)[i];hash*=1099511628211ull;}};
 for(int trial=0;trial<1500;++trial){System s;System::Instance inst;System::ShapeState state;auto def=std::make_shared<EffectDef>();inst.def=def;def->name="synthetic";def->exprs.resize(5);for(auto&e:def->exprs)e.op=ExprOp::Const;def->exprs[0].v0=glm::vec4(0);def->exprs[1].v0=glm::vec4(.1f+.01f*(trial%17),.2f,1,0);def->exprs[2].v0=glm::vec4(.1f,.2f,.3f,0);def->exprs[3].v0=glm::vec4(.15f,.7f,.3f,.8f);def->exprs[4].v0=glm::vec4(.25f,.125f,.5f,.5f);
 FoldEffectConstants(*def);
 ShapeDef shape;shape.attr[kPosition]=0;shape.attr[kScale]=1;shape.attr[kRotation]=2;shape.attr[kColor]=3;shape.attr[kUv]=4;shape.kind=trial%3==0?ShapeKind::Plane:trial%3==1?ShapeKind::Sprite:ShapeKind::SpriteRot;shape.local_space=trial%2==0;shape.sort_mode=1;shape.separate_draw=trial%2==0;
 if(trial%5==0){shape.kind=ShapeKind::Model;shape.model=42;shape.model_uv=trial%2;shape.invert_face=trial%3==0;auto mesh=std::make_shared<ModelMesh>();mesh->positions={{-.2f,-.1f,0},{.2f,-.1f,0},{0,.3f,0},{0,0,.5f}};mesh->uvs={{0,0},{1,0},{.5f,1},{.5f,.5f}};mesh->indices={0,1,2,0,2,3,0,3,1,1,3,2};s.models_[42]=mesh;}
 auto&m=shape.material;m.kind=trial%4==0?MaterialKind::Scroll:trial%4==1?MaterialKind::Lit:trial%4==2?MaterialKind::Liquid:MaterialKind::DynamicLuminance;m.blend=BlendMode::Alpha;m.camera_z_offset=trial%2?.025f:0.f;m.texture="diffuse";m.rain_texture="rain";m.rain_angle=glm::vec2(.21f,.64f);m.rain_swing=glm::vec2(.15f,.01f);m.texture_angle=.35f;m.texture_scale=glm::vec2(.4f,.6f);m.rain_scale=glm::vec2(1.3f,1.7f);state.rain_phase=glm::vec2(.04f*trial,.05f*trial);state.rain_offset=glm::vec2(.123f,.321f);
 for(uint32_t i=0;i<40+rng()%100;++i){System::Particle p;p.life=4;p.age=.01f*i;p.spawn_world[3]=glm::vec4(.01f*int(rng()%120),.005f*int(rng()%80),-.02f*int(rng()%150),1);state.particles.push_back(p);}
 s.SetLightBlockProbe([](const glm::vec3& c,const glm::vec3& h){LightBlock b;b.sky=glm::vec3(.5f)+glm::abs(c)*.01f;b.directional=h*.07f;b.count=1;b.position[0]=c;b.color[0]=h;b.inv_r4[0]=.3f;return b;});ViewInfo view;view.position=glm::vec3(.5f,.25f,2);view.forward=glm::vec3(0,0,-1);RenderList list;std::vector<LightOut> lights;
 glm::mat4 world(1);world[0].x=1.17f;world[1].y=.83f;world[3]=glm::vec4(.21f,.37f,-.15f,1.0f);
 s.BuildShape(inst,world,shape,state,view,[](const std::string&n){return n=="rain"?92u:17u;},{},[](const glm::vec3&p,bool){return LightingSample{glm::vec3(.4f),glm::abs(p)*.02f,glm::vec3(.01f)};},list,lights);bytes(&trial,sizeof(trial));
 for(const auto&q:list.quads){bytes(q.corner,sizeof(q.corner));bytes(&q.uv,sizeof(q.uv));bytes(&q.uv_next,sizeof(q.uv_next));bytes(&q.color,sizeof(q.color));bytes(&q.params,sizeof(q.params));bytes(&q.luminance,sizeof(q.luminance));bytes(&q.extra,sizeof(q.extra));bytes(&q.info,sizeof(q.info));bytes(&q.rain_rotation,sizeof(q.rain_rotation));bytes(&q.light_factors,sizeof(q.light_factors));bytes(q.light_position,sizeof(q.light_position));bytes(q.light_color,sizeof(q.light_color));}}
 printf("%016llx\n",static_cast<unsigned long long>(hash));}
