#pragma once
#include "engine/assets/texture_read_budget.h"
#include <algorithm>
#include <condition_variable>
#include <deque>
#include <functional>
#include <future>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

namespace pt {
// CPU-only preparation. At most eight results and two readers; payload leases
// share one byte budget and are released when a result is consumed or dropped.
template<class T> class CpuPreparationQueue {
public:
    using Load = std::function<std::shared_ptr<T>(const std::string&, const std::shared_ptr<TextureReadBudget>&)>;
    CpuPreparationQueue(uint64_t bytes, Load load) : budget_(std::make_shared<TextureReadBudget>(bytes)), load_(std::move(load)) {
        try { for(int i=0;i<2;++i)workers_.emplace_back([this]{Run();}); }
        catch(...) { Stop(); throw; }
    }
    ~CpuPreparationQueue(){Stop();}
    bool Enqueue(const std::string& path) {
        std::lock_guard lock(mutex_);
        // A missing optional model must not fill the queue for the session.
        std::erase_if(pending_,[](const auto& item){return item.second.wait_for(std::chrono::seconds(0))==std::future_status::ready&&!item.second.get();});
        if(stopping_ || pending_.contains(path) || pending_.size()>=8)return false;
        Job job{path,{}};pending_.emplace(path,job.promise.get_future().share());jobs_.push_back(std::move(job));
        ready_.notify_one();return true;
    }
    std::shared_ptr<T> Take(const std::string& path) {
        std::shared_future<std::shared_ptr<T>> result;
        {
            std::lock_guard lock(mutex_);auto it=pending_.find(path);if(it==pending_.end())return {};
            result=std::move(it->second);pending_.erase(it);
            auto job=std::find_if(jobs_.begin(),jobs_.end(),[&](const Job& j){return j.path==path;});
            if(job!=jobs_.end()){Job next=std::move(*job);jobs_.erase(job);jobs_.push_front(std::move(next));}
        }
        return result.get();
    }
    void Stop() {
        {std::lock_guard lock(mutex_);stopping_=true;for(auto& job:jobs_)job.promise.set_value({});jobs_.clear();}
        ready_.notify_all();for(auto& worker:workers_)if(worker.joinable())worker.join();workers_.clear();
        {std::lock_guard lock(mutex_);pending_.clear();}
    }
    uint64_t RetainedBytes() const { return budget_->Used(); }
private:
    struct Job {std::string path;std::promise<std::shared_ptr<T>> promise;};
    void Run() {
        for(;;) {
            Job job;
            {std::unique_lock lock(mutex_);ready_.wait(lock,[&]{return stopping_||!jobs_.empty();});if(stopping_)return;job=std::move(jobs_.front());jobs_.pop_front();}
            std::shared_ptr<T> value;
            try {auto lease=std::make_shared<TextureReadBudget>(0,budget_);value=load_(job.path,lease);}
            catch(...) {}
            job.promise.set_value(std::move(value));
        }
    }
    std::shared_ptr<TextureReadBudget> budget_;Load load_;
    std::mutex mutex_;std::condition_variable ready_;std::deque<Job> jobs_;
    std::unordered_map<std::string,std::shared_future<std::shared_ptr<T>>> pending_;
    std::vector<std::thread> workers_;bool stopping_=false;
};
}
