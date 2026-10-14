vec3 EvaluateSh(Probe p, vec3 n) {
    vec3 e = p.sh[0].rgb;
    e += (p.sh[1].rgb * n.y + p.sh[2].rgb * n.z + p.sh[3].rgb * n.x);
    e += (p.sh[4].rgb * (n.x * n.y) + p.sh[5].rgb * (n.y * n.z) + p.sh[6].rgb * (3.0 * n.z * n.z - 1.0));
    e += (p.sh[7].rgb * (n.x * n.z) + p.sh[8].rgb * (n.x * n.x - n.y * n.y));
    return e;
}

// SH_SphereMap (rendering.md 5): an atlas texel at cell coordinate t in [-1, 1]^2 holds E at the view direction of a Lambert
// azimuthal equal area map around -z
vec3 SphereMapDirection(vec2 t) {
    float len2 = dot(t, t);
    float z = min(1.0, 2.0 * len2 - 1.0);
    vec2 xy = len2 > 0.0 ? vec2(-t.x, t.y) * inversesqrt(len2) * sqrt(max(0.0, 1.0 - z * z)) : vec2(0.0);
    return vec3(xy, z);
}

vec3 SphereMapTexel(Probe p, View v, ivec2 texel) {
    // the 16x16 cell's texel centre; the vertex shader flips v (inTexcoord.y = 1 - 2 v)
    vec2 s = (vec2(texel) + 0.5) / 16.0;
    vec3 d = SphereMapDirection(vec2(2.0 * s.x - 1.0, 1.0 - 2.0 * s.y));
    vec3 w = mat3(v.inv_view) * d;
    return EvaluateSh(p, vec3(-w.x, -w.z, w.y));
}

