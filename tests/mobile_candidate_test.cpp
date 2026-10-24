#include "engine/render/shadow_template_pool.h"
#include "engine/core/parallel_for.h"
#include "engine/core/bounded_retirement.h"
#include "engine/render/dynamic_resolution.h"
#include <cassert>
#include <atomic>
#include <cstdio>
#include <set>
#include <limits>
int main(){
    std::vector<uint64_t> values(4000);std::vector<std::thread::id> ids(4);
    for(unsigned repeat=0;repeat<30;++repeat)pt::ParallelChunks(values.size(),4,[&](size_t c,size_t b,size_t e){ids[c]=std::this_thread::get_id();for(auto i=b;i<e;++i)values[i]=i*i;});
    for(uint64_t i=0;i<values.size();++i)assert(values[i]==i*i);
    std::set<std::thread::id> unique(ids.begin(),ids.end());assert(unique.size()<=4);
    std::atomic<unsigned> nested{0};pt::ParallelChunks(16,4,[&](size_t,size_t b,size_t e){pt::ParallelChunks(e-b,4,[&](size_t,size_t x,size_t y){nested+=unsigned(y-x);});});assert(nested==16);
    std::atomic<unsigned> finished{0};bool caught=false;
    try{pt::ParallelChunks(4000,4,[&](size_t c,size_t,size_t){if(c==0)throw std::runtime_error("expected");++finished;});}catch(const std::runtime_error&){caught=true;}
    assert(caught&&finished==pt::PreparationPool::Get().Workers());
    pt::DynamicResolution scale;uint64_t sample=0;
    assert(!scale.Observe(++sample,std::numeric_limits<float>::quiet_NaN(),15.3f,true));
    for(unsigned i=0;i<8;++i)scale.Observe(++sample,22,15.3f,true);
    assert(scale.Observe(sample,1,15.3f,true)); // Duplicate results cannot shorten hysteresis.
    for(unsigned i=0;i<300;++i)scale.Observe(++sample,6,15.3f,true);
    assert(!scale.Observe(++sample,6,15.3f,true));
    for(unsigned i=0;i<250;++i)scale.Observe(++sample,25,15.3f,true);
    assert(scale.Observe(sample,0,15.3f,true));assert(!scale.Observe(++sample,25,15.3f,false));
    std::array<bool,16> eligible;eligible.fill(true);std::array<uint32_t,16> work{};for(unsigned i=0;i<16;++i)work[i]=i+1;
    for(auto [atlas,tile,count]:{std::tuple{8192u,2048u,4u},std::tuple{4096u,1024u,16u},std::tuple{2048u,512u,16u}}) {
        auto plan=pt::PlanShadowTemplates(atlas,tile,eligible,work);assert(plan.count==count&&uint64_t(plan.width)*plan.width*4<=64ull*1024*1024);
        std::set<std::pair<int,int>> occupied;
        for(unsigned i=0;i<plan.count;++i){auto id=plan.tiles[i];auto o=plan.offsets[id];assert(plan.use[id]&&o.x>=0&&o.y>=0&&o.x+tile<=plan.width&&o.y+tile<=plan.width);assert(occupied.emplace(o.x,o.y).second);}
        if(count==4){for(unsigned i=0;i<12;++i)assert(!plan.use[i]);for(unsigned i=12;i<16;++i)assert(plan.use[i]);}
    }
    assert(pt::PlanShadowTemplates(0,0,eligible,work).count==0);
    puts("Candidate policies: deterministic pooled work, bounded threads, nested work, exception draining and dynamic-resolution hysteresis passed");
}
