# pt4droid 0.2.1-test.13

Test 13 release candidate prepared from Test 12. Keep it as a release candidate until Adreno and Mali device testing is complete. Install over Test 12 to retain app data and saves.

## Seven compatibility and rendering refinements

1. **Converted-texture residency:** track live converted images, protect textures used by visible models, shadow alpha tests, light masks, reflections and VFX, and release unused converted images under a bounded budget. Descriptor indices stay stable. Released images can be restored from their conversion cache. Reclamation waits for earlier GPU use to finish.
2. **Explicit compatibility texture size:** Original, 2048, 1024 or 512 px. Smaller settings select existing mip levels only for unsupported BC formats. Native supported textures keep their formats, mipmaps and detail. Changing this setting requires a game restart.
3. **Renderer-resource compatibility:** film-grain and effect-mask uploads use the compatibility converter. Replacement failures retain the previous valid image. Staging offsets remain aligned for small R8/RG8 mip levels.
4. **PanVK diagnostics/recovery:** name `VK_ERROR_INVALID_EXTERNAL_HANDLE` correctly, retry an advertised FIFO presentation mode once for that specific failure, and explain how to return to System Vulkan if Android buffer import still fails. This is a recovery attempt, not a verified fix for every PanVK/device combination.
5. **Earlier conversion preparation and disk cache:** existing bounded workers also prepare unsupported BC conversions. A versioned, content-keyed cache preserves exact decoded bytes with integrity checks. Cache IO uses fixed-size streaming buffers; native supported textures bypass it.
6. **Composition/probe work:** skip SH-map evaluation for offscreen probes, precompute the affine view-to-probe transform per draw, and remove invariant fog work from composition. Keep full float precision, probe order, interpolation and effects. The coordinate transform is numerically equivalent within float rounding; it is not claimed to be bit-identical on every GPU.
7. **Failure handling:** distinguish source, format/conversion, residency-budget and Vulkan upload failures. Cache permanent source/format failures and avoid repeatedly reading backing data while the active set cannot fit. Failed backing reads do not evict an unrelated protected image.

## Five completed fixes included in Test 13

- Continue bounded texture-preparation windows after taking a stage prefetch; prefetch no longer suppresses preparation of the remaining textures.
- Recover refresh requests after focus, surface and display changes for every FPS target, including Unlimited. Recovery attempts are bounded. The selected FPS cap and sustained-mode behavior remain unchanged.
- Warm opening-stage cutscene model/skeleton/simulation assets during loading, without triggering gameplay events. Log remaining model handover time.
- Reuse collision worlds when geometry, transforms, labels and destination worlds are unchanged. Retain a published transform snapshot so a discarded worker cannot falsely validate stale collision data.
- Reuse invariant VFX vertex calculations while keeping particle state, lighting and ordering live.

## Validation

The accompanying verification record contains actual build and APK results. Focused checks cover native compressed-byte preservation; conversion formats, signed channels, alpha, mipmaps and cube faces; corrupt caches and concurrency; residency lifetime/slot preservation; retained renderer fallback images; VFX output; collision invalidation; display selection and Android lifecycle.

Native Adreno texture uploads were checked for zero conversion and byte equality. This does not establish device-level FPS or visual results. Android can still choose a different display rate, and GPU/thermal limits still apply.

## Device acceptance before stable

- Adreno: compare Test 12 and Test 13 using the same route, driver, graphics and FPS target; check texture detail, mirrors, shadows and VFX.
- Mali-G57: start with Automatic/System Vulkan. Check missing surfaces, UI, particles, grain, masks, reflection cubes and mirrors. Keep Original detail for comparison; if the log reports that the active converted textures exceed memory, explicitly select 1024 or 512 px and restart.
- Check capped and Unlimited modes, pacing on/off, pause/resume and live FPS changes. Test longer sessions and game progression through cutscenes and transitions.
- PanVK remains experimental; a FIFO retry can still fail if the driver cannot import Android display buffers.

These source changes are proposed in the Test 13 pull request. The Test 13 APK was built and verified before this source update; device acceptance is still required before a stable release.
