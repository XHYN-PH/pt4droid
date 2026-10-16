#include "engine/assets/texture_preparation_stream.h"
#include <cassert>
#include <chrono>
#include <iostream>

// Archive I/O is replaced by a tiny budgeted source; queue and budget are production code.
namespace pt {std::string FtexStem(std::string_view path){return std::string(path);}}
using namespace std::chrono_literals;

int main(){
    for(int iteration=0;iteration<100;++iteration){
        for(int mode=0;mode<3;++mode){
            auto budget=std::make_shared<pt::TextureReadBudget>(8);
            std::mutex mutex;std::condition_variable changed;int attempts=0;
            pt::TexturePreparationStream stream(budget,[&](const std::string& path,pt::FtexTexture&,pt::TextureReadBudget* lease){
                const bool ok=lease->Reserve(8);
                if(path=="waiting"){std::lock_guard lock(mutex);++attempts;changed.notify_all();}
                return ok;
            });
            auto first=stream.Enqueue("held");assert(first.wait_for(1s)==std::future_status::ready);
            auto held=first.get();assert(held.texture&&budget->Used()==8);first={};
            auto waiting=stream.Enqueue("waiting");
            {std::unique_lock lock(mutex);assert(changed.wait_for(lock,1s,[&]{return attempts>0;}));}
            if(mode==0){held={};assert(waiting.wait_for(1s)==std::future_status::ready);assert(waiting.get().texture);}
            else if(mode==1){stream.Prioritize("waiting");assert(waiting.wait_for(1s)==std::future_status::ready);assert(waiting.get().failure==pt::TexturePreparationFailure::Budget);held={};}
            else {stream.Stop();assert(waiting.wait_for(1s)==std::future_status::ready);assert(waiting.get().failure==pt::TexturePreparationFailure::Cancelled);held={};}
        }
    }
    auto budget=std::make_shared<pt::TextureReadBudget>(8);
    pt::TexturePreparationStream stream(budget,[](const std::string&,pt::FtexTexture&,pt::TextureReadBudget* lease){return lease->Reserve(9);});
    auto oversized=stream.Enqueue("oversized");assert(oversized.wait_for(1s)==std::future_status::ready);assert(oversized.get().failure==pt::TexturePreparationFailure::Budget);
    std::cout<<"Texture backpressure passed: 100 release/demand/cancellation rounds; oversized fallback\n";
}
