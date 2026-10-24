#pragma once
#include <algorithm>
#include <vector>

namespace pt {
// Match inspects only live entries. Retire must never dereference an owner's
// former stage: removal may have already freed that allocation.
template<class Owners,class Entries,class Match,class Retire>
bool RetireCollisionOwners(Owners& owners,const Entries& entries,Match match,Retire retire){
    // Retaining the old grid is exact only when surviving blocks keep their
    // relative order, including equal-distance ray/contact tie breaks.
    auto next=owners.begin();
    for(const auto& entry:entries) {
        next=std::find_if(next,owners.end(),[&](const auto& owner){return match(owner,entry);});
        if(next==owners.end())return false;
        ++next;
    }
    auto live=[&](const auto& owner){return std::any_of(entries.begin(),entries.end(),[&](const auto& entry){return match(owner,entry);});};
    for(const auto& owner:owners)if(!live(owner))retire(owner);
    std::erase_if(owners,[&](const auto& owner){return !live(owner);});
    return true;
}
}
