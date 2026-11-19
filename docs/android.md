# Android build instructions

For installation, controls and device requirements, see the [README](../README.md).

## Toolchain

- JDK 17.
- Android SDK/platform and build-tools 35.
- Android NDK r27 or newer.
- CMake 3.28+, Ninja and `glslc` on `PATH`.

Set `ANDROID_HOME` to your SDK and `ANDROID_NDK_HOME` to your NDK. The native build stages ARM64 libraries, shaders and assets before Gradle packages the APK.

## Community handoff

Active development by XHYN-PH has ended. The [handoff guide](community-handoff.md) links the complete source history, public signing kit, archived development materials and the last packaged native libraries/assets. The SDK, NDK and compiler caches are standard downloads and are not required from the old cloud workspace.

## Signed test release

Set `PT_KEYSTORE` to your persistent signing keystore and `PT_STORE_PASSWORD` to its password. Set `PT_KEY_ALIAS` and `PT_KEY_PASSWORD` if different from the defaults. The original signing kit is intentionally public in the handoff release. For your own new signing identity, keep credentials outside the repository. A public signature no longer authenticates the maintainer.

From the repository root:

```bash
bash tools/android/build_test_release.sh
```

The script defaults to version `0.2.1-test.26`, version code `69`, package `com.ptport.app.test`, and app label `pt4droid`. Override `PT_VERSION` and `PT_VERSION_CODE` for a future release, increasing the code each time. Reuse the signing key to allow upgrades that retain user data.

Output: `android/app/build/outputs/apk/release/app-release.apk`. The script also runs release lint, which stops the build on errors.

To use the released signing kit without printing its passwords:

```bash
python3 tools/android/build_with_signing_kit.py /absolute/path/to/extracted-signing-kit --verify-only
python3 tools/android/build_with_signing_kit.py /absolute/path/to/extracted-signing-kit --version 0.2.1-community.1 --version-code 70
```

The helper loads only the supplied signing kit and passes its values to the existing build in environment variables. Future updates to the same application must use a version code greater than 69. A fork may instead choose its own signing key and application ID.

## Development

Host checks are in `tools/android/tests`. They use JDK 17 and the headers and tools prepared for the Android build. Some checks also use historical Git tags or locally supplied game-file fixtures.

The inherited [GitHub Actions workflow](../.github/workflows/android.yml) targets upstream `android-port` and `android-*` release conventions. A push to this fork's `pt4droid` branch does not publish a Test release automatically.
