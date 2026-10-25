#include "engine/render/shadow_template_pool.h"
#include "engine/render/light_scissor.h"
#include "engine/render/frame_diagnostics.h"
#include "engine/platform/mobile_pacing_guard.h"
#include "engine/platform/mobile_performance.h"
#include "engine/core/log.h"
#include <glm/gtc/matrix_transform.hpp>
#include <cassert>
#include <random>
#include <set>
#include <cstdio>
namespace pt {bool LogEnabled(){return true;}}
int main(){
    using namespace pt;
    std::array<bool,16> eligible{};std::array<uint32_t,16> savings{};
    std::array<int32_t,16> owners;owners.fill(-1);
    for(unsigned i=0;i<4;++i){eligible[i]=true;savings[i]=100+i;owners[i]=int32_t(i);}
    std::mt19937 rng(24067);
    for(int frame=0;frame<2000;++frame){
        for(unsigned i=0;i<4;++i)savings[i]=32+rng()%200;
        auto p=PlanShadowTemplates(8192,2048,eligible,savings,&owners);
        assert(p.count==4);
        for(unsigned i=0;i<4;++i){assert(p.use[i]);assert(p.offsets[i].x==int32_t(i%2*2048)&&p.offsets[i].y==int32_t(i/2*2048));}
    }
    eligible[0]=false;
    auto partial=PlanShadowTemplates(8192,2048,eligible,savings,&owners);
    assert(partial.count==3&&partial.offsets[3].x==2048&&partial.offsets[3].y==2048);
    eligible[4]=true;savings[4]=1000;
    auto replacement=PlanShadowTemplates(8192,2048,eligible,savings,&owners);
    assert(replacement.count==4&&replacement.offsets[4].x==0&&replacement.offsets[4].y==0);
    assert(replacement.offsets[1].x==2048&&replacement.offsets[1].y==0);
    std::uniform_real_distribution<float> u(-1,1);unsigned covered=0,tightened=0;
    const VkExtent2D display{2400,1080};
    for(int trial=0;trial<3000;++trial){
        auto camera=glm::perspectiveRH_ZO(1.1f,2400.f/1080,.05f,100.f);
        const glm::vec3 center(u(rng)*5,u(rng)*3,-3-u(rng)*2);
        const float radius=1.5f+std::abs(u(rng))*2;
        auto radial=glm::translate(glm::mat4(1),center)*glm::scale(glm::mat4(1),glm::vec3(radius));
        auto box=glm::translate(glm::mat4(1),center+glm::vec3(u(rng),u(rng),u(rng)))*
            glm::rotate(glm::mat4(1),u(rng)*3,glm::normalize(glm::vec3(.3f,.7f,.2f)))*
            glm::scale(glm::mat4(1),glm::vec3(.1f+std::abs(u(rng))*3,.1f+std::abs(u(rng)),.1f+std::abs(u(rng))*2));
        auto a=LightScissor(camera*radial,display),b=LightScissor(camera*box,display);
        auto bounds=IntersectLightScissors(a,b);
        tightened+=uint64_t(bounds.extent.width)*bounds.extent.height<uint64_t(a.extent.width)*a.extent.height;
        for(int sample=0;sample<40;++sample){
            const auto world=box*glm::vec4(u(rng),u(rng),u(rng),1);
            if(glm::length(glm::vec3(world)-center)>radius)continue;
            const auto clip=camera*world;
            if(clip.w<=0||clip.z<0||clip.z>clip.w)continue;
            const auto pixel=(glm::vec2(clip)/clip.w*.5f+.5f)*glm::vec2(display.width,display.height);
            if(pixel.x<0||pixel.y<0||pixel.x>=display.width||pixel.y>=display.height)continue;
            assert(pixel.x>=bounds.offset.x&&pixel.y>=bounds.offset.y);
            assert(pixel.x<=bounds.offset.x+bounds.extent.width&&pixel.y<=bounds.offset.y+bounds.extent.height);++covered;
        }
    }
    assert(covered>10000&&tightened>1000);
    assert(IntersectLightScissors({{0,0},{4,4}},{{8,8},{4,4}}).extent.width==0);
    FrameDiagnostics frames;frames.Add(16.667f,std::array<float,5>{});frames.AddLoading(1522.78f);
    frames.Add(40.f,std::array<float,5>{});assert(frames.samples==2&&frames.over_33ms==1);
    assert(frames.Percentiles()[2]==40.f&&frames.loading_intervals==1&&frames.loading_worst_ms>1500);
    frames.Clear();assert(!frames.samples&&!frames.loading_intervals);
    for(float hz:{60.f,90.f,120.f,144.f}){
        android::PacingGuard guard;
        for(int i=0;i<600;++i)assert(!guard.Observe(i==250?1522.78f:16.667f,i==250?1000.f:1.f,60,hz,16666666,10));
        guard.Reset(); // Reentering after a pause must not inherit a bad window.
        for(int i=0;i<60;++i)assert(!guard.Observe(16.667f,1.f,60,hz,16666666,10));
    }
    assert(std::string_view(ThermalStatusName(3))=="Severe");
    for(int status=0;status<7;++status)assert(ThermalFrameLimit(90,status,true)==90);
    LogSetVerbose(false);assert(!LogAccept(LogLevel::Debug,"debug"));assert(!LogAccept(LogLevel::Info,"sound: tick"));
    assert(LogAccept(LogLevel::Info,"frame pacing: summary"));assert(LogAccept(LogLevel::Warn,"sound: missing"));
    assert(LogAccept(LogLevel::Error,"demo: error"));LogSetVerbose(true);assert(LogAccept(LogLevel::Debug,"debug"));
    printf("Test24: 2000 stable shadow-slot plans, %u conservative light samples, %u tighter bounds, loading separation, fixed-60 pacing and normal/verbose logging passed\n",covered,tightened);
}
