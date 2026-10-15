// The probe pass (rendering.md 5 and 12.6): probe.frag, and probe_ao.frag with PT_RT_AO (the ray traced ambient occlusion of
// 12.21 multiplies each probe's light, so the blended ambient, by rt_ao.comp's result at the pixel)
#include "common.glsl"
#ifdef PT_RT_AO
layout(set = 2, binding = 7, r32f) uniform readonly image2D rt_ao;
#endif

layout(push_constant) uniform PassPush {
    uvec4 ids;
    vec4 f0;
    vec4 f1;
    vec4 f2;
    mat4 m;
} pass;

layout(location = 0) out vec4 out_diffuse;

#include "probe_sh.glsl"
layout(constant_id = 0) const bool cached_probe_map = false;

vec3 ProbeTexel(Probe p, View v, ivec2 texel) {
    if (!cached_probe_map) return SphereMapTexel(p, v, texel);
    ivec2 cell = ivec2(int(pass.ids.y % 32u), int(pass.ids.y / 32u));
    return ImgFetch(IMG_PROBE_MAP, cell * 16 + texel).rgb;
}

// SSLighting2_SH_MultiBlend reads the atlas at r = 0.40625 sqrt(0.5 N.z + 0.5) from the cell centre along -normalize(N.xy),
// linearly filtered: the sample stays 13 of the cell's 16 texels wide, so it looks the irradiance up at the view direction of
// z = 0.66 (N.z + 1) - 1, not at N (a wall seen edge on, N.z = 0, takes the irradiance 20 degrees toward the camera), between
// the 16x16 texels of the map. The port evaluates the four texels the original filters.
vec3 SphereMapLookup(Probe p, View v, vec3 n_view) {
    float r = 0.40625 * sqrt(max(0.0, 0.5 * n_view.z + 0.5));
    float len = length(n_view.xy);
    vec2 dir = len > 0.0 ? n_view.xy / len : vec2(1.0, 0.0);
    vec2 s = 0.5 - r * dir;
    // Each sample stays inside its own 16x16 cell, including the half-texel
    // footprint. One hardware bilinear lookup replaces four fetches and mixes.
    if (cached_probe_map) {
        vec2 cell = vec2(pass.ids.y % 32u, pass.ids.y / 32u);
        return ImgLod(IMG_PROBE_MAP, SMP_LINEAR_CLAMP, (cell + s) * vec2(1.0 / 32.0, 1.0 / 16.0), 0.0).rgb;
    }
    vec2 texel = s * 16.0 - 0.5;
    ivec2 i0 = ivec2(floor(texel));
    vec2 f = texel - vec2(i0);
    vec3 a = ProbeTexel(p, v, i0);
    vec3 b = ProbeTexel(p, v, i0 + ivec2(1, 0));
    vec3 c = ProbeTexel(p, v, i0 + ivec2(0, 1));
    vec3 d = ProbeTexel(p, v, i0 + ivec2(1, 1));
    return mix(mix(a, b, f.x), mix(c, d, f.x), f.y);
}

void main() {
    View v = frame.views[pass.ids.x];
    Probe p = frame.probes[pass.ids.y];
    // pass.ids.z: the half resolution accumulation (isShrinkSHBuffer): texel (i, j) stands for the full resolution pixel
    // (2i, 2j), whose depth BilateralUpscale2x2 compares with
    ivec2 pixel = ivec2(gl_FragCoord.xy);
    vec2 frag = gl_FragCoord.xy;
    if (pass.ids.z != 0u) {
        pixel *= 2;
        frag = vec2(pixel) + 0.5;
    }
    float depth = ImgFetch(IMG_DEPTH, pixel).x;
    if (depth <= 0.0) {
        discard;
    }
    vec3 P = ViewPosition(v, PixelNdc(v, frag), depth);
    vec4 point = vec4(P, 1.0);
    vec3 q;
    if (pass.ids.w != 0u) {
        q = vec3(dot(pass.f0, point), dot(pass.f1, point), dot(pass.f2, point));
    } else {
        vec3 world = (v.inv_view * point).xyz;
        q = (p.box * vec4(world, 1.0)).xyz;
    }
    vec3 f = min(clamp((1.0 - q) * p.positive.xyz, 0.0, 1.0), clamp((1.0 + q) * p.negative.xyz, 0.0, 1.0));
    float weight = p.positive.w * f.x * f.y * f.z;
    if (weight <= 0.0) {
        discard;
    }
    vec3 N = DecodeNormal(ImgFetch(IMG_NORMAL, pixel).xyz);
    vec3 e = SphereMapLookup(p, v, N) * v.exposure.x;
#ifdef PT_RT_AO
    e *= imageLoad(rt_ao, pixel).x;
#endif
    out_diffuse = vec4(max(vec3(0.0), weight * e), max(0.0, 1.0 - weight));
}
