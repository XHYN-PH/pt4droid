#pragma once
// Independent test.6 reference: preserve stable ordering and probe weighting.
#include <algorithm>
#include <cmath>
#include <vector>
#include "engine/render/scene_lighting.h"
#include "engine/vfx/vfx_system.h"
namespace reference {
using namespace pt;
glm::vec3 ProbeAmbient(const SceneLighting& lighting, const glm::vec3& p) {
    glm::vec3 sum(0.0f);
    float total = 0.0f;
    const SceneProbe* nearest = nullptr;
    float nearest_d = 1e30f;
    for (const SceneProbe& probe : lighting.probes) {
        const glm::vec3 q = glm::vec3(probe.world_to_box * glm::vec4(p, 1.0f));
        const glm::vec3 fp = glm::clamp((glm::vec3(1.0f) - q) * probe.positive_scale, 0.0f, 1.0f);
        const glm::vec3 fn = glm::clamp((glm::vec3(1.0f) + q) * probe.negative_scale, 0.0f, 1.0f);
        const glm::vec3 f = glm::min(fp, fn);
        const float w = probe.weight * f.x * f.y * f.z;
        if (w > 0.0f) {
            sum += probe.sh[0] * w;
            total += w;
        }
        const float d = glm::length(glm::max(glm::abs(q) - glm::vec3(1.0f), glm::vec3(0.0f)));
        if (d < nearest_d) {
            nearest_d = d;
            nearest = &probe;
        }
    }
    if (total > 0.0f) {
        return sum / total;
    }
    return nearest ? nearest->sh[0] : glm::vec3(0.02f);
}
vfx::LightBlock LightBlockAt(const SceneLighting& lighting, const glm::vec3& center, const glm::vec3& half) {
    vfx::LightBlock block;
    const float radius = std::max({half.x, half.y, half.z});
    struct Candidate {
        const SceneLight* light = nullptr;
        float score = 0.0f;
        float cone = 1.0f;
    };
    std::vector<Candidate> candidates;
    for (const SceneLight& l : lighting.lights) {
        if ((l.type != LightType::Point && l.type != LightType::Spot) || (l.hidden_views & 1u) != 0 || l.outer_range <= 0.0f) {
            continue;
        }
        const glm::vec3 reach(l.outer_range + l.dimmer);
        if (glm::any(glm::greaterThan(l.position - reach, center + half)) || glm::any(glm::lessThan(l.position + reach, center - half))) {
            continue;
        }
        const glm::vec3 to = center - l.position;
        const float dist = glm::length(to);
        float cone = 1.0f;
        if (l.type == LightType::Spot) {
            const float cos_a = dist > 1.0e-6f ? glm::dot(glm::normalize(l.direction), to / dist) : 1.0f;
            cone = std::pow(std::clamp((cos_a - l.cos_outer) * l.inv_cone_range, 0.0f, 1.0f), 2.7182817f);
        }
        const float d = std::max(dist, l.inner_range);
        const float r = l.outer_range + radius;
        const float falloff = 1.0f / std::max(d * d, 1.0e-12f) - d * d / std::max(r * r * r * r, 1.0e-12f);
        candidates.push_back({&l, glm::length(l.intensity) * cone * falloff, cone});
    }
    std::stable_sort(candidates.begin(), candidates.end(), [](const Candidate& a, const Candidate& b) { return a.score > b.score; });
    for (size_t k = 0; k < 3 && k < candidates.size(); ++k) {
        const SceneLight& l = *candidates[k].light;
        const float range = l.outer_range + l.dimmer;
        block.position[block.count] = l.position;
        block.color[block.count] = l.intensity * candidates[k].cone;
        block.inv_r4[block.count] = 1.0f / std::max(range * range * range * range, 1.0e-12f);
        ++block.count;
    }
    std::vector<const SceneProbe*> probes;
    for (const SceneProbe& p : lighting.probes) {
        const glm::vec3 q = glm::vec3(p.world_to_box * glm::vec4(center, 1.0f));
        if (glm::all(glm::lessThanEqual(glm::abs(q), glm::vec3(1.0f + 1.0e-3f)))) {
            probes.push_back(&p);
        }
    }
    std::stable_sort(probes.begin(), probes.end(), [](const SceneProbe* a, const SceneProbe* b) { return a->priority < b->priority; });
    float weights[3] = {0.0f, 0.0f, 0.0f};
    float left = 1.0f;
    float total = 0.0f;
    for (size_t k = 0; k < 3 && k < probes.size(); ++k) {
        const SceneProbe& p = *probes[k];
        const glm::vec3 q = glm::vec3(p.world_to_box * glm::vec4(center, 1.0f));
        const glm::vec3 f = glm::min(glm::clamp((1.0f - q) * p.positive_scale, 0.0f, 1.0f), glm::clamp((1.0f + q) * p.negative_scale, 0.0f, 1.0f));
        weights[k] = probes.size() == 1 ? 1.0f : left * p.weight * f.x * f.y * f.z;
        left -= weights[k];
        total += weights[k];
    }
    if (total >= 1.0e-5f) {
        for (size_t k = 0; k < 3 && k < probes.size(); ++k) {
            const std::array<glm::vec3, 9>& sh = probes[k]->sh;
            block.sky += weights[k] / total * glm::max(sh[0] + sh[2] + 2.0f * sh[6], glm::vec3(0.0f));
        }
    } else {
        // without a probe holding the centre: the global SH light (type 4) in the original; the port has the nearest probe's
        block.sky = ProbeAmbient(lighting, center);
    }
    // m_localParam[1] is zero in every Prim_Poly_LitDP3_NS_VF draw of ending_fx_trace (976 draws over 20 frames), although the
    // ending has the atmosphere's moon: the scene has no directional GrLight for D4D500 to write, so the term stays 0 in P.T.
    block.directional = glm::vec3(0.0f);
    return block;
}
}
