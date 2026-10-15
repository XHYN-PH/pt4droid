# Test 13 build and verification

Prepared 15 October 2026 (Asia/Manila). This record describes the Test 13 device-test candidate built before source publication.

- Base source: `f5feb2c3668fdd0f28dd41250d4ad7e1b233f651` (Test 12 default branch).
- APK: `pt4droid-v0.2.1-test.13-arm64.apk`; version 0.2.1-test.13, code 55.
- Application ID: `com.ptport.app.test`; ARM64; minimum Android 10/API 29, target API 35.
- APK SHA-256: `df9137805a6351d53d1d6dbb1945dfde7bcfa3d434a764d4d603b9a83ab42411`.
- Signature verifies and matches the published Test 12 APK, permitting an in-place update with data retained.
- Release APK is not debuggable. All 19 native libraries have 16 KiB LOAD alignment; APK zip alignment passes.
- All 72 packaged shaders match the verified build output and pass Vulkan 1.3 SPIR-V validation. Native ClipDistance and feature-independent clipping variants were checked separately.
- Both bundled Vulkan driver binaries match Test 12 byte for byte. No game archives, FTEX/FMDL assets or signing keys are packaged.
- Full ARM64 native build, Android Java compilation, APK assembly and release lint pass. Lint reports 0 errors and 73 warnings; no fatal issue blocks the build.

## Focused behavior checks

- Compatibility: native bypass with no conversion/cache IO; exact conversion output across supported BC fallback formats, signed channels, alpha, odd dimensions, mip tails and six cube faces; bounded payloads; cache checksums, versions, truncation and concurrent readers.
- Residency/upload recording: active images protected; stable descriptor slots; byte-identical restoration; budget pressure; failure isolation; valid resource fallback retained when allocation/submission fails; aligned small mip staging.
- Address/undefined-behavior sanitizers pass for conversion/cache and production residency/resource boundary fixtures. LeakSanitizer is unavailable in this managed host; leak checks are not claimed.
- VFX: 240,000 cached/live expression comparisons, generation wrap/effect switches; two sets of 1,500 mixed local/world particle workloads match Test 12 payload, ordering and lighting exactly.
- Probes: 100,000 affine-transform/blend-weight comparisons; maximum coordinate difference 0.00003815 and blend-weight difference 0.00002605 in the fixture. Full float precision is retained; GPU pixel identity is not claimed.
- Streaming/preparation: individual readiness, bounded memory, priority and cancellation; pacing recovery and unchanged thermal FPS caps.
- Collision: production reuse condition tested for unchanged geometry, moved transforms, discarded-worker state, explicit invalidation, walk surfaces, labels and world identity.
- Android: 45 graphics/refresh assertions, 25 sensor/thermal lifecycle checks and 16 first-run storage checks. Every tested FPS target retains its selected cap; ignored refresh requests have bounded recovery.
- The source patch applies to the clean base. All 35 changed/new files then match the working source byte for byte.

Reproduce the focused suite after configuring Android dependencies:

```bash
python3 tools/android/tests/check_test13_refinements.py
```

GPU visuals, actual frame-time improvements, long-session thermals and PanVK WSI behavior still require testing on phones. Keep the build a prerelease until those device checks pass. These checks were completed before source publication. This repository update contains the same implementation; only documentation and the release-version marker were subsequently updated.
