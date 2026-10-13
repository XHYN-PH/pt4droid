# pt4droid 0.2.1-test.9 — verified ARM64 release

APK: `pt4droid-v0.2.1-test.9-arm64.apk`

- Package: `com.ptport.app.test`; version code **51**; minimum Android API **29**, target **35**.
- Size: **90,250,654 bytes**.
- SHA-256: `6c8da1f12b8456ef673461ae3bb90cc9f032319cd53060c1ce99005310c09dd5`.
- Signing certificate SHA-256: `d17b8e79ff97002a76b23f99c105fb002aeea01ba12734130293ab48b3e124af` — identical to Test 8. Install over the matching Test 8 app to retain game files, settings, saves and layouts.
- Native ARM64 Release compilation and signed Android release packaging passed. Toolchain: NDK **r27d (27.3.13750724)**, SDK/build-tools **35**, JDK **17**, CMake **3.28.3**, Gradle **8.12**, Android Gradle plugin **8.7.3**.
- All **22** host regression scripts passed, including real production texture preparation/material selection, shadow recording and opt-outs, particle payload comparisons, BC decoder/native uploads, Android storage migration, controls, settings, driver import, frame pacing, logging and menu/return checks.
- Release lint: **0 errors**, **73 existing warnings**.
- APK Signature Scheme v2 verified. Package, version, ARM64 architecture, ZIP integrity and APK alignment verified. All **19** staged native libraries and **99** staged assets exactly match the packaged bytes. Every packaged ELF LOAD segment has at least **16 KB** alignment.
- No game dump, signing private key or credentials are packaged. Dependency notices remain included.

## Source correspondence

Built from the clean, published commit `4a868c8ac6bb64a137cad95765917b9fc3c5b274` (runtime build ID `4a868c8`). All **30** files in the implementation/README/changelog publication were compared byte for byte with the GitHub branch before the final native rebuild. All **26** implementation files also match `docs/android-test9-source-manifest.json`; the remaining four publication files are documentation. The release tag adds this verification record without changing the compiled code.

The source is based on Test 8 (`0a124fa39dbb29134980fea32ee24f623fd81cbb`). It implements the four refinements and Android migration completion fix described in `docs/android-test9-update.md`. Graphics settings, fixed render scale, driver selection defaults and supported-format native BC uploads are retained.

## Test environment

Host regression checks use GCC 13, JDK 17, GLM and Vulkan headers. The importer fixture uses the official `org.json` **20240303** JAR through `PT_TEST_JSON_JAR` and the staged Turnip library through `PT_TEST_DRIVER_SO`. Historical comparison checks require the referenced repository commits and Test 7/Test 8 tags. Official Gradle/Maven dependencies were restored locally for this environment; production dependency versions and the repository's build configuration were not changed.

The storage fixture first reproduces the Test 8 marker error when rename/link publication is unsupported, then checks fresh installation, upgrades, existing settings and saves, repeated launch, malformed/linked markers, failed migration and retry against the actual Android implementation. Native data imports still refuse unsafe or failed publication rather than marking partial work complete.

## Device validation limits

No physical Android GPU was connected. Rendering appearance, installation/startup on the reported phone, sustained FPS and thermals still need device testing. The host checks and compilation do not establish an FPS gain. Use matching saves, camera positions, settings, drivers and frame caps for a warmed-up Test 8/Test 9 comparison.

Mali/PanVK and Xclipse compatibility remains experimental. The incomplete upstream Vulkan 1.1 pull request is not merged. The existing Vulkan 1.3 renderer and its required features still apply.
