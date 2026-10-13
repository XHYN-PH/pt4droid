# pt4droid 0.2.1-test.8

This release implements refinements 1–5 from the test.7 audit. Resolution stays fixed and existing graphics options remain available.

- **Shadow reuse:** selected lights keep stable atlas slots. Unchanged alpha-tested casters can reuse their shadows. Material flags, texture redirects, enhanced replacements and filtering changes invalidate their dependencies. Moving geometry, light transforms and animated skin still update correctly; skinned casters retain per-frame rendering. Selection limits and mirror caster sets are preserved.
- **Lighting:** conservative spot-cone bounds restrict the light's screen rectangle. Camera/near-plane crossings keep the full rectangle. Deferred lighting rejects pixels outside the original range, cone or area before fetching their normal and material data; the light equations and forward rendering remain.
- **Particles:** rain constants and texture lookups move out of the particle loop. Sorting moves small indices instead of full quad payloads. Offscreen rain quads can be omitted after sorting, using all four corners and a generous edge margin. Simulation, random values, emitted lights, shared lighting boxes, liquids, flares and screen effects are preserved.
- **Loading:** stage texture reads use two bounded workers and overlap geometry preparation. Uploads share up to 16 MiB of staging storage and 64 textures per submission, with synchronous completion before rendering. Oversized textures use the original individual path. Prepared compressed texture data is capped at 96 MiB.
- **Composition and buffers:** fog coefficients shared by every pixel are computed once per frame. AO, depth-of-field and motion-blur targets allocate only when enabled; motion data required by temporal upscaling is retained. Once allocated, targets persist until resize to avoid repeated allocation when settings change.

## Experimental texture compatibility

Unsupported BC1, BC2, BC3, BC4, BC5 and BC7 formats can decode into supported RGBA/R/RG formats. The entry point covers game FTEX textures, generated BC7 replacements, mip chains and cubemaps. sRGB interpretation, alpha and BC5's two normal-map channels are preserved. Other Vulkan requirements and driver/API restrictions still apply.

**Supported Adreno formats keep their native compressed uploads.** Compatibility decoding and memory-budget queries execute only for unsupported formats. The tests explicitly check zero conversion calls and byte-identical native uploads on the supported-format path. This does not establish an FPS result for the other renderer refinements.

Decoded textures are retained by the existing session texture cache. Expansion is bounded to 128 MiB per texture and 2 GiB per session, with an additional device-budget check and reserve when the driver exposes memory budgets. If conversion cannot fit or a format remains unsupported, the renderer logs the reason and refuses that texture. No mip levels are silently dropped. There is no new persistent disk conversion cache.

Mali/PanVK and other unsupported-BC devices remain **experimental**. The bundled PanVK driver is unchanged and requires Android 15+. A device must still provide the renderer's other Vulkan capabilities. No actual Mali or Xclipse gameplay benchmark was available.

## Updating and verification

Install over test.7 to retain game files, saves, settings and layouts. Package: `com.ptport.app.test`; version code: **50**; ARM64. The existing test signing key is reused. Game files and the private key are not included in the source kit.

The build record and audit accompany the APK. Phone comparisons should use the same save, driver, preset, render scale and FPS cap. Check slow turns, doors, the flashlight, mirror, rain and later scripted scenes, then compare timing logs and longer sessions. Dynamic resolution and the separate tile-memory renderer experiment are not included.

## Verified release APK

- ARM64, Android 10+, package `com.ptport.app.test`, version code **50**.
- APK size: **90,244,894 bytes**.
- SHA-256: `1d90b5f3bc9123a256320e39b96da8706b352fa42c845e5a3a40a5ff65cb6f4f`.
- Uses the Test 7 signing certificate. Install over Test 7 to retain game files, saves, settings and layouts.
- All **19** host regression suites, native ARM64 compilation and Android release compilation/package passed. Release lint: **0 errors**, **73 warnings**. Signature, package/version, staged library/asset equality and 16 KB ELF/APK alignment passed.
- See `docs/android-test8-build-validation.md` and `docs/android-test8-audit.md` for the build record and testing limits. No physical-phone FPS claim is made.
