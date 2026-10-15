#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <random>

int main(){
    std::mt19937 rng(13015);std::uniform_real_distribution<float> unit(-1,1);
    float max_coordinate=0,max_weight=0;
    for(int i=0;i<100000;++i){
        glm::mat4 view=glm::translate(glm::mat4(1),glm::vec3(unit(rng),unit(rng),unit(rng))*100.f);
        view=glm::rotate(view,unit(rng)*3.14f,glm::normalize(glm::vec3(.3f,.7f,.2f)));
        glm::mat4 box=glm::scale(glm::mat4(1),glm::vec3(.1f+std::abs(unit(rng)),.1f+std::abs(unit(rng)),.1f+std::abs(unit(rng))));
        box=glm::translate(box,glm::vec3(unit(rng),unit(rng),unit(rng))*100.f);
        glm::vec4 p(glm::vec3(unit(rng),unit(rng),unit(rng))*100.f,1.f);
        if(i%2)p=glm::inverse(view)*glm::inverse(box)*glm::vec4(glm::vec3(unit(rng),unit(rng),unit(rng))*1.05f,1.f);
        glm::vec3 original=glm::vec3(box*(view*p));glm::mat4 combined=box*view;
        glm::vec3 actual;for(int row=0;row<3;++row)actual[row]=glm::dot(glm::vec4(combined[0][row],combined[1][row],combined[2][row],combined[3][row]),p);
        for(int c=0;c<3;++c){float error=std::abs(original[c]-actual[c]);max_coordinate=std::max(max_coordinate,error);assert(error<.0001f);}
        auto weight=[](glm::vec3 q){glm::vec3 f=glm::min(glm::clamp(1.f-q,0.f,1.f),glm::clamp(1.f+q,0.f,1.f));return f.x*f.y*f.z;};
        float error=std::abs(weight(original)-weight(actual));max_weight=std::max(max_weight,error);assert(error<.0001f);
    }
    printf("100000 full-float view/probe comparisons passed; maximum coordinate error %.8f, blend-weight error %.8f\n",max_coordinate,max_weight);
}
