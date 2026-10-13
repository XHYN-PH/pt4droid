#pragma once
#include <glm/glm.hpp>
#include <volk.h>
#include <algorithm>
#include <cmath>
namespace pt {
inline bool SpotBounds(glm::vec3 position,glm::vec3 axis,float cosine,float radius,glm::vec3& lo,glm::vec3& hi){
 const float norm=glm::length(axis);
 if(!std::isfinite(norm+cosine+radius)||norm<1e-5f||cosine<0||cosine>=1||radius<=0)return false;
 axis/=norm;cosine=std::max(0.0f,cosine-1e-5f);const float sine=std::sqrt(std::max(0.0f,1.0f-cosine*cosine));
 for(int i=0;i<3;++i){const float perpendicular=std::sqrt(std::max(0.0f,1.0f-axis[i]*axis[i]));const float maximum=axis[i]>=cosine?1.0f:axis[i]*cosine+perpendicular*sine;const float minimum=axis[i]<=-cosine?-1.0f:axis[i]*cosine-perpendicular*sine;lo[i]=position[i]+radius*std::min(0.0f,minimum)-0.01f;hi[i]=position[i]+radius*std::max(0.0f,maximum)+0.01f;}return true;
}
inline VkRect2D LightScissor(const glm::mat4& clip,VkExtent2D area){
 glm::vec2 lo(1.0f),hi(-1.0f);
 for(uint32_t i=0;i<8;++i){const glm::vec4 p=clip*glm::vec4((i&1)?1.0f:-1.0f,(i&2)?1.0f:-1.0f,(i&4)?1.0f:-1.0f,1.0f);
  // Crossing a camera or near plane keeps the full view, including mirrors.
  if(!std::isfinite(p.x+p.y+p.z+p.w)||p.w<=1e-5f||p.z<0.0f||p.z>p.w)return {{0,0},area};
  const glm::vec2 q=glm::vec2(p)/p.w;lo=glm::min(lo,q);hi=glm::max(hi,q);
 }
 const glm::vec2 size(area.width,area.height);lo=glm::clamp(glm::floor((lo*.5f+.5f)*size)-2.0f,glm::vec2(0),size);hi=glm::clamp(glm::ceil((hi*.5f+.5f)*size)+2.0f,glm::vec2(0),size);
 return {{int32_t(lo.x),int32_t(lo.y)},{uint32_t(std::max(0.0f,hi.x-lo.x)),uint32_t(std::max(0.0f,hi.y-lo.y))}};
}
}
