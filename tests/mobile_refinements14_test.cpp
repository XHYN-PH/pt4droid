#include <cassert>
#include <cstdio>
#include <random>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/packing.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/component_wise.hpp>
#include "engine/render/mobile_gpu_policy.h"
#include "engine/render/shadow_cache.h"
#include "engine/vfx/quad_visibility.h"

int main(){
    for(uint32_t vendor:{0x5143u,0x1002u,0x10DEu,0x8086u,0u})assert(!pt::UseMaliLighting(vendor,true));
    assert(pt::UseMaliLighting(0x13B5,true));assert(!pt::UseMaliLighting(0x13B5,false));
    pt::ShadowMapState source;source.view=glm::translate(glm::mat4(1),glm::vec3(3,4,5));source.bias_slope=2;source.caster_views=3;
    pt::ShadowCasterState caster;caster.mesh=77;caster.pose=std::make_shared<const std::vector<glm::mat4>>(1,glm::mat4(1));source.casters.push_back(caster);
    pt::ShadowMapState target;target.casters.reserve(100);const auto capacity=target.casters.capacity();target.CopyProjection(source);
    assert(pt::SameShadowProjection(source,target));assert(target.caster_views==3&&target.casters.empty()&&target.casters.capacity()==capacity);
    target.casters=source.casters;pt::ShadowTileCache cache;cache.Recorded(target);assert(cache.Reuse(source));
    source.casters[0].pose=std::make_shared<const std::vector<glm::mat4>>(1,glm::translate(glm::mat4(1),glm::vec3(1,0,0)));
    assert(cache.Miss(source)==pt::ShadowMiss::Casters);source.casters=target.casters;source.casters[0].alpha[0]++;assert(!cache.Reuse(source));
    std::mt19937 random(1415);std::uniform_real_distribution<float> unit(-1.f,1.f),positive(.001f,1.f);
    float max_ray_error=0,max_atlas_error=0;
    auto reference_visible=[](const glm::mat4& matrix,const glm::vec4* corners){
        bool outside[4]={true,true,true,true};
        for(int i=0;i<4;++i){const glm::vec4 p=matrix*glm::vec4(glm::vec3(corners[i]),1);if(!std::isfinite(p.x+p.y+p.z+p.w))return true;
            float margin=.02f*(std::abs(p.w)+1);const float planes[4]={p.w+p.x,p.w-p.x,p.w+p.y,p.w-p.y};for(int k=0;k<4;++k)outside[k]&=planes[k]<-margin;
        }return !(outside[0]||outside[1]||outside[2]||outside[3]);
    };
    for(int i=0;i<100000;++i){
        glm::mat4 matrix;for(int c=0;c<4;++c)for(int r=0;r<4;++r)matrix[c][r]=unit(random)*3;
        glm::vec4 corners[4];for(auto& p:corners)p=glm::vec4(unit(random)*10,unit(random)*10,unit(random)*10,1);
        assert(pt::vfx::QuadVisible(matrix,corners)==reference_visible(matrix,corners));
        glm::mat4 inverse=glm::rotate(glm::mat4(1),unit(random)*3.f,glm::normalize(glm::vec3(.3f,.8f,.2f)));
        inverse[3]=glm::vec4(unit(random)*100.f,unit(random)*100.f,unit(random)*100.f,1);
        glm::vec3 eye(inverse[3]);glm::vec2 projection(positive(random)*2,positive(random)*2),jitter(unit(random)*.003f,unit(random)*.003f);
        glm::vec2 uv(positive(random),positive(random));float z=positive(random)*1000;
        glm::vec2 ndc=uv*2.f-1.f-jitter;
        glm::vec3 original=glm::vec3(inverse*glm::vec4(ndc*projection*z,z,1))-eye;
        glm::vec3 rays[3];const glm::vec2 vertices[3]={{0,0},{2,0},{0,2}};
        for(int j=0;j<3;++j)rays[j]=glm::mat3(inverse)*glm::vec3((vertices[j]*2.f-1.f-jitter)*projection,1);
        glm::vec3 ray=rays[0]*(1.f-uv.x*.5f-uv.y*.5f)+rays[1]*(uv.x*.5f)+rays[2]*(uv.y*.5f);
        glm::vec3 actual=glm::vec3(inverse[3])-eye+ray*z;
        float error=glm::compMax(glm::abs(actual-original));max_ray_error=std::max(error,max_ray_error);
        assert(error<.002f);
        // Half atlas quantization precedes the same full-float bilinear mix.
        glm::vec4 taps;for(int k=0;k<4;++k)taps[k]=positive(random)*8.f;
        glm::vec4 half=glm::unpackHalf4x16(glm::packHalf4x16(taps));
        auto mix=[&](glm::vec4 t){return glm::mix(glm::mix(t.x,t.y,uv.x),glm::mix(t.z,t.w,uv.x),uv.y);};
        error=std::abs(mix(taps)-mix(half));max_atlas_error=std::max(error,max_atlas_error);assert(error<.004f);
    }
    printf("Test 14: Mali/Adreno isolation, shadow state/invalidation and 100000 ray/atlas comparisons passed (max ray %.6f, half atlas %.6f)\n",max_ray_error,max_atlas_error);
}
