#include <bits/stdc++.h>
#include <format>
#define private public
#include "engine/vfx/vfx_system.h"
#undef private
#include "engine/assets/texture_preparation_wave.h"
#include "engine/render/frame_diagnostics.h"
#include "engine/render/shadow_cache.h"
#include "engine/render/light_scissor.h"
#include "engine/core/log.h"
namespace pt { bool LogEnabled(){return false;} void LogWrite(LogLevel,std::string_view){} }
int main() {
    using namespace pt;
    FrameDiagnostics frames;
    assert((frames.Percentiles() == std::array<float,3>{}));
    for(int i=1;i<=100;++i)frames.Add(float(i),{1,2,3,4,5});
    auto q=frames.Percentiles();assert(q[0]==50&&q[1]==95&&q[2]==99&&frames.over_33ms==67);
    assert(frames.Mean(2)==3&&frames.worst_ms==100);
    frames.Add(NAN,{});frames.Add(0,{});assert(frames.samples==100);
    frames.Clear();for(int i=0;i<4096;++i)frames.Add(10,{});assert(frames.Percentiles()[2]==10);
    frames.Clear();assert(frames.samples==0);
    auto pool=std::make_shared<TextureReadBudget>(32);
    auto load=[&](const std::string& path,FtexTexture& data,TextureReadBudget* budget){
        if(!budget->Reserve(8))return false;
        assert(pool->Used()<=32);data.mips={{uint8_t(path[0]),2,3,4,5,6,7,8}};return true;
    };
    auto first=PrepareTextureWave(std::vector<std::string>{"a","b","c","d","e","f"},pool,load);
    assert(pool->Used()==32);size_t kept=0;for(auto& e:first)kept+=bool(e.second);assert(kept==4);
    auto more=PrepareTextureWave(std::vector<std::string>{"g","h"},pool,load);assert(!more[0].second&&!more[1].second);
    for(auto& e:first)e.second.reset();assert(pool->Used()==0);
    more=PrepareTextureWave(std::vector<std::string>{"g","h"},pool,load);assert(pool->Used()==16);
    auto retained=more[0].second;more.clear();assert(pool->Used()==8);pool.reset();retained.reset(); // parent ownership survives the caller
    ShadowTileCache cache;ShadowMapState state;state.casters.push_back({});assert(cache.Miss(state)==ShadowMiss::Cold);
    cache.Recorded(state);assert(cache.Miss(state)==ShadowMiss::None);
    auto changed=state;changed.reusable=false;assert(cache.Miss(changed)==ShadowMiss::Dynamic);
    changed=state;changed.eye.x=1;assert(cache.Miss(changed)==ShadowMiss::Projection);
    changed=state;changed.casters[0].alpha[0]=1;assert(cache.Miss(changed)==ShadowMiss::Casters);
    std::mt19937 rng(891);
    using namespace pt::vfx;
    for(int trial=0;trial<4000;++trial) {
        EffectDef def;def.exprs.resize(20);
        for(int i=0;i<20;++i) {
            auto& e=def.exprs[i];e.v0=glm::vec4(float(rng()%100)/128);e.v1=glm::vec4(.15f);
            e.op=i<3?ExprOp::Const:std::array{ExprOp::Composition,ExprOp::Multiply,ExprOp::UniformVelocityTime,ExprOp::Pass,ExprOp::UniformVelocity,ExprOp::DragTime}[rng()%6];
            e.in[0]=i?int(rng()%i):-1;e.in[1]=i?int(rng()%i):-1;
        }
        System system;System::EvalContext ctx;ctx.def=&def;ctx.age=.3f;ctx.ratio=.25f;
        std::array<glm::vec4,20> before;for(int i=0;i<20;++i)before[i]=system.Eval(ctx,i);
        FoldEffectConstants(def);
        for(int i=0;i<20;++i){auto after=system.Eval(ctx,i);assert(std::memcmp(&after,&before[i],sizeof(after))==0);}
    }
    EffectDef cyclic;cyclic.exprs.resize(1);cyclic.exprs[0].op=ExprOp::Pass;cyclic.exprs[0].in[0]=0;FoldEffectConstants(cyclic);assert(!cyclic.constants[0]);
    const VkExtent2D extent{1200,540};
    glm::mat4 clip(1);clip[2][2]=.2f;clip[3][2]=.5f;
    auto bounds=LightScissor(clip,extent);assert(bounds.extent.width==1200&&bounds.extent.height==540);
    clip[0][0]=.25f;clip[1][1]=.25f;bounds=LightScissor(clip,extent);assert(bounds.extent.width<1200&&bounds.extent.height<540);
    clip[3][2]=2;bounds=LightScissor(clip,extent);assert(bounds.extent.width==0&&bounds.extent.height==0); // the whole volume is beyond the clip plane
    puts("Test 10: frame distributions, bounded/reclaimed wave leases, exact shadow miss classes, 80000 bit-exact expression evaluations and probe bounds passed");
}
