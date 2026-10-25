#pragma once

#include <algorithm>
#include <cstdint>

namespace pt {
// Rendering, simulation and fence waits have already used part of this frame's budget.
// A late frame must continue immediately instead of sleeping for another full period.
inline int64_t FrameLimitWaitNs(int64_t elapsed_ns, int fps_limit) {
    if (fps_limit <= 0) return 0;
    const int64_t period = 1'000'000'000LL / fps_limit;
    return std::max<int64_t>(0, period - std::max<int64_t>(0, elapsed_ns));
}
// Steady deadlines compensate small sleep overshoots without changing the FPS cap.
class FrameDeadline {
public:
    int64_t Wait(int64_t started_ns,int64_t now_ns,int fps_limit) {
        if(fps_limit<=0){Reset();return 0;}
        const int64_t period=1'000'000'000LL/fps_limit;
        if(cap_!=fps_limit || !deadline_ || started_ns<previous_start_ || started_ns-previous_start_>4*period)
            deadline_=started_ns+period;
        else deadline_+=period;
        cap_=fps_limit;previous_start_=started_ns;
        if(now_ns>=deadline_){deadline_=now_ns;return 0;}
        return std::min(period,deadline_-now_ns);
    }
    void Reset(){deadline_=previous_start_=0;cap_=0;}
private:
    int64_t deadline_=0,previous_start_=0;
    int cap_=0;
};

}
