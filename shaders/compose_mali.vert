#version 460
#include "common.glsl"
layout(push_constant) uniform PassPush { uvec4 ids; vec4 f0; vec4 f1; vec4 f2; mat4 m; } pass;
layout(location = 0) out vec2 out_uv;
layout(location = 1) out vec3 out_fog_ray;
layout(location = 2) flat out vec3 out_fog_origin;
void main() {
    vec2 position = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
    out_uv = position;
    gl_Position = vec4(position * 2.0 - 1.0, 0.0, 1.0);
    View v = frame.views[pass.ids.x];
    vec2 ndc = position * 2.0 - 1.0 - v.jitter.xy;
    // Affine view reconstruction interpolates exactly over the fullscreen
    // triangle. Fog remains full float and uses the original depth and curve.
    out_fog_ray = mat3(v.inv_view) * vec3(ndc * v.projection_param.xy, 1.0);
    out_fog_origin = v.inv_view[3].xyz - v.eye.xyz;
}
