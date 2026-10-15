#version 460
#include "common.glsl"
#include "probe_sh.glsl"
layout(push_constant) uniform PassPush { uvec4 ids; vec4 f0; vec4 f1; vec4 f2; uvec4 visibility[4]; } pass;
layout(location = 0) out vec4 out_map;
void main() {
    ivec2 pixel = ivec2(gl_FragCoord.xy);
    uint probe = uint(pixel.x / 16 + (pixel.y / 16) * 32);
    if (probe >= pass.ids.y) discard;
    uint word = probe / 32u;
    uint visible = pass.visibility[word / 4u][word % 4u];
    if ((visible & (1u << (probe % 32u))) == 0u) discard;
    out_map = vec4(SphereMapTexel(frame.probes[probe], frame.views[pass.ids.x], pixel % 16), 1.0);
}
