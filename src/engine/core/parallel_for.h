#pragma once
#include <algorithm>
#include <condition_variable>
#include <cstddef>
#include <deque>
#include <exception>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>
namespace pt {
class PreparationPool {
    std::mutex mutex_;std::condition_variable changed_;std::deque<std::function<void()>> jobs_;
    std::vector<std::thread> workers_;bool stopping_=false;
    inline static thread_local bool worker_thread_=false;
public:
    PreparationPool(){
        const unsigned n=std::min(3u,std::max(1u,std::thread::hardware_concurrency())-1);
        try {for(unsigned i=0;i<n;++i)workers_.emplace_back([this]{worker_thread_=true;
            for(;;){std::function<void()> job;{std::unique_lock lock(mutex_);changed_.wait(lock,[&]{return stopping_||!jobs_.empty();});
                if(jobs_.empty()&&stopping_)return;job=std::move(jobs_.front());jobs_.pop_front();}job();}});
        }catch(const std::system_error&){} // A smaller pool or inline fallback remains valid.
    }
    ~PreparationPool(){{std::lock_guard lock(mutex_);stopping_=true;}changed_.notify_all();for(auto& w:workers_)w.join();}
    static PreparationPool& Get(){static PreparationPool pool;return pool;}
    static bool IsWorker(){return worker_thread_;}
    size_t Workers()const{return workers_.size();}
    void Submit(std::function<void()> job){
        {std::lock_guard lock(mutex_);if(!workers_.empty()&&jobs_.size()<16){jobs_.push_back(std::move(job));changed_.notify_one();return;}}
        job();
    }
};
template<typename Fn> void ParallelChunks(size_t count,size_t chunks,Fn&& fn){
    if(chunks<=1||count<2||PreparationPool::IsWorker()){fn(size_t{0},size_t{0},count);return;}
    auto& pool=PreparationPool::Get();chunks=std::max<size_t>(1,std::min({chunks,count,pool.Workers()+1}));
    std::vector<std::future<void>> futures;futures.reserve(chunks-1);
    std::exception_ptr error;
    try {
        for(size_t c=1;c<chunks;++c){auto task=std::make_shared<std::packaged_task<void()>>([&fn,c,count,chunks]{fn(c,count*c/chunks,count*(c+1)/chunks);});futures.push_back(task->get_future());pool.Submit([task]{(*task)();});}
        fn(size_t{0},size_t{0},count/chunks);
    } catch(...) {error=std::current_exception();}
    // Dispatch can fail after earlier jobs captured fn. Drain every submitted
    // job before propagating either dispatch or callback errors to the caller.
    for(auto& future:futures)try{future.get();}catch(...){if(!error)error=std::current_exception();}
    if(error)std::rethrow_exception(error);
}
inline size_t ParallelChunkCount(size_t count,size_t minimum){return std::max<size_t>(1,std::min<size_t>({std::max(1u,std::thread::hardware_concurrency()),size_t{4},count/std::max<size_t>(minimum,1)}));}
}
