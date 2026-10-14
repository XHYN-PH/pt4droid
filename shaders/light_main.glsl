// The light pass (rendering.md 12.2): light.frag, light_rt.frag with PT_RT_SHADOWS (ray traced shadows, 12.21) and
// light_contact.frag with PT_RT_CONTACT (the shadow maps and the ray traced contact shadows, 12.21; light_rt.frag has those too)
#include "common.glsl"
#if defined(PT_RT_SHADOWS) || defined(PT_RT_CONTACT)
#include "rt_shadow.glsl"
#endif
#include "lighting.glsl"

layout(push_constant) uniform PassPush {
    uvec4 ids;
    vec4 f0;
    vec4 f1;
    vec4 f2;
    mat4 m;
} pass;

layout(location = 0) out vec4 out_diffuse;
layout(location = 1) out vec4 out_specular;

void main() {
    View v = frame.views[pass.ids.x];
    Light l = frame.lights[pass.ids.y];
    ivec2 pixel = ivec2(gl_FragCoord.xy);
    float depth = ImgFetch(IMG_DEPTH, pixel).x;
    if (depth <= 0.0) {
        discard;
    }
    Surface s;
    s.P = ViewPosition(v, PixelNdc(v, gl_FragCoord.xy), depth);
    s.world = (v.inv_view * vec4(s.P, 1.0)).xyz;
    vec3 L,radiance;float shadow_cone,d;
    if(!PrepareLightView(l,v,s.world,s.P,pass.f1.xyz,pass.f2.xyz,L,radiance,shadow_cone,d))discard;
    vec4 g_normal = ImgFetch(IMG_NORMAL, pixel);
    vec4 g_material = ImgFetch(IMG_MATERIAL, pixel);
    s.N = DecodeNormal(g_normal.xyz);
    s.roughness = g_material.x;
    s.specular = g_material.y;
    s.material_u = g_material.z;
    s.translucency = g_material.w;
    vec3 diffuse;
    vec3 specular;
#if defined(PT_RT_SHADOWS) || defined(PT_RT_CONTACT)
    g_contact_reach = pass.f0.x;
    g_contact_legacy = pass.f0.y > 0.5;
#endif
    if (!ShadeLight(l,v,s,pass.ids.z!=0u,gl_FragCoord.xy,L,radiance,shadow_cone,d,diffuse,specular)) {
        discard;
    }
    out_diffuse = vec4(diffuse, 1.0);
    out_specular = vec4(specular, 1.0);
}
