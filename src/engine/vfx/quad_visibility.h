#pragma once
#include <glm/glm.hpp>
#include <cmath>
namespace pt::vfx {
inline bool QuadVisible(const glm::mat4& projection,const glm::vec4* corners){
 bool outside[4]={true,true,true,true};
 for(uint32_t i=0;i<4;++i){const glm::vec4 p=projection*glm::vec4(glm::vec3(corners[i]),1.0f);if(!std::isfinite(p.x+p.y+p.z+p.w))return true;
  // Generous padding also covers temporal jitter at low render scales.
  const float margin=.02f*(std::abs(p.w)+1.0f);const float planes[4]={p.w+p.x,p.w-p.x,p.w+p.y,p.w-p.y};for(uint32_t k=0;k<4;++k)outside[k]&=planes[k]<-margin;
 }return !(outside[0]||outside[1]||outside[2]||outside[3]);
}
}
