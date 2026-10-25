# pt4droid Test 9 refinements

Test 9 implements the four refinements from the Test 8 performance review and fixes the reported Android data-folder startup failure. Package `com.ptport.app.test`, ARM64, version `0.2.1-test.9`, version code **51**.

## Changes

1. **Avoid discarded texture preparation.** Stage loading derives its texture list from the same material builder used for GPU uploads. It skips already loaded texture variants and disables preparation when mods are active. The two preparation workers reserve the 96 MiB compressed-payload budget from FTEX mip descriptors before reading streams or inflating them. Prepared data is immutable and can serve both raw/linear and sRGB uploads without rereading the archive or changing its flags. GPU texture formats, mip levels, BC compatibility limits and upload batching remain as before.
2. **Adapt shadow clearing to useful saved work.** Android can clear and redraw the atlas when at least three quarters of its active canonical tiles are dirty and retaining the other tiles saves at most one eighth of both draw calls and submitted indices. Expensive cached geometry and fully unchanged maps still reuse their contents. This avoids forcing a depth LOAD solely for a cheap cached tile. `PT_SHADOW_ADAPTIVE=0` selects the previous policy for diagnostic comparisons. Actual GPU benefit depends on the driver and scene.
3. **Share equivalent main/mirror shadow maps.** Copies of the same light share an atlas lookup only when the actual accepted Draw pointers, projections, eye/hemisphere parameters, biases, culling and tile dimensions match. Both hemispheres of a point light must match together. GPU light references are rewritten before frame upload. A tile that stops sharing is invalidated and redrawn; skin can share within a frame but never becomes eligible for static previous-frame reuse. `PT_SHADOW_SHARE=0` disables sharing for diagnostics.
4. **Reduce rain preparation memory traffic.** Sorting retains 8-byte depth/index entries while quad payloads use separate reusable storage. Fully offscreen world rain keeps its depth entry, preserving equal-depth alpha ordering, while skipping payload, UV-animation and subsequent preparation. Fully culled shapes return early. The attribute evaluation, simulation, random sequence, lighting, mesh particles, liquid and screen-effect paths remain intact. Existing conservative four-corner culling and its `PT_VFX_CULL=0` diagnostic switch remain.

## Android migration marker fix

The reported folder passed its write probe but failed to publish `.pt-user-data-migration-v1` with `Operation not permitted`. The old marker used rename-without-replacement and an atomic hard-link fallback. These operations can be unavailable on Android shared storage.

Android now records successful completion by atomically creating an empty `.pt-user-data-migration-v1.done` directory. This requires neither hard links nor a special rename operation. Existing valid file markers remain accepted on upgrades; both formats can be read on other platforms. A linked, non-directory or nonempty completion directory is rejected. Migration completion is written only after successful import, and a legacy completion directory cannot complete a different destination.

Existing destination files still win, legacy data remains untouched, failed imports remain retryable, and deleting a save after completion does not cause it to be restored from the old folder. Actual data-file publication retains its existing no-overwrite checks and still reports failures rather than declaring an incomplete import successful. Install this update over the existing app to retain its data; the fix does not require deleting settings, saves or game files.

## Review and regression evidence

All **22** Android host regression scripts passed. Added or extended checks include:

- The real Test 8 migration failure reproduced with unsupported rename/link operations (`ENOSYS`, `EINVAL`, `EPERM`); the Android fix passes fresh installation, existing settings/save preservation, repeat launch, old markers, invalid and linked markers, failed import and retry.
- Production shadow recording covers unchanged maps, partial/full clears, the integrated adaptive branch and opt-out, expensive cached geometry, shared/unshared spot lights, whole point lights, mismatched hemispheres, hidden-view masks and animated skin. Shader inspection confirms atlas lookup uses the rewritten shadow rectangle.
- **2,400** material cases preserve Test 8 GPU materials and requested texture variants exactly, and the preparation list matches the upload selection.
- Production FTEX/texture-manager checks reject excess budget before stream reads, retain native BC bytes, share prepared data for either raw/sRGB upload order, preserve flags, reject malformed descriptors and enforce a concurrent byte budget.
- Production particle output matches the Test 7 culling-disabled reference and Test 8 culling-enabled reference exactly, including payloads and ordering. Existing decoder, upload, controls, settings, frame pacing, logging, menu/return, driver import and optional-target checks passed.


## Device evidence and limits

The supplied Snapdragon 8 Gen 3/Adreno 750 capture at 1200×540 is GPU limited in the heavy mirror scene: about 39–43 FPS with GPU frames around 22–24 ms, including roughly 9.75 ms of shadows in the cited sample. Test 8's hallway loading preparation was also longer than Test 7 while later texture reads remained similar. These observations motivate the changes; the captures use different floors and frame caps and do not prove a matched version regression.

The other supplied log identifies **Xclipse 960**, not Mali. Its lighter captured scene does not benchmark the same heavy mirror workload. Native BC uploads remain the supported-format path on compatible Adreno drivers. No resolution, quality preset, saved FPS cap or driver default is changed by Test 9.

[StoneColdOS/pt-pc PR #1](https://github.com/StoneColdOS/pt-pc/pull/1), `Mali g57 vk11`, was reviewed at head `1821adf3f5df41cccbe027db3c3881730576747a`. It contains preparatory Vulkan/device/submission changes and planning documents, not a complete compatible Vulkan 1.1 renderer. It is not merged into this release. Existing Mali/PanVK and unsupported-BC compatibility remains experimental, and Vulkan 1.3 plus the renderer's other required features still apply.

No physical phone or Android GPU was connected for this build. Compare Test 8 and Test 9 on the same save, camera position, driver, graphics settings, render scale and FPS cap after warmup. Check the mirror, flashlight, moving characters, rain, scene changes and a longer session; enable logs for GPU timing and the new shadow-sharing/cache-redraw counters. An FPS gain is not claimed without that comparison.
