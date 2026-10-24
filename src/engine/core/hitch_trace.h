#pragma once
#include <array>
#include <algorithm>
#include <utility>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <mutex>
namespace pt::hitch {
struct Event { const char* name=nullptr; uint64_t frame=0; double milliseconds=0; };
struct Recorder { std::atomic<bool> enabled{false}; std::atomic<uint64_t> frame{0}; std::mutex mutex;
    std::array<Event,128> events{}; uint64_t written=0; };
inline Recorder& Get() { static Recorder recorder; return recorder; }
inline void Enable(bool enabled) { Get().enabled.store(enabled,std::memory_order_relaxed); }
inline void Frame(uint64_t frame) { Get().frame.store(frame,std::memory_order_relaxed); }
class Scope {
    const char* name_; bool active_; uint64_t frame_; std::chrono::steady_clock::time_point started_;
public:
    explicit Scope(const char* name):name_(name),active_(Get().enabled.load(std::memory_order_relaxed)),
        frame_(active_?Get().frame.load(std::memory_order_relaxed):0) { if(active_)started_=std::chrono::steady_clock::now(); }
    ~Scope() { if(!active_)return;
        const double ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-started_).count();
        if(ms<1.0)return;auto& r=Get();std::lock_guard lock(r.mutex);r.events[r.written++%r.events.size()]={name_,frame_,ms}; }
};
inline std::array<Event,8> Slowest(uint64_t& cursor) {
    auto& r=Get();std::lock_guard lock(r.mutex);std::array<Event,8> out{};
    const auto begin=cursor>r.written?0:cursor;const auto retained=r.written>r.events.size()?r.written-r.events.size():0;
    for(auto i=std::max(begin,retained);i<r.written;++i) {auto value=r.events[i%r.events.size()];
        for(auto& slot:out)if(value.milliseconds>slot.milliseconds)std::swap(value,slot);}
    cursor=r.written;return out;
}
}
