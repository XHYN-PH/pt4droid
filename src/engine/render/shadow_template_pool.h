#pragma once
#include <vulkan/vulkan.h>
#include <array>
#include <algorithm>
#include <cstdint>
namespace pt {
struct ShadowTemplatePlan {uint32_t width=0,count=0;std::array<bool,16> use{};std::array<uint32_t,16> tiles{};std::array<VkOffset2D,16> offsets{};};
inline ShadowTemplatePlan PlanShadowTemplates(uint32_t atlas,uint32_t tile,const std::array<bool,16>& eligible,const std::array<uint32_t,16>& savings) {
    ShadowTemplatePlan p;p.width=std::min(atlas,4096u);if(!tile||tile>p.width)return p;
    for(uint32_t i=0;i<16;++i)if(eligible[i])p.tiles[p.count++]=i;
    std::stable_sort(p.tiles.begin(),p.tiles.begin()+p.count,[&](auto a,auto b){return savings[a]>savings[b];});
    const uint32_t columns=p.width/tile;p.count=std::min(p.count,columns*columns);
    for(uint32_t i=0;i<p.count;++i){p.use[p.tiles[i]]=true;p.offsets[p.tiles[i]]={int32_t(i%columns*tile),int32_t(i/columns*tile)};}
    return p;
}
}
