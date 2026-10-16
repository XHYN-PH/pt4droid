#pragma once
#include "engine/assets/ftex.h"
#include <algorithm>
#include <condition_variable>
#include <deque>
#include <functional>
#include <future>
#include <mutex>
#include <thread>
#include <unordered_set>

namespace pt {
enum class TexturePreparationFailure { None, Budget, Read, Cancelled };
struct PreparedTextureResult {
    std::shared_ptr<const FtexTexture> texture;
    TexturePreparationFailure failure=TexturePreparationFailure::None;
};
// Two readers publish each texture independently. The queue keeps promises,
// never copies of finished payloads, so consumption releases its budget lease.
class TexturePreparationStream {
public:
    using Load=std::function<bool(const std::string&,FtexTexture&,TextureReadBudget*)>;
    TexturePreparationStream(std::shared_ptr<TextureReadBudget> budget, Load load) : budget_(std::move(budget)),load_(std::move(load)),wake_(budget_->Signal()) {
        try { for (int i=0;i<2;++i) workers_.emplace_back([this]{Run();}); }
        catch (...) { Stop(); throw; }
    }
    ~TexturePreparationStream(){Stop();}
    std::shared_future<PreparedTextureResult> Enqueue(std::string path) {
        Job job{std::move(path),{}};auto future=job.promise.get_future().share();
        { std::lock_guard lock(wake_->mutex); if(stopping_) {job.promise.set_value({{},TexturePreparationFailure::Cancelled});return future;} jobs_.push_back(std::move(job)); }
        wake_->ready.notify_one();return future;
    }
    void Prioritize(const std::string& path) {
        {std::lock_guard lock(wake_->mutex);
        demanded_.insert(path);
        auto it=std::find_if(jobs_.begin(),jobs_.end(),[&](const Job& job){return FtexStem(job.path)==path;});
        if(it!=jobs_.end()&&it!=jobs_.begin()){Job job=std::move(*it);jobs_.erase(it);jobs_.push_front(std::move(job));}}
        wake_->ready.notify_all();
    }
    void Stop() {
        {std::lock_guard lock(wake_->mutex);stopping_=true;for(auto& job:jobs_)job.promise.set_value({{},TexturePreparationFailure::Cancelled});jobs_.clear();}
        wake_->ready.notify_all();for(auto& worker:workers_)if(worker.joinable())worker.join();workers_.clear();
    }
private:
    struct Job {std::string path;std::promise<PreparedTextureResult> promise;unsigned retries=0;bool deferred=false;uint64_t generation=0;};
    void Run() {
        for(;;) {
            Job job;
            {
                std::unique_lock lock(wake_->mutex);
                auto runnable=[&](const Job& candidate){return !candidate.deferred || candidate.generation!=wake_->generation || demanded_.contains(FtexStem(candidate.path));};
                wake_->ready.wait(lock,[&]{return stopping_||std::any_of(jobs_.begin(),jobs_.end(),runnable);});
                if(stopping_)return;
                auto it=std::find_if(jobs_.begin(),jobs_.end(),runnable);job=std::move(*it);jobs_.erase(it);
            }
            PreparedTextureResult result;
            try {
                auto lease=std::make_shared<TextureReadBudget>(96ull*1024*1024,budget_);
                auto texture=std::shared_ptr<FtexTexture>(new FtexTexture,[lease](FtexTexture* p){delete p;});
                if(load_(job.path,*texture,lease.get()))result.texture=std::move(texture);
                else result.failure=lease->Rejected()?TexturePreparationFailure::Budget:TexturePreparationFailure::Read;
            } catch(...) {result.failure=TexturePreparationFailure::Read;}
            {
                std::lock_guard lock(wake_->mutex);
                if(stopping_){/* Publish after releasing the wake lock. */}
                else if(result.failure==TexturePreparationFailure::Budget && job.retries<2 && budget_->Used()>0 && !demanded_.contains(FtexStem(job.path))){
                    ++job.retries;job.deferred=true;job.generation=wake_->generation;jobs_.push_back(std::move(job));continue;
                }
            }
            if(stopping_.load(std::memory_order_relaxed))result={{},TexturePreparationFailure::Cancelled};
            job.promise.set_value(std::move(result));
        }
    }
    std::shared_ptr<TextureReadBudget> budget_;Load load_;
    std::shared_ptr<TextureBudgetSignal> wake_;std::deque<Job> jobs_;std::unordered_set<std::string> demanded_;std::vector<std::thread> workers_;std::atomic<bool> stopping_{false};
};
}
