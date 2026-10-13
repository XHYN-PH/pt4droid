# Test 10 build validation

APK: `pt4droid-v0.2.1-test.10-arm64.apk`

| Check | Result |
| --- | --- |
| Package / version | `com.ptport.app.test` / `0.2.1-test.10` / code **52** |
| Platform | ARM64; minimum API 29, target API 35 |
| APK size | **90,268,898 bytes** |
| Host regressions | **23 scripts passed** |
| Native compilation | ARM64 Release passed |
| Android packaging | Signed release and release lint passed |
| Lint | **0 errors**, **73 existing warnings**, matching Test 9 |
| Signature | APK v2 verified; same certificate as Test 9 |
| Packaged content | All **19 libraries** and **99 assets** match staged bytes |
| Integrity / alignment | ZIP integrity, 16 KB ELF LOAD segments and APK alignment passed |

APK SHA-256: `815c1de9bee73a1e27bfc5fefabd4308e3efbbf8cee868e87d66cc4c5bdb2ddc`

Signing certificate SHA-256: `d17b8e79ff97002a76b23f99c105fb002aeea01ba12734130293ab48b3e124af`. Matching test installations can update without clearing game files, saves, settings or layouts.

## Source and toolchain

Built from clean, published commit `03aae56342497f1d75678cc52de1e266f1f69037`, runtime build ID `03aae56`. The complete published tree matched the reviewed local source before the final rebuild. The [source manifest](android-test10-source-manifest.json) records all 31 changed production, test and build-tool files. The release adds this verification record without changing compiled code.

Toolchain: NDK r27d, SDK/build-tools 35, JDK 17, CMake 3.28.4, Gradle 8.12, Android Gradle plugin 8.7.3. Bundled driver downloads were checked against the repository's pinned hashes. Game data and signing credentials are excluded from the APK and repository.

## Regression coverage

Checks cover frame diagnostics, preparation ownership/budget reclamation, probe bounds, shadow invalidation/recording, constant VFX evaluation, texture decoding/native uploads, Android storage migration, controls, settings, driver import, pacing, logging and menu return. Production VFX evaluation passed 80,000 bit-exact comparisons; particle output matched 1,500 Test 7 and 1,500 Test 8 reference cases with constant folding enabled.

The new suite passed AddressSanitizer and UndefinedBehaviorSanitizer. LeakSanitizer was disabled because this container cannot inspect its process threads; preparation ownership/reclamation checks passed. Import fixtures use `org.json` 20240303 and the staged Turnip library. Historical comparisons require the repository's Test 7/8 tags.

## Device testing limits

No physical Android GPU was connected. Host checks and builds do not establish a phone FPS gain or complete device compatibility. Compare the same scene, camera route, settings, driver, render scale and FPS cap after warmup, including longer sessions.

Mali/PanVK and Xclipse remain experimental. Test 10 implements refinements 1–6; it does not remove the Vulkan feature requirements behind the reported Mali Android 14 startup failure. Bundled PanVK still requires Android 15+ and compatible hardware.
