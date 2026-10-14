#include "common.glsl"

layout(location = 0) in vec2 in_uv0;
#ifdef PT_CLIP_FALLBACK
layout(location = 1) in float in_clip_distance;
#endif

layout(push_constant) uniform DrawPush {
    mat4 model;
    uvec4 ids;
    vec4 tint;
} draw;

void main() {
#ifdef PT_CLIP_FALLBACK
    // No early_fragment_tests: discarded hemisphere fragments must not write depth.
    if (in_clip_distance < 0.0) discard;
#endif
    Material m = materials[draw.ids.y];
    if ((m.flags & MAT_ALPHA_TEST) != 0u) {
        float a = texture(textures[nonuniformEXT(m.albedo)], in_uv0).a;
        if (a < ShadowAlphaCutoff(m.flags)) {
            discard;
        }
    }
}
