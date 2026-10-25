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

The script defaults to version `0.2.1-test.24`, version code `67`, package `com.ptport.app.test`, and app label `pt4droid`. Override `PT_VERSION` and `PT_VERSION_CODE` for a future release, increasing the code each time. Reuse the signing key to allow upgrades that retain user data.

Output: `android/app/build/outputs/apk/release/app-release.apk`. The script also runs release lint, which stops the build on errors.

## Development

Host checks are in `tools/android/tests`. They use JDK 17 and the headers and tools prepared for the Android build. Some checks also use historical Git tags or locally supplied game-file fixtures.

The inherited [GitHub Actions workflow](../.github/workflows/android.yml) targets upstream `android-port` and `android-*` release conventions. A push to this fork's `pt4droid` branch does not publish a Test release automatically.
