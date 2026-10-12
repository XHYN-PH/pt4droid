# test.7 renderer refinements and verification

## Evidence and scope

`pt (5).log` identifies test.6 running on an imported Turnip Adreno 750 driver. Later gameplay samples show approximately 35–40 FPS and 24.66–27.64 ms average GPU work at 1006×452, with VSync enabled and a configured 144 FPS limit. Recent-frame shadow-stage values are around 12–15 ms and mirror values around 4 ms. Particle preparation ranges from 0.662 to 4.391 ms per rendered frame. No warning/error/fatal entries are recorded.

These measurements suggest GPU-side work deserves priority. They do not isolate a graphics preset or establish thermal throttling. The old total GPU values were averages while pass values described one recent frame; adding them was misleading. The old shadow-stage timing could also include ray structure construction. test.7 corrects both diagnostic ambiguities.

This update implements the six proposed refinements without automatic resolution changes, new quality effects or automatic preset changes. Dynamic resolution remains excluded.

## Implementation safeguards

1. `render_sync.h` maps known color, depth, sampled and transfer layouts to their necessary stage/access scopes. Read-after-write, write-after-read and write-after-write dependencies remain. General/unknown layouts use the conservative fallback. The scene's sampled images remain synchronized for vertex, fragment and compute access.
2. `shadow_cache.h` compares exact matrices, raster state and ordered caster states rather than relying on hashes. It is indexed by the current atlas tile, so reassigning a tile invalidates reuse when its content changes. Alpha-tested or skinned casters make that tile non-reusable. Mesh destruction, atlas replacement, shutdown and whole-area clears invalidate cached tiles. Partial updates use depth clears confined to dirty rectangles. No new device-idle waits were introduced into per-frame rendering.
3. `geometry_bounds.h` computes bounds from indexed triangles with signed vertex offsets. Invalid ranges/positions and non-affine transforms fall back safely. Transformed boxes include nonuniform scale, reflections, shear and a conservative margin. Skinned meshes retain their former bounds. Only shadow caster tests use the new submesh bounds.
4. Mirror occlusion uses the active per-frame camera occluder set already used for light selection. It rejects only a complete padded world-space enclosure. Mirrors touching an occluder's plane or partly extending outside its volume remain visible. The renderer retains the previous capture until a currently visible mirror updates again, matching the previous offscreen behavior.
5. `StableTop<T,3>` preserves source order on ties, and the total number of matching probes is retained for the one-probe weighting rule. Exact light blocks match the independent test.6 reference in randomized cases. Existing particle sort/blend/draw order remains in place. Scene light and preload lists reuse their storage.
6. Attachment invalidation is restricted to passes that fully overwrite their output. Partial mirror targets retain loads. Live depth, shadow, transparency and history data are stored/preserved where subsequently consumed. Empty effect-buffer passes are skipped using the same buffered-effect classification as the draw path, including liquid and legacy-liquid behavior.

`PT_SHADOW_CACHE=0`, `PT_SHADOW_BOUNDS=0` and `PT_MIRROR_OCCLUSION=0` retain developer comparison paths. These are process environment variables for native development; they are not added to the player-facing launcher.

## Diagnostics

Only successful, new GPU timing samples contribute to the ten-second GPU/pass averages. Unavailable samples are explicitly reported rather than presented as measured zero GPU cost. The ray setup timestamp divides the old shadow stage into structure preparation and shadow-map work. Average renderer CPU time and work counters are collected separately over the rendered frames. The timing CSV includes ray setup as its own column.

Logging Off retains the existing GPU-query gate and also bypasses the status-window timing and memory-budget collection. Diagnostic counters are corrected to count each recorded draw once.

## Verification and limits

- 15 host suites passed, including unchanged import/save/lifecycle/input coverage and the new production renderer checks.
- 12,000 full light-block comparisons, including ties, missing lights/probes and weighting edge cases, match test.6 exactly with zero allocations in the replacement light-block function.
- 8,000 affine bounds cases verify transformed geometry containment; signed/invalid indices, touching planes and non-affine fallback are covered.
- 3,000 cache mutations plus production-recording checks cover complete/partial clears, unchanged frames, point-half/view changes, moving casters, alpha streaming, skinning, caster removal, tile reassignment, invalidation and the cache-disabled fallback.
- Occluder checks cover a fully hidden mirror box, partial edge visibility, a wall-coincident box and an opened/disabled occluder.
- Native ARM64 compilation and Android SDK resource/Java/DEX packaging passed. The APK uses the test.6 certificate, code 49, and the same package ID. All 19 packaged ARM64 libraries and staged assets match the native build; APK and ELF alignment are verified.
- Gradle lint is unavailable because the distribution download is blocked. There was no physical Android GPU for validation-layer testing, visual comparisons or performance measurements.

## Phone comparison

Use the same driver, preset, resolution and save position for test.6 and test.7. Compare a static hallway view, turning with the flashlight, approaching the bathroom mirror, opening/closing doors, and scenes with characters, blood/liquid and particles. Check the ending and repeated Return to menu/Start cycles too. Compare a warmed-up scene and a longer session separately.

Enable logs for a short diagnostic run to capture the new averages and cache counters, then disable them for normal performance comparison. GPU timing queries themselves can influence measurements. A larger FPS number in a different scene is not a controlled before/after result.
