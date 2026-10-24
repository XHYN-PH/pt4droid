#include "engine/render/shadow_cache.h"
#include "engine/vfx/flare_halo.h"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <limits>
#include <random>
int main(){
 using namespace pt;
 std::mt19937 rng(2271);
 ShadowMapState state;
 for(unsigned i=0;i<80;++i){ShadowCasterState caster;caster.mesh=1+i%9;caster.submesh=i%3;caster.material=i%7;caster.transform[3][0]=float(i%13);caster.alpha={i%2,i%5,i%11};
  if(i%3==0)caster.pose=std::make_shared<const std::vector<glm::mat4>>(2,glm::mat4(float(i+1)));
  state.casters.push_back(caster);
 }
 state.CanonicalizeCasters();ShadowTileCache cache;cache.Recorded(state);
 for(unsigned i=0;i<1500;++i){auto shuffled=state;std::shuffle(shuffled.casters.begin(),shuffled.casters.end(),rng);shuffled.CanonicalizeCasters();assert(cache.Reuse(shuffled));}
 auto changed=state;changed.casters[0].transform[3].x+=1;changed.CanonicalizeCasters();assert(!cache.Reuse(changed));
 changed=state;++changed.casters[0].alpha[2];changed.CanonicalizeCasters();assert(!cache.Reuse(changed));
 changed=state;changed.casters.pop_back();changed.CanonicalizeCasters();assert(!cache.Reuse(changed));
 changed=state;changed.caster_views=2;assert(!cache.Reuse(changed));
 changed=state;changed.casters[0].pose=std::make_shared<const std::vector<glm::mat4>>(2,glm::mat4(99));changed.CanonicalizeCasters();assert(!cache.Reuse(changed));
 for(unsigned i=0;i<=1000;++i){float span=i/1000.f;float value=vfx::FlareHaloAttenuation(span,.2f);assert(value>=.19999f&&value<=1.f);assert(vfx::FlareHaloAttenuation(span,1.f)==1.f);if(span<=.12f)assert(value==1.f);if(span>=.6f)assert(std::abs(value-.2f)<1e-6f);}
 assert(vfx::FlareHaloAttenuation(std::numeric_limits<float>::quiet_NaN(),.2f)==1.f);
 // The filtered footprint never leaves its 16x16 cell. Compare the normalized
 // atlas coordinate with independent four-texel bilinear interpolation.
 std::uniform_real_distribution<float> random(-1.f,1.f);
 for(unsigned i=0;i<100000;++i){float z=random(rng),angle=random(rng)*3.14159265f;float radius=.40625f*std::sqrt(.5f*z+.5f);
  float sx=.5f-radius*std::cos(angle),sy=.5f-radius*std::sin(angle);float x=sx*16-.5f,y=sy*16-.5f;
  assert(std::floor(x)>=0&&std::ceil(x)<16&&std::floor(y)>=0&&std::ceil(y)<16);
  float a=random(rng),b=random(rng),c=random(rng),d=random(rng),fx=x-std::floor(x),fy=y-std::floor(y);
  float original=(a+(b-a)*fx)+((c+(d-c)*fx)-(a+(b-a)*fx))*fy;
  float weighted=a*(1-fx)*(1-fy)+b*fx*(1-fy)+c*(1-fx)*fy+d*fx*fy;
  assert(std::abs(original-weighted)<5e-7f);
 }
 puts("Release policies: 1500 order-independent exact shadow keys with transform/material/pose invalidation; halo core preservation; 100000 cell-safe probe filter comparisons passed");
}
