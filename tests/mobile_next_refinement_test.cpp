#include <algorithm>
#include <array>
#include <cassert>
#include <cstdio>
#include <random>
#include <glm/gtc/matrix_transform.hpp>
#include "engine/render/shadow_atlas.h"
#include "engine/render/light_scissor.h"
#include "engine/vfx/quad_visibility.h"
#include "engine/render/shadow_cache.h"
int main(){
 std::mt19937 rng(7331);pt::StableShadowAtlas atlas;std::vector<pt::StableShadowAtlas::Request> lights;
 for(uint32_t i=0;i<8;++i)lights.push_back({std::to_string(i),2,i*2});atlas.Assign(lights);auto original=lights;
 for(int trial=0;trial<5000;++trial){std::shuffle(lights.begin(),lights.end(),rng);atlas.Assign(lights);std::array<bool,16> used{};for(const auto& l:lights){assert(l.slot==original[std::stoi(l.key)].slot && l.slot%2==0);for(uint32_t i=0;i<l.width;++i){assert(!used[l.slot+i]);used[l.slot+i]=true;}}}
 lights.clear();for(uint32_t i=0;i<16;++i)lights.push_back({"spot"+std::to_string(i),1,i});atlas.Assign(lights);lights.clear();for(uint32_t i=0;i<4;++i)lights.push_back({"spot"+std::to_string(i*2),1,i});for(uint32_t i=0;i<6;++i)lights.push_back({"point"+std::to_string(i),2,4+i*2});atlas.Assign(lights);std::array<bool,16> used{};for(const auto& l:lights)for(uint32_t i=0;i<l.width;++i){assert(l.slot+i<16 && !used[l.slot+i]);used[l.slot+i]=true;}
 std::uniform_real_distribution<float> v(-1,1);
 for(int trial=0;trial<5000;++trial){glm::vec3 axis=glm::normalize(glm::vec3(v(rng),v(rng),v(rng))),origin(v(rng),v(rng),v(rng)),lo,hi;float cosine=std::abs(v(rng))*.99f,radius=.1f+std::abs(v(rng))*30;assert(pt::SpotBounds(origin,axis,cosine,radius,lo,hi));for(int i=0;i<100;++i){glm::vec3 d=glm::normalize(glm::vec3(v(rng),v(rng),v(rng)));if(glm::dot(d,axis)<cosine)continue;glm::vec3 point=origin+d*(std::abs(v(rng))*radius);assert(glm::all(glm::greaterThanEqual(point,lo)) && glm::all(glm::lessThanEqual(point,hi)));}}
 for(int trial=0;trial<8000;++trial){auto projection=glm::perspectiveRH_ZO(1.0f,2.0f,.05f,100.0f);auto volume=glm::translate(glm::mat4(1),glm::vec3(v(rng)*8,v(rng)*4,-8-v(rng)*6))*glm::scale(glm::mat4(1),glm::vec3(.1f+std::abs(v(rng))));auto clip=projection*volume;VkExtent2D area{1200,540};auto rect=pt::LightScissor(clip,area);for(int i=0;i<16;++i){auto q=clip*glm::vec4(v(rng),v(rng),v(rng),1);if(q.w<=0 || q.z<0 || q.z>q.w)continue;glm::vec2 p=(glm::vec2(q)/q.w*.5f+.5f)*glm::vec2(area.width,area.height);if(p.x<0||p.y<0||p.x>=area.width||p.y>=area.height)continue;assert(p.x>=rect.offset.x&&p.y>=rect.offset.y&&p.x<=rect.offset.x+rect.extent.width&&p.y<=rect.offset.y+rect.extent.height);}}
 glm::vec4 corners[4]={{-2,-1,0,1},{0,-1,0,1},{-2,1,0,1},{0,1,0,1}};assert(pt::vfx::QuadVisible(glm::mat4(1),corners));for(auto& q:corners)q.x-=3;assert(!pt::vfx::QuadVisible(glm::mat4(1),corners));corners[0].x=std::numeric_limits<float>::quiet_NaN();assert(pt::vfx::QuadVisible(glm::mat4(1),corners));
 struct Item{float depth;std::array<float,48> payload;uint32_t id;};for(int trial=0;trial<4000;++trial){std::vector<Item> items;for(uint32_t i=0;i<1+rng()%250;++i)items.push_back({float(rng()%15),{},i});auto ref=items;std::sort(ref.begin(),ref.end(),[](const auto&a,const auto&b){return a.depth>b.depth;});std::vector<size_t> order(items.size());for(size_t i=0;i<order.size();++i)order[i]=i;std::sort(order.begin(),order.end(),[&](size_t a,size_t b){return items[a].depth>items[b].depth;});for(size_t i=0;i<order.size();++i)assert(items[order[i]].id==ref[i].id);}
 pt::ShadowMapState state;state.casters.push_back({1,2,3,0,glm::mat4(1),{1,17,17}});pt::ShadowTileCache cache;cache.Recorded(state);assert(cache.Reuse(state));state.casters[0].alpha[2]=19;assert(!cache.Reuse(state));cache.Recorded(state);state.casters[0].alpha[1]=23;assert(!cache.Reuse(state));
 puts("Atlas, spot/scissor coverage, conservative quads, exact sort permutations and alpha dependency checks passed");
}
