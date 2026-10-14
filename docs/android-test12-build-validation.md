# Test 12 validation

The supplied Adreno 750 Test 11 logs show about 30 FPS with Android pacing enabled despite 8–12 ms GPU time, and about 60 FPS with the native limiter. These observations motivated window binding, actual display-rate tracking, and a guarded native fallback. They do not measure Test 12 performance.

## Focused checks

- Production pacing policy: blocked presentation, incompatible refresh, GPU-bound frames, unavailable GPU timing, target changes, and isolated loading spikes.
- Android Java: 27 graphics/refresh checks, 25 sensor/thermal/display lifecycle checks, and 16 storage checks.
- VFX: 240,000 bit-exact cached/live evaluations, plus actual shape generation wrap and effect switches.
- CPU preparation: eight-result queue limit, two readers, byte-budget refusal/reclamation, failed loads, cancellation, and worker joining.
- Rendering: production shadow recording tests pass for invalidation, sharing, dynamic skin, and fallback paths; 10,000 fog-factor comparisons pass within floating-point tolerance.
- All 72 production shader modules compile and validate for Vulkan 1.3. Mali variants contain neither the `ClipDistance` capability nor its builtin; capable devices retain native clipping variants.
- Fresh ARM64 native build and Android release Java compilation pass. Release lint reports **0 errors, 0 fatal issues, 73 warnings**.

Reproduce the focused checks after building native shaders:

```bash
python tools/android/tests/check_test12_refinements.py
python tools/android/tests/check_renderer_refinements.py
```

Set `JAVA_HOME` to JDK 17 and `GLSLC` to the NDK shader compiler. The scripts use the dependency headers in `build/android/_deps`.

## Device testing still needed

Mali-G57 Android 14 startup, mirror and point-shadow visuals, display transitions, and sustained phone performance require hardware testing. The fallback removes the two feature blockers in the supplied Mali log; other renderer/driver requirements remain. No FPS improvement is claimed from host checks.

Before publishing, the signed APK is checked for package/version, update-compatible certificate, archive integrity, staged assets/libraries, and 16 KB alignment.
