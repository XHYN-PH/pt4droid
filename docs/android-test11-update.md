# Test 11 implementation

Based on the Adreno 750 Test 10 log. Existing graphics choices, render scale and effect order are preserved.

| Refinement | Change | Boundary |
| --- | --- | --- |
| Shadows | Exact immutable pose snapshots; static depth templates for stable lights with at least 32 static and one moving caster. | Projection, pose, accepted casters, alpha, cull and bias changes invalidate reuse. Invalid bone ranges remain dynamic. Extra atlas limited to 64 MiB. |
| Probes | Per-view 16×16 SH maps in a 512×256 RGBA32F atlas; four explicit texel fetches and float interpolation. | Original shader for unsupported format/allocation/pipeline and ray AO. No half-precision maps. |
| VFX | Lazily caches expressions uniform within a shape/instance/view, and resolves texture handles once per preparation. | Particle age, randomness, spawn world, position and order remain live. Scratch reset per shape. |
| Uploads | Three same-queue commands retain staging until completion fences signal. Polls completed jobs; waits when the ring is full. | Up to 96 MiB in-flight staging. Normal batches 16 MiB; individual textures above 32 MiB keep synchronous submission. Shutdown drains before destroying images. |
| Preparation | Two readers publish per-texture futures across a sliding placement window. First use prioritizes its queued read. | Shared 96 MiB stage preparation budget, last-variant reclamation, mod path and scoped cancellation/cleanup. Logs budget/read/unscheduled fallbacks. |
| Diagnostics | Separates recording completion, queue submit and native/Swappy present CPU times; actual SDL display Hz/interval and upload ring waits. | Respects logging; timings alone do not establish an FPS improvement. |
| Bandwidth | Removes the fullscreen composition depth attachment, since it samples depth without depth testing/writing. | Forward/VFX depth passes, post order, effects and color formats remain unchanged. |

One FPS/display target is sent when the active cap changes, including menu transitions, with or without optional Swappy pacing. Selects the same-resolution exact rate, then the lowest supported integer multiple, then a supported fallback. Requests affect the app window; Android controls the actual rate.

Sustained mode uses `Window.setSustainedPerformanceMode` when supported. It does not reduce the FPS cap or graphics. Thermal status is recorded independently of the toggle; Android still controls clocks and temperature.

Focused production checks cover shadow reuse/copies/invalidation, exact VFX payload/order comparisons, cached/live expression equality, individual readiness/priority/cancellation and preparation leases, texture upload bytes and completion-fence ownership, and Android display selection. The upload ring runs under AddressSanitizer and UndefinedBehaviorSanitizer. All shaders and native code must compile in the ARM64 Release build; APK signing, alignment, version, contents and lint are verified after packaging.

No physical Adreno/Mali device is attached. Compare the same route/settings/driver with Test 10, including animated/mirror shadows, lighting, liquid/smoke and a long warm session. `PT_PROBE_MAP=0` and `PT_SHADOW_STATIC=0` retain the original paths for developer A/B checks.
