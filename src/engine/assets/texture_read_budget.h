#pragma once

#include <atomic>
#include <cstdint>
#include <memory>
#include <condition_variable>
#include <mutex>
#include <utility>

namespace pt {
struct TextureBudgetSignal {
    std::mutex mutex;
    std::condition_variable ready;
    uint64_t generation=0;
};

// Reserves retained mip bytes before archive streams are read or inflated.
// Concurrent readers never allocate a mip payload beyond the stage's limit.
class TextureReadBudget {
public:
    explicit TextureReadBudget(uint64_t limit, std::shared_ptr<TextureReadBudget> parent = {}) : limit_(limit), parent_(std::move(parent)), signal_(parent_?parent_->Signal():std::make_shared<TextureBudgetSignal>()) {}
    ~TextureReadBudget() { if (parent_) parent_->Release(Used()); }

    bool Reserve(uint64_t bytes) {
        if (parent_) {
            if (!parent_->Reserve(bytes)) { rejected_.fetch_add(1,std::memory_order_relaxed); return false; }
            used_.fetch_add(bytes, std::memory_order_relaxed);
            return true;
        }
        uint64_t used = used_.load(std::memory_order_relaxed);
        do {
            if (used > limit_ || bytes > limit_ - used) { rejected_.fetch_add(1,std::memory_order_relaxed); return false; }
        } while (!used_.compare_exchange_weak(used, used + bytes, std::memory_order_relaxed));
        return true;
    }
    void Release(uint64_t bytes) {
        used_.fetch_sub(bytes, std::memory_order_relaxed);
        if(parent_){parent_->Release(bytes);return;}
        if(bytes){std::lock_guard lock(signal_->mutex);++signal_->generation;signal_->ready.notify_all();}
    }
    std::shared_ptr<TextureBudgetSignal> Signal()const{return signal_;}
    uint64_t Used() const { return used_.load(std::memory_order_relaxed); }
    uint64_t Rejected() const { return rejected_.load(std::memory_order_relaxed); }

private:
    const uint64_t limit_;
    std::shared_ptr<TextureReadBudget> parent_;
    std::shared_ptr<TextureBudgetSignal> signal_;
    std::atomic<uint64_t> used_{0};
    std::atomic<uint64_t> rejected_{0};
};

}  // namespace pt
