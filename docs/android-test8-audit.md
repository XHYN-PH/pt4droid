# test.8 implementation and validation

The base is the published `pt4droid-v0.2.1-test.7` tag, commit `3e3e77f8d13076723ce1547446664b50b72a0511`. This update avoids repeated rendering/loading work and adds a capability-based texture fallback.

## Correctness safeguards

- Stable atlas assignment remaps only the lights accepted by the existing allocator. If old slot ownership fragments the atlas, the entire original packing is used. Exact view/caster states still decide reuse; pointer reuse after destruction and atlas replacement invalidate cached maps.
- Alpha dependencies include the effective redirected/enhanced image and anisotropic-filtering setting. Images remain immutable. Skin animation conservatively prevents reuse.
- Light scissoring bounds the supported cone while retaining the old raster volume. Unusual cone/range values and plane crossings use the existing bounds. Rejection uses the same attenuation, area, cone and radiance calculations before surface fetches.
- Particle index sorting includes invisible quads, preserving the former equal-depth order. Only unlit world rain is culled; shared lighting, refraction, flare and screen paths stay intact. The padded four-corner check conservatively keeps edge and non-finite cases.
- Upload data is copied into owned staging storage before source vectors disappear. A batch's storage is released only after its submission completes. Materials flush pending uploads before rendering. Cubemap faces and mip levels retain their order. Failed allocations and submissions do not publish a completed texture; failed batch submission stops rendering instead of silently sampling incomplete uploads.
- Texture creation validates dimensions, mip counts and byte lengths. The Vulkan submit helper checks queue success before waiting on its fence, avoiding a wait on a failed submission. Deferred-upload timing is not counted twice.
- Lazy optional targets remain allocated after disabling an effect. Unchanged frames perform no allocation, device-idle wait or descriptor rewrite. Partial allocation or initialization failure rolls back newly made targets.

## Test scope

The existing 15 host suites cover imports, save deletion and locks, storage permissions, Bluetooth, touch/editor input, frame limiting/look behavior, graphics settings, sensors, thermal controls, logging, menu exit and renderer synchronization/cache checks.

Four additional suites exercise production implementations:

1. Stable atlas permutations, fragmented layouts, spherical-cone containment, projected scissor samples, four-corner visibility, alpha dependencies and exact index-sort permutations.
2. 8,800 comparisons against an independent decoder across multi-block/non-multiple-of-four images and all BC7 modes. Separate golden cases cover BC1 alpha, BC2 explicit alpha and reversed BC3 endpoints, where the independent reference has different semantics or an error. BC1/3/5 reference interpolation permits a one-byte rounding difference; BC7 comparisons are exact.
3. 1,500 mixed rain/lit/liquid/luminance shapes compare complete production quad payloads and blend order with test.7, with visibility culling disabled for the comparison.
4. Instrumented production texture creation records native byte equality, zero supported-format conversion, sRGB/cubemap/mip conversion, staging limits, nested batches, memory caps and allocation/queue failures. Instrumented production optional-target creation covers 8,000 steady frames, enabling effects later, retaining storage, all 18 allocation-failure positions, initialization failure and 1-pixel dimensions.

These are host and packaging checks. No physical Android GPU was connected for validation layers, screenshot comparisons, sustained thermals or FPS measurements. Native compilation and Android lint/package results are recorded in the final validation record; the source patch is checked against the exact base.

## Boundaries

Per-format BC fallback preserves the Adreno upload path where the driver supports it. It can increase memory and loading work on a device that needs conversion. Compatibility remains experimental and does not bypass other Vulkan requirements. It can refuse textures at memory limits; an actual device log is needed to diagnose those cases.

The new refinements have no measured phone FPS claim. Shadow reuse still depends on unchanged caster geometry; scenes dominated by animation may have few cache hits. The upload change primarily targets loading pauses. Fog precomputation changes floating-point evaluation order slightly and still needs an actual image comparison. No global reduced-precision format, dynamic-resolution controller, graphics preset downgrade or tile-memory rendering path was introduced.
