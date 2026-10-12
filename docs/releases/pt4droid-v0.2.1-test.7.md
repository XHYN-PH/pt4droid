# pt4droid 0.2.1-test.7

This update targets repeated rendering work while retaining your existing graphics settings and fixed render resolution.

- **Improved GPU scheduling:** Replaced broad render-target barriers with dependencies for the stages that actually use each image. Repeated read-only uses avoid redundant barriers; write dependencies and the conservative fallback for general/unknown layouts remain.
- **Shadow-map reuse:** Unchanged shadow tiles can keep their previous contents. Moving lights or objects, changes in the selected geometry, atlas resizing and mesh destruction invalidate reuse. Skinned characters and alpha-tested textures retain per-frame updates. Partial updates clear only their dirty tiles; fully dirty frames keep the original whole-area clear.
- **More precise shadow culling:** Static submeshes use bounds computed from their indexed triangles. Animated or invalid bounds retain the existing fallback. This culls in the light's view, preserving shadows from objects outside the camera view.
- **Smarter mirror rendering:** A mirror capture can be skipped when its entire padded bounds lie behind an active scene occluder. Partly visible mirrors still update. Checks use the current scene's occluders without GPU queries or readback waits.
- **Less particle CPU work:** Particle lighting selects the best three lights and probes with fixed storage, preserving the original tie order and probe weights. Temporary light and preload lists retain their capacity between frames.
- **Reduced buffer traffic:** Fully overwritten AO, reflection, composition and upscale outputs can discard their old contents. Partial mirror outputs and passes that blend or resume rendering preserve their needed contents. Empty offscreen particle passes skip depth downsampling and effect-buffer clears/stores.
- **More useful diagnostics:** GPU totals and individual passes now use the same completed samples and averaging window. Ray-tracing structure preparation is reported separately from shadow-map rendering. Logs include average shadow reuse, draw counts, mirror occlusion and active graphics settings. Corrected doubled shadow draw counters.

## Updating

Install the ARM64 APK over test.6 to keep your game files, saves, settings and touch layouts. The package is still `com.ptport.app.test`, version code 49, with the same signing certificate.

Game files are not included. Mali/PanVK support remains experimental with its existing Android/driver requirements.

## Validation

All 15 host regression suites and native ARM64 compilation passed. New checks compare 12,000 complete particle-light blocks with the independent test.6 implementation, verify zero temporary allocations in that function, exercise 8,000 transformed bounds and 3,000 cache mutations, and execute the production shadow recorder against an instrumented command backend. Existing checks cover imports, saves, settings, controls, layout editing, storage, logging, frame limits and menu exit.

Java/resources/DEX compilation, APK signing, native-library and asset equality, and 16 KB ELF/APK alignment passed. Gradle lint could not run because this environment cannot download the Gradle distribution; this APK was packaged with the Android SDK tools.

A phone was not connected. FPS gains, thermal behavior, real-driver rendering correctness and sustained gameplay must still be checked on device. This test release does not promise a particular FPS increase.
