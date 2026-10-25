# pt4droid 0.2.1-test.9

This release implements refinements 1–4 from the Test 8 review and fixes the Android data-folder startup error. It builds on Test 8: the lighting, fog, effect-target and texture-compatibility improvements remain included. Resolution stays fixed and existing graphics options remain available.

- **Shadow reuse:** Test 8's stable atlas slots and unchanged alpha-tested shadow reuse remain included. Test 9 adds adaptive atlas clearing when most tiles change and keeping a cheap cached tile would save little work. Equivalent main/mirror light maps can share a capture after exact projection and accepted-caster checks; point lights share both hemispheres together. Moving geometry, light transforms, alpha dependencies and animated skin still update correctly.
- **Lighting:** retained from Test 8. Conservative spot-cone bounds restrict the light's screen rectangle, and deferred lighting rejects pixels outside the original range, cone or area before fetching normal and material data. Camera/near-plane crossings keep the full rectangle; the light equations remain the same.
- **Particles:** Test 9 uses compact depth/index entries with separate reusable quad storage. Conservatively rejected offscreen rain skips payload and UV-animation preparation, while retaining its depth entry to preserve equal-depth alpha ordering. Test 8's rain constants and texture-lookup refinements remain included. Simulation, random values, emitted lights, shared lighting boxes, liquids, flares and screen effects are preserved.
- **Loading:** Test 9 prepares only textures selected by materials and checks the 96 MiB preparation budget before reading streams or inflating data. Prepared compressed data can serve both raw/linear and sRGB uploads without archive rereads or flag changes. Test 8's two bounded workers, overlapping geometry preparation and batched uploads remain included.
- **Composition and buffers:** retained from Test 8. Fog coefficients shared across pixels are computed once per frame. AO, depth-of-field and motion-blur targets allocate only when needed; motion data required by temporal upscaling is retained. Allocated targets persist until resize to avoid repeated allocation when settings change.
- **Startup fix:** Android records migration completion using an atomic empty directory, avoiding the unsupported rename/hard-link operation that caused the reported `Operation not permitted` error. Existing valid markers, settings and saves are retained. Failed data imports remain retryable, and deleting a save after migration does not restore it from the old folder.

## Experimental texture compatibility

Test 9 retains Test 8's fallback for unsupported BC1, BC2, BC3, BC4, BC5 and BC7 formats. Game FTEX textures, generated BC7 replacements, mip chains and cubemaps can decode into supported RGBA/R/RG formats. sRGB interpretation, alpha and BC5's two normal-map channels are preserved.

**Supported Adreno formats keep their native compressed uploads.** Compatibility decoding and memory-budget queries execute only for unsupported formats.

The existing limits remain 128 MiB per converted texture and 2 GiB per session, with the device-budget check and reserve when available. No mip levels are silently dropped. The bundled drivers and driver-selection defaults are unchanged.

Mali/PanVK and Xclipse compatibility remain **experimental**. Bundled PanVK requires Android 15+ and compatible hardware; the renderer still requires Vulkan 1.3 and its other required features. The upstream Mali Vulkan 1.1 pull request is incomplete and is not part of this release.

## Updating

Install over Test 8 to retain game files, saves, settings and layouts. Package: `com.ptport.app.test`; version code: **51**; ARM64. The existing test signing key is reused. The startup fix does not require clearing app data or reinstalling game files. Game files and the private signing key are not included.

Phone comparisons should use the same save, camera position, driver, preset, render scale and FPS cap after warmup. Check slow turns, doors, the flashlight, mirror, rain and later scripted scenes, then compare timing logs and longer sessions. Physical-phone rendering, sustained FPS and thermals still need testing.
