# pt4droid 0.2.1-test.9

Test 9 refines Test 8 loading, shadows and rain preparation and fixes the reported data-folder startup error.

- **Loading:** prepare only textures selected by materials, enforce the 96 MiB preparation budget before stream reads, and reuse immutable compressed data for raw and sRGB uploads.
- **Shadows:** avoid preserving the atlas solely for a cheap cached tile when most shadow work changes; share equivalent main/mirror light maps only after exact projection and accepted-caster checks. Point lights share both hemispheres together. Moving geometry, alpha dependencies and animated skin retain their invalidation behavior.
- **Rain:** sort compact entries and skip payload preparation for conservatively rejected offscreen rain while preserving particle output and equal-depth ordering.
- **Startup fix:** Android writes migration completion using an atomic empty directory, avoiding unsupported hard links or rename operations. Existing markers, settings and saves are retained. Install over the previous matching test app; no data reset is required.

Package: `com.ptport.app.test`; ARM64; Android 10+; version code **51**. The existing test signing key is reused. Game files are not included.

All **22** host regression scripts and native ARM64 Release compilation passed. See `docs/android-test9-update.md` for implementation details and `docs/android-test9-build-validation.md` for final APK verification.

Graphics settings, fixed render scale, native supported BC uploads and driver defaults are retained. The incomplete upstream Mali Vulkan 1.1 pull request is not included. Mali/Xclipse compatibility remains experimental; physical-phone rendering and sustained FPS still need testing. Compare the same scene and settings after warmup before judging performance.
