#pragma once
#include <condition_variable>
#include <deque>
#include <memory>
#include <mutex>
#include <thread>
#include <system_error>
namespace pt {
template<class T> class BoundedRetirement {
    std::mutex mutex_; std::condition_variable changed_; std::deque<std::unique_ptr<T>> pending_;
    std::thread worker_; bool stopping_=false;
    void Run() {
        for(;;) {
            std::unique_ptr<T> value;
            { std::unique_lock lock(mutex_); changed_.wait(lock,[&]{return stopping_||!pending_.empty();});
              if(pending_.empty()&&stopping_)return;
              value=std::move(pending_.front());pending_.pop_front(); }
            value.reset();
        }
    }
public:
    BoundedRetirement()=default;
    BoundedRetirement(const BoundedRetirement&)=delete;
    ~BoundedRetirement(){ {std::lock_guard lock(mutex_);stopping_=true;}changed_.notify_one();if(worker_.joinable())worker_.join(); }
    bool Retire(std::unique_ptr<T> value) {
        if(!value)return false;
        std::lock_guard lock(mutex_);
        if(pending_.size()>=2)return false;
        if(!worker_.joinable()) {try {worker_=std::thread([this]{Run();});}catch(const std::system_error&){return false;}}
        pending_.push_back(std::move(value));changed_.notify_one();return true;
    }
};
}
