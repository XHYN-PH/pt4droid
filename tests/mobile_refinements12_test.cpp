#include <cassert>
#include <chrono>
#include <cstdio>
#include <random>
#include <glm/glm.hpp>
#include "engine/assets/cpu_preparation_queue.h"
#include "engine/platform/mobile_pacing_guard.h"
#include "engine/platform/mobile_driver_policy.h"
#include "engine/render/mobile_probe_policy.h"

int main() {
    using pt::android::PacingGuard;
    auto run=[](float interval,float present,float gpu,float hz,uint64_t swap) {
        PacingGuard guard;bool fallback=false;
        for(int i=0;i<100;++i)fallback|=guard.Observe(interval,present,60,hz,swap,gpu);
        return fallback;
    };
    // Fixtures reflect the supplied Test 11 paced/native logs; unavailable GPU
    // measurements do not imply spare GPU capacity.
    assert(run(33.16f,25,10,90,22222222));
    assert(run(33.16f,25,10,120,16666666));
    assert(!run(16.67f,.8f,11.5f,90,16666666));
    assert(!run(33,25,27,120,16666666));
    assert(!run(33,25,0,120,16666666));
    assert(run(33,25,0,90,22222222));
    PacingGuard spikes;
    for(int i=0;i<120;++i)assert(!spikes.Observe(i==60?1380:16.67f,i==60?1:.5f,60,120,16666666,8));
    PacingGuard changed;
    for(int i=0;i<50;++i)assert(!changed.Observe(33,25,60,120,16666666,8));
    for(int i=0;i<50;++i)assert(!changed.Observe(16.67f,1,60,60,16666666,8));
    changed.Reset();assert(!changed.Observe(10,1,0,120,0,5));
    assert(!pt::ProbeMapWorthBuilding(0,12));
    assert(!pt::ProbeMapWorthBuilding(8000,1));
    assert(pt::ProbeMapWorthBuilding(200000,12));
    assert(!pt::ProbeMapWorthBuilding(1000000,0));
    using pt::android::BundledTurnipCompatible;
    assert(!BundledTurnipCompatible("Mali-G57"));assert(!BundledTurnipCompatible("Immortalis-G715"));
    assert(!BundledTurnipCompatible("Samsung Xclipse 920"));assert(BundledTurnipCompatible("Adreno (TM) 750"));
    assert(BundledTurnipCompatible(""));
    // Independent numerical comparison for the reordered per-frame fog factor.
    std::mt19937 random(12);std::uniform_real_distribution<float> value(.0001f,3);
    for(int i=0;i<10000;++i) {
        glm::vec3 mie(value(random),value(random),value(random)),ray(value(random),value(random),value(random));
        float gain=value(random),mp=value(random),rp=value(random);glm::vec3 inverse=1.f/(mie+ray);
        const glm::vec3 original=gain*(mie*mp+ray*rp)*inverse;
        const glm::vec3 prepared=(mie*mp+ray*rp)*(gain*inverse);
        assert(glm::all(glm::lessThanEqual(glm::abs(original-prepared),glm::max(glm::vec3(1e-6f),glm::abs(original)*2e-6f))));
    }
    auto load=[](const std::string& path,const std::shared_ptr<pt::TextureReadBudget>& lease)->std::shared_ptr<int> {
        if(path=="bad")throw 1;
        if(!lease->Reserve(8))return {};
        return std::shared_ptr<int>(new int(7),[lease](int* p){delete p;});
    };
    pt::CpuPreparationQueue<int> queue(16,load);
    assert(queue.Enqueue("a"));assert(!queue.Enqueue("a"));auto a=queue.Take("a");assert(a&&*a==7);
    assert(queue.Enqueue("b"));auto b=queue.Take("b");assert(b&&queue.RetainedBytes()==16);
    assert(queue.Enqueue("full"));assert(!queue.Take("full"));
    assert(queue.Enqueue("bad"));assert(!queue.Take("bad"));
    a.reset();b.reset();assert(queue.RetainedBytes()==0);assert(!queue.Take("missing"));queue.Stop();assert(!queue.Enqueue("stopped"));
    std::mutex mutex;std::condition_variable condition;int active=0;bool release=false;
    pt::CpuPreparationQueue<int> bounded(16,[&](const std::string&,const std::shared_ptr<pt::TextureReadBudget>& lease) {
        {std::unique_lock lock(mutex);++active;condition.notify_all();condition.wait(lock,[&]{return release;});}
        return load("ok",lease);
    });
    for(int i=0;i<8;++i)assert(bounded.Enqueue(std::to_string(i)));assert(!bounded.Enqueue("overflow"));
    {std::unique_lock lock(mutex);condition.wait(lock,[&]{return active==2;});}
    auto stopping=std::async(std::launch::async,[&]{bounded.Stop();});
    // Stop cancels queued jobs and joins the two existing readers.
    auto cancelled=bounded.Take("7");assert(!cancelled);
    {std::lock_guard lock(mutex);release=true;}condition.notify_all();stopping.get();
    assert(active==2&&bounded.RetainedBytes()==0);
    puts("Test 12: pacing recovery, refresh changes, Mali driver guard, probe policy, 10000 fog comparisons, bounded CPU preparation/cancellation passed");
}
