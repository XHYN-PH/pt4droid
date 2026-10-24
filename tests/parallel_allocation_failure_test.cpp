#include "engine/core/parallel_for.h"
#include <atomic>
#include <cassert>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <new>
thread_local int allocation_countdown=-1;
std::atomic<bool> dispatch_failed{false};
void* operator new(std::size_t n) {
    if(allocation_countdown>0 && --allocation_countdown==0){allocation_countdown=-1;dispatch_failed=true;throw std::bad_alloc();}
    if(void* p=std::malloc(n?n:1))return p;throw std::bad_alloc();
}
void operator delete(void* p)noexcept{std::free(p);}
void operator delete(void* p,std::size_t)noexcept{std::free(p);}
int main(){
    auto& pool=pt::PreparationPool::Get();if(!pool.Workers())return 2;
    bool escaped=false;unsigned dispatch_failures=0;
    for(int fault=1;fault<=24;++fault){
        dispatch_failed=false;std::atomic<bool> release=false,main_called=false;std::atomic<unsigned> finished=0;
        std::thread unblock([&]{while(!dispatch_failed&&!main_called)std::this_thread::yield();std::this_thread::sleep_for(std::chrono::milliseconds(20));release=true;});
        bool caught=false;allocation_countdown=fault;
        auto callback=[&](size_t c,size_t,size_t){if(c==0){main_called=true;return;}while(!release)std::this_thread::yield();++finished;};
        try{pt::ParallelChunks(100,4,callback);}catch(const std::bad_alloc&){caught=true;}
        allocation_countdown=-1;unsigned before=finished.load();main_called=true;unblock.join();
        pt::ParallelChunks(100,4,[](size_t,size_t,size_t){});
        unsigned after=finished.load();
        if(caught&&after)++dispatch_failures;
        if(caught&&before!=after){std::printf("fault %d: ParallelChunks returned while %u dispatched callbacks were unfinished\n",fault,after-before);escaped=true;}
    }
    assert(dispatch_failures>0);
    if(!escaped)std::printf("Parallel allocation recovery: 24 injected allocation positions, including %u failures after dispatch, drained all callbacks before return\n",dispatch_failures);
    return escaped?1:0;
}
