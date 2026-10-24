#version 460
#include "common.glsl"
layout(push_constant) uniform PassPush {uvec4 ids;vec4 f0;vec4 f1;vec4 f2;mat4 m;} pass;
layout(location=0) in vec2 in_uv;
layout(location=0) out vec4 out_color;
vec3 Lut16(uint lut,vec3 c){
    float slice=floor(15.0*c.b);
    vec2 uv=vec2(c.r*(15.0/256.0)+1.0/512.0+slice/16.0,c.g*(15.0/16.0)+1.0/32.0);
    return mix(Img(lut,SMP_LINEAR_CLAMP,uv).rgb,Img(lut,SMP_LINEAR_CLAMP,uv+vec2(1.0/16.0,0)).rgb,15.0*c.b-slice);
}
void main(){
    ivec2 pixel=clamp(ivec2(in_uv*ImgSize(pass.ids.x)),ivec2(0),ivec2(ImgSize(pass.ids.x))-1);
    vec3 c=ImgFetch(pass.ids.x,pixel).rgb;
    vec3 color=Lut16(RES_COLOR_LUT,c);
    if(pass.f0.y<1.0)color=mix(Lut16(RES_COLOR_LUT_PREV,c),color,pass.f0.y);
    // Preserve the grading pass's original RGBA8 UNORM boundary before the
    // screen pass's dither. Distortion/sharpening retain the separate path.
    color=floor(clamp(color,0.0,1.0)*255.0+.5)/255.0;
    vec3 seed=fract(vec3(gl_FragCoord.xyx)*vec3(.1031,.1030,.0973));seed+=dot(seed,seed.yxz+33.33);
    float noise=fract((seed.x+seed.y)*seed.z)-.5;
    out_color=vec4(clamp(color+noise/255.0,0.0,1.0),1.0);
}
