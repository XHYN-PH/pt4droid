#pragma once

#include <atomic>
#include <cstdint>

namespace pt {

// Reserves retained mip bytes before archive streams are read or inflated.
// Concurrent readers never allocate a mip payload beyond the stage's limit.
class TextureReadBudget {
public:
    explicit TextureReadBudget(uint64_t limit) : limit_(limit) {}

    bool Reserve(uint64_t bytes) {
        uint64_t used = used_.load(std::memory_order_relaxed);
        do {
            if (used > limit_ || bytes > limit_ - used) return false;
        } while (!used_.compare_exchange_weak(used, used + bytes, std::memory_order_relaxed));
        return true;
    }
    uint64_t Used() const { return used_.load(std::memory_order_relaxed); }

private:
    const uint64_t limit_;
    std::atomic<uint64_t> used_{0};
};

}  // namespace pt
