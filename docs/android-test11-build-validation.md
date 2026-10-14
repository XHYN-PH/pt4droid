# Test 11 build validation

Release: **0.2.1-test.11**, ARM64, version code **53**. Built from public commit `877b82beccecb8662537a19829a69e565e026b98` with no local source changes.

| Check | Result |
| --- | --- |
| Shadows | Production recording checks passed: template copies, invalidation, sharing, fallback and adaptive paths. |
| VFX and streaming | 240,000 cached/live expression comparisons passed; per-texture readiness, priority, cancellation and budget reclamation passed. |
| Texture uploads | Upload byte and fallback checks passed; production submit/retire code passed fake-fence lifetime checks under ASan/UBSan. |
| Android display | 25 Java menu/display checks passed, including exact/multiple selection and unsupported sustained mode. |
| Compilation | All ARM64 Release native code and shaders, plus Android Java compilation, passed. |
| Android lint | 0 errors, 0 fatal findings; 73 warnings remain. |
| Signing | Existing test certificate matched; APK Signature Scheme v2 verified. |
| Package | Android 10+ / target API 35; 19 ARM64 libraries and 100 assets matched the staged build byte for byte; ZIP integrity and 16 KB ZIP/ELF alignment passed. |

APK: `pt4droid-v0.2.1-test.11-arm64.apk` (90,295,137 bytes).

SHA-256: `35a4afd5c075e23c962d00d7ba100835261078062b842485e44263ea46e8b894`

No physical phone or GPU is attached. These checks do not measure FPS, thermal behavior or visual correctness on hardware. Compare the same route, settings and driver with Test 10, including moving/mirror shadows, probe lighting, VFX, stage transitions and a long warm session. Existing Mali/driver feature requirements remain.
