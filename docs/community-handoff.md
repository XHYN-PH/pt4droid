# Community handoff

XHYN-PH ended active development of pt4droid on November 19, 2026. Anyone may fork, build, modify and continue the port under its existing MIT license. Keep the original authors' credits and license notices; third-party libraries and drivers retain their own licenses. This handoff does not grant rights to P.T. game content, which is not included.

## Downloads

Everything prepared for this handoff is attached to the [community handoff release](https://github.com/XHYN-PH/pt4droid/releases/tag/community-handoff-2026-11-19):

- **pt4droid-v0.2.1-test.26-arm64.apk**: the latest completed test build, version code **69**, application ID **com.ptport.app.test**. It was built from commit **4c32c00dbd9a1a0a931c3fcc65bdeca195cd99e7**. Later handoff commits change documentation and build helpers, not the app's runtime source.
- **pt4droid-PUBLIC-signing-kit.zip**: the original keystore, signing credentials and explicit public-handoff instructions. The previous private-use note is preserved as historical information and is superseded by the owner's public release on November 19, 2026.
- **pt4droid-complete-history.bundle**: all saved local source branches, commits and tags, including unpublished work.
- **pt4droid-community-archive.tar.gz**: the available historical APKs, drivers, source patches, development notes, test reports, project-related diagnostics and gameplay evidence, cloud build/helper scripts, packaged Android libraries/assets and available native debug libraries. The archive has a file manifest and documents exclusions. Historical scripts are records of past workflows; use the current documented build commands for new releases.
- **pt4droid-dependency-sources.tar.gz**: the downloaded third-party source trees used by the native build, with their licenses and source revisions where available.
- **SHA256SUMS.txt**: checksums for the release files.

The public ESMERINGHOY driver remains **27.0.0-devel**. The archive also preserves earlier experimental packages for historical reference; they are not recommendations for every device.

## Signing identity

The key and passwords were intentionally made public so community developers can produce updates compatible with existing installations. The signing certificate SHA-256 fingerprint is:

```text
d17b8e79ff97002a76b23f99c105fb002aeea01ba12734130293ab48b3e124af
```

**A matching signature no longer identifies a trusted maintainer.** Users should obtain updates from a continuation project they trust. Developers can use a new signing key and application ID for an independent fork; that normally installs separately. For an update using the existing package, keep the same key and increase the version code beyond **69**.

Extract the public signing ZIP outside the repository, then follow [Android build instructions](android.md). `tools/android/build_with_signing_kit.py` can verify the key or run the signed native/Gradle build without printing the passwords. Standard builds need JDK 17, Android platform/build-tools 35, NDK r27 or newer, CMake 3.28+, Ninja and glslc. The last cloud build used NDK **27.2.12479018** and JDK **17.0.16+8**.

## Restore the complete history

```bash
git clone pt4droid-complete-history.bundle pt4droid-history
cd pt4droid-history
git branch -a
```

The bundle contains Git history, not account credentials or the old workspace's Git configuration. The repository's default branch includes the final application source and the handoff guide. The archive supplies work that was stored outside tracked source.

## Last build and remaining work

The Test26 APK passed 48 host regression entrypoints, Android compilation/lint, original-certificate verification, ARM64/16 KiB alignment checks and packaged-file checks during the October 25, 2026 build. These results are historical checks, not a guarantee of correct rendering or performance on every phone. See the archived Test26 state and completion records for details.

The last user reports implicated ordinary shadow maps in a dark copy of the hanging lamp. Turning ordinary shadows off removed it; traced shadows were already off. The precise projection/caster cause has **not been confirmed fixed**. A separate repeated-enable bug in normal/red/off lamp state was reproduced and fixed while preserving the independently controlled glass effect.

Next steps for the lamp issue:

1. Keep ordinary shadows enabled and compare **Settings → Troubleshooting → Force fresh shadow maps** Off and On in the same area.
2. Enable **Verbose shadow and loading details**, reproduce the issue, and inspect the bounded per-light/lamp-caster records.
3. If fresh maps show the same outline, investigate the light transform, lamp mesh groups, skinning/pose and projection rather than assuming stale cached tiles.
4. Keep legitimate shadows; removing all lamp casters globally has not been established as the correct fix.

Test26 also includes PC-style shadow/AO/reflection controls, optional capability caching, clearer Android settings, CPU deadline pacing, conservative particle culling, common shadow-caster filtering and missing-texture preparation refinements. These paths apply across graphics presets. No new Turnip binary changes or thermal FPS adaptation were added in Test26. Compare sustained frame times and visual output on actual devices before claiming performance gains.

## What is outside this handoff

GitHub/account tokens, cloud-session authentication, the cloud proxy configuration and unrelated personal uploads are excluded. Standard SDK/compiler installations, Gradle caches and disposable object files can be downloaded or rebuilt using the documented tools. No game dump is distributed. The handoff archive preserves the project materials available in this workspace; it cannot recreate files that were never uploaded or saved here.
