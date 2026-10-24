#pragma once
#include <algorithm>
#include <numeric>
#include <vector>
#include <cstdint>
#include <cmath>
namespace pt {
// Keep the previous permutation when it is still sorted. An explicit source
// index tie-break preserves stable_sort(iota)'s original emission order even
// after depths, priorities, layers, or the entire draw list change.
template<class Draws> void UpdateVfxOrder(std::vector<uint32_t>& order,const Draws& draws) {
    if(order.size()!=draws.size()){order.resize(draws.size());std::iota(order.begin(),order.end(),0u);}
    auto before=[&](uint32_t a,uint32_t b){
        const auto& x=draws[a];const auto& y=draws[b];
        if(x.layer!=y.layer)return x.layer<y.layer;
        // Screen is layer 2 in the serialized VFX interface.
        if(int(x.layer)==2){if(x.priority!=y.priority)return x.priority<y.priority;}
        else {const bool nx=std::isnan(x.depth),ny=std::isnan(y.depth);if(nx!=ny)return !nx;if(x.depth>y.depth)return true;if(x.depth<y.depth)return false;}
        return a<b;
    };
    if(!std::is_sorted(order.begin(),order.end(),before))std::sort(order.begin(),order.end(),before);
}
}
