#version 460
#include "common.glsl"
#include "probe_sh.glsl"
layout(push_constant) uniform PassPush { uvec4 ids; vec4 f0; vec4 f1; vec4 f2; mat4 m; } pass;
layout(location = 0) out vec4 out_map;
void main() {
    ivec2 pixel = ivec2(gl_FragCoord.xy);
    uint probe = uint(pixel.x / 16 + (pixel.y / 16) * 32);
    if (probe >= pass.ids.y) discard;
    out_map = vec4(SphereMapTexel(frame.probes[probe], frame.views[pass.ids.x], pixel % 16), 1.0);
}
