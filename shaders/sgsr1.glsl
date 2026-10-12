// Copyright (c) 2025, Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause
// Adapted from SGSR1 mobile RGBA kernel at d926f074bcb9d714e179f1ce0fcb9ee2eeb5074e.
// Vulkan sampler/function interface, adjustable blend and finite-denominator guards.
// Full license: assets/notices/SGSR1-LICENSE.txt.
float SgsrLanczos(float x){float a=x-4.0;float b=x*a-a;return b*a*a;}
vec2 SgsrWeight(float dx,float dy,float c,float deviation){
    float w=SgsrLanczos((dx*dx+dy*dy)*.55+clamp(abs(c)*deviation,0.0,1.0));
    return vec2(w,w*c);
}
vec3 Sgsr1(sampler2D source,vec2 uv,vec2 reciprocal,float amount){
    vec3 base=textureLod(source,uv,0.0).rgb;
    vec2 pixel=uv/reciprocal+vec2(-.5,.5),origin=floor(pixel),coord=origin*reciprocal;
    vec2 pl=pixel-origin;
    vec4 left=textureGather(source,coord,1);
    float vote=abs(left.z-left.y)+abs(base.g-left.y)+abs(base.g-left.z);
    if(vote<=8.0/255.0)return base;
    coord.x+=reciprocal.x;
    vec4 right=textureGather(source,coord+vec2(reciprocal.x,0),1);
    vec4 upDown;
    upDown.xy=textureGather(source,coord+vec2(0,-reciprocal.y),1).wz;
    upDown.zw=textureGather(source,coord+vec2(0,reciprocal.y),1).yx;
    float mean=(left.y+left.z+right.x+right.w)*.25;
    left-=mean;right-=mean;upDown-=mean;
    float current=base.g-mean;
    float sum=dot(abs(left)+abs(right)+abs(upDown),vec4(1));
    float deviation=2.181818/max(sum,1e-6);
    vec2 weight=SgsrWeight(pl.x,pl.y+1.0,upDown.x,deviation);
    weight+=SgsrWeight(pl.x-1.0,pl.y+1.0,upDown.y,deviation);
    weight+=SgsrWeight(pl.x-1.0,pl.y-2.0,upDown.z,deviation);
    weight+=SgsrWeight(pl.x,pl.y-2.0,upDown.w,deviation);
    weight+=SgsrWeight(pl.x+1.0,pl.y-1.0,left.x,deviation);
    weight+=SgsrWeight(pl.x,pl.y-1.0,left.y,deviation);
    weight+=SgsrWeight(pl.x,pl.y,left.z,deviation);
    weight+=SgsrWeight(pl.x+1.0,pl.y,left.w,deviation);
    weight+=SgsrWeight(pl.x-1.0,pl.y-1.0,right.x,deviation);
    weight+=SgsrWeight(pl.x-2.0,pl.y-1.0,right.y,deviation);
    weight+=SgsrWeight(pl.x-2.0,pl.y,right.z,deviation);
    weight+=SgsrWeight(pl.x-1.0,pl.y,right.w,deviation);
    if(abs(weight.x)<1e-6)return base;
    float low=min(min(left.y,left.z),min(right.x,right.w));
    float high=max(max(left.y,left.z),max(right.x,right.w));
    float y=clamp(2.0*weight.y/weight.x,low,high);
    float delta=clamp(y-current,-23.0/255.0,23.0/255.0);
    return mix(base,clamp(base+delta,0.0,1.0),clamp(amount*.5,0.0,1.0));
}
