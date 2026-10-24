#version 460
#include "common.glsl"
#define IMG_MOTION 45
layout(push_constant) uniform SgsrPush {uvec4 ids;vec4 f0;vec4 f1;vec4 f2;mat4 m;} pass;
struct SgsrParams {vec4 clipToPrevClip[4];vec2 renderSize;vec2 outputSize;vec2 renderSizeRcp;vec2 outputSizeRcp;vec2 jitterOffset;vec2 scaleRatio;float cameraFovAngleHor;float minLerpContribution;float reset;uint bSameCamera;};
SgsrParams params;
void SetupParams(){params.renderSize=pass.f0.xy;params.outputSize=pass.f0.zw;params.renderSizeRcp=1.0/params.renderSize;params.outputSizeRcp=1.0/params.outputSize;
 params.jitterOffset=pass.f1.xy;params.scaleRatio=vec2(1.0+params.outputSize.x/params.renderSize.x,2.0);params.cameraFovAngleHor=pass.f1.z;params.minLerpContribution=.25;params.reset=pass.f1.w;params.bSameCamera=0u;}
#define InputDepth sampler2D(images[IMG_DEPTH],samplers[SMP_POINT_CLAMP])
#define InputVelocity sampler2D(images[IMG_MOTION],samplers[SMP_POINT_CLAMP])
#define PrevOutput sampler2D(images[pass.ids.x],samplers[SMP_LINEAR_CLAMP])
#define MotionDepthClipAlphaBuffer sampler2D(images[60],samplers[SMP_LINEAR_CLAMP])
#define InputColor sampler2D(images[pass.ids.y],samplers[SMP_LINEAR_CLAMP])

//============================================================================================================
//
//
//                  Copyright (c) 2024, Qualcomm Innovation Center, Inc. All rights reserved.
//                              SPDX-License-Identifier: BSD-3-Clause
//
//============================================================================================================


layout(location = 0) out vec4 Converted;
layout(location = 0) in vec2 texCoord;




vec2 decodeVelocityFromTexture(vec2 ev) {
    const float inv_div = 1.0f / (0.499f * 0.5f);
    vec2 dv;
    dv.xy = ev.xy * inv_div - 32767.0f / 65535.0f * inv_div;
    //dv.z = uintBitsToFloat((uint(round(ev.z * 65535.0f)) << 16) | uint(round(ev.w * 65535.0f)));
    return dv;
}

void main()
{
    SetupParams();
    uvec2 InputPos = uvec2(texCoord * params.renderSize);
    vec2 gatherCoord = texCoord - vec2(0.5) * params.renderSizeRcp;


    // texture gather to find nearest depth
    //      a  b  c  d
    //      e  f  g  h
    //      i  j  k  l
    //      m  n  o  p
    //btmLeft mnji
    //btmRight oplk
    //topLeft  efba
    //topRight ghdc

    vec4 btmLeft = textureGather(InputDepth, gatherCoord, 0);
    vec2 v10 = vec2(params.renderSizeRcp.x * 2.0f, 0.0);
    vec4 btmRight = textureGather(InputDepth,(gatherCoord+v10), 0);
    vec2 v12 = vec2(0.0, params.renderSizeRcp.y * 2.0f);
	vec4 topLeft = textureGather(InputDepth,(gatherCoord+v12), 0);
	vec2 v14 = vec2(params.renderSizeRcp.x * 2.0f, params.renderSizeRcp.y * 2.0f);
	vec4 topRight = textureGather(InputDepth,(gatherCoord+v14), 0);
	float maxC = max(max(max(btmLeft.z,btmRight.w),topLeft.y),topRight.x);
	float btmLeft4 = max(max(max(btmLeft.y,btmLeft.x),btmLeft.z),btmLeft.w);
	float btmLeftMax9 = max(topLeft.x,max(max(maxC,btmLeft4),btmRight.x));

    float depthclip = 0.0;
    if (maxC > 1.0e-05f)
    {
        float btmRight4 = max(max(max(btmRight.y,btmRight.x),btmRight.z),btmRight.w);
        float topLeft4 = max(max(max(topLeft.y,topLeft.x),topLeft.z),topLeft.w);
        float topRight4 = max(max(max(topRight.y,topRight.x),topRight.z),topRight.w);

        float Wdepth = 0.0;
        float Ksep = 1.37e-05f;
        float Kfov = params.cameraFovAngleHor;
        float diagonal_length = length(params.renderSize);
        float Ksep_Kfov_diagonal = Ksep * Kfov * diagonal_length;

		float Depthsep = Ksep_Kfov_diagonal * maxC;
		float EPSILON = 1.19e-07f;
		Wdepth += clamp((Depthsep / (abs(maxC - btmLeft4) + EPSILON)), 0.0, 1.0);
		Wdepth += clamp((Depthsep / (abs(maxC - btmRight4) + EPSILON)), 0.0, 1.0);
		Wdepth += clamp((Depthsep / (abs(maxC - topLeft4) + EPSILON)), 0.0, 1.0);
		Wdepth += clamp((Depthsep / (abs(maxC - topRight4) + EPSILON)), 0.0, 1.0);
        depthclip = clamp(1.0f - Wdepth * 0.25, 0.0, 1.0);
    }

    vec2 motion = -2.0 * texelFetch(InputVelocity,clamp(ivec2(InputPos),ivec2(0),ivec2(params.renderSize)-1),0).xy;
    Converted = vec4(motion, depthclip, 0.0);
}
