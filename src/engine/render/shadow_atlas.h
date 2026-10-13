#pragma once
#include <array>
#include <string>
#include <vector>
#include <cstdint>
namespace pt {
class StableShadowAtlas {
public:
 struct Request {std::string key;uint32_t width,slot;};
 void Assign(std::vector<Request>& requests) {
  auto original=requests;std::array<bool,16> used{};
  auto claim=[&](uint32_t slot,uint32_t width){
   if(slot>=16 || width<1 || width>2 || slot%4+width>4 || (width==2 && slot%2))return false;
   for(uint32_t i=0;i<width;++i)if(used[slot+i])return false;
   for(uint32_t i=0;i<width;++i)used[slot+i]=true;return true;
  };
  for(auto& r:requests){r.slot=16;for(uint32_t i=0;i<16;++i)if(owners_[i].key==r.key && owners_[i].width==r.width && claim(i,r.width)){r.slot=i;break;}}
  bool complete=true;
  for(uint32_t width:{2u,1u})for(auto& r:requests)if(r.slot==16 && r.width==width){for(uint32_t i=0;i<16;++i)if(claim(i,r.width)){r.slot=i;break;}complete &= r.slot!=16;}
  // Fall back to the complete original packing, preserving every selected light.
  if(!complete)requests=std::move(original);owners_={};
  for(const auto& r:requests)if(r.slot<16)owners_[r.slot]={r.key,r.width,r.slot};
 }
 void Clear(){owners_={};}
private: std::array<Request,16> owners_{};
};
}
