# pt4droid 0.2.1-test.8 — verified ARM64 release

APK: `pt4droid-v0.2.1-test.8-arm64.apk`

- Package: `com.ptport.app.test`; version code **50**; minimum Android API **29**, target **35**.
- Size: **90,244,894 bytes**.
- SHA-256: `1d90b5f3bc9123a256320e39b96da8706b352fa42c845e5a3a40a5ff65cb6f4f`.
- Signing certificate: `d17b8e79ff97002a76b23f99c105fb002aeea01ba12734130293ab48b3e124af` — identical to Test 7. Install over the matching Test 7 app to retain its data.
- Full native ARM64 build and Android release compilation/package completed. Native toolchain: NDK r27d; Android SDK 35; JDK 17; CMake 3.28.4; Gradle 8.12.
- All **19** host regression suites passed, including the production BC decoder comparisons, stable shadow atlas/scissor checks, particle payload comparisons, optional target allocation checks, and native texture upload checks. Supported formats retain byte-identical uploads and bypass conversion.
- Release lint: **0 errors**, **73 warnings**. Existing warnings remain.
- Signature, package/version, ARM64 architecture, ZIP integrity and APK alignment verified. All **19** staged native libraries and **99** staged assets match their APK bytes. Every packaged ELF LOAD segment has at least 16 KB alignment.
- BC decoder licensing is included; no game data or signing private key is packaged.

## Source

Built from `pt4droid-v0.2.1-test.7`, commit `3e3e77f8d13076723ce1547446664b50b72a0511`, with the 35-file Test 8 source update. Every changed source file matches the saved source manifest. This rebuild adds no new gameplay, renderer or control changes beyond the saved Test 8 update.

## Device validation

No physical Android GPU was connected. Sustained FPS, thermals and rendering appearance still need phone testing. Mali/Xclipse texture compatibility remains experimental and depends on the device's other Vulkan capabilities. The fallback preserves native compressed texture uploads where supported, including compatible Adreno drivers.
