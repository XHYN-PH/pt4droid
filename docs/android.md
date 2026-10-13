# Android build instructions

For installation, controls and device requirements, see the [README](../README.md).

## Toolchain

- JDK 17.
- Android SDK/platform and build-tools 35.
- Android NDK r27 or newer.
- CMake 3.28+, Ninja and `glslc` on `PATH`.

Set `ANDROID_HOME` to your SDK and `ANDROID_NDK_HOME` to your NDK. The native build stages ARM64 libraries, shaders and assets before Gradle packages the APK.

## Signed test release

Set `PT_KEYSTORE` to your persistent signing keystore and `PT_STORE_PASSWORD` to its password. Set `PT_KEY_ALIAS` and `PT_KEY_PASSWORD` if different from the defaults. Keep credentials outside the repository.

From the repository root:

```bash
bash tools/android/build_test_release.sh
```

The script defaults to version `0.2.1-test.10`, version code `52`, package `com.ptport.app.test`, and app label `pt4droid`. Override `PT_VERSION` and `PT_VERSION_CODE` for a future release, increasing the code each time. Reuse the signing key to allow upgrades that retain user data.

Output: `android/app/build/outputs/apk/release/app-release.apk`. The script also runs release lint, which stops the build on errors.

## Validation and releases

Host regression scripts are in `tools/android/tests`. Some need JDK 17, GLM/Vulkan headers, historical repository tags, and the importer fixtures specified in the [build validation record](android-test10-build-validation.md).

Before publishing, check the APK signature, package/version, staged assets and libraries, and 16 KB alignment. Host checks do not replace testing rendering, startup and sustained performance on Android hardware.

The inherited [GitHub Actions workflow](../.github/workflows/android.yml) targets upstream `android-port` and `android-*` release conventions. A push to this fork's `pt4droid` branch does not publish a Test release automatically.
