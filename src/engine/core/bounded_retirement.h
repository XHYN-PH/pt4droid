#pragma once
#include <chrono>
#include <future>
#include <memory>
#include <system_error>

namespace pt {
// At most one retired packet is destroyed off-thread. If that worker is still
// busy, destroy the next packet locally instead of growing a queue or waiting.
// The future joins at owner destruction, so no detached cleanup outlives it.
template<class T>
class BoundedRetirement {
public:
    bool Retire(std::unique_ptr<T> value) {
        if (!value || (worker_.valid() && worker_.wait_for(std::chrono::seconds(0)) != std::future_status::ready)) return false;
        if (worker_.valid()) worker_.get();
        try {
            worker_ = std::async(std::launch::async, [retired = std::move(value)]() mutable { retired.reset(); });
            return true;
        } catch (const std::system_error&) {
            return false;
        }
    }
private:
    std::future<void> worker_;
};
}
