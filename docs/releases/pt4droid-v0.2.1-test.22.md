# pt4droid 0.2.1-test.22

The complete performance, graphics and reliability update since Test18. [Version-by-version changelog](https://github.com/XHYN-PH/pt4droid/blob/pt4droid/CHANGELOG.md).

## Upscaling, resolution and frame pacing

- Added **SGSR2 temporal upscaling** across graphics presets, with motion, exposure, camera-cut and particle-reactivity handling.
- Added optional **Dynamic resolution** driven by GPU frame times and the selected FPS target, with **50%, 60% and 67%** minimum scene scales.
- Reused prepared resolution resources and shared output-sized buffers to reduce allocations and memory use during scale changes.
- Reduced SGSR2 bandwidth and memory use by producing one history image and using it directly for reconstruction.
- Reduced unnecessary upscaling buffers and motion-blur output work when those outputs are not needed.
- Added **100%, 90% and 80% Scene output resolution**, with menus and touch controls at display resolution when using reduced output modes. Full output resolution is the default.
- Added switchable **Android performance hints** on Android 12 and later, following the render thread, frame target and pause/resume state.
- Kept the new advanced options independent of graphics presets and unrelated frame-limit choices.

## Shadows and lighting

- Improved shadow reuse when draw order changes, while updating moving objects, animated characters and material changes correctly.
- Improved static-shadow caching and kept moving objects separate from reusable scenery.
- Added a bounded static-shadow template pool for high-quality shadows.
- Added tighter shadow bounds to avoid drawing objects outside a light's range.
- Updated cached alpha-tested shadows when their textures are restored.
- Improved cached probe lighting with a filtered lookup on supported formats and retained compatible fallback paths.
- Kept subsurface-lighting resources correctly sized and bound during dynamic-resolution changes.

## Lamp glare and visual effects

- Added a separate **Lens flares (lamp glare)** switch, independent of Lens flare ghosts. Room lighting and shadows stay active when glare is disabled.
- Added **Subtle, Balanced and Original Lamp halo strength** options to reduce broad bright halos while retaining compact lamp glows.
- Cleared disabled glare output and aligned flare depth with temporal-upscaling output.
- Added compatible full-screen pass optimizations for particle compositing, tonemapping, grading and screen effects.
- Reused particle refraction snapshots instead of copying the same scene again.
- Reduced repeated effect-texture lookups and particle sorting while preserving transparency order and effect density.
- Streamlined depth of field by skipping unused near-blur work and reducing sampling where blur is zero.

## Loading, memory and background work

- Combined mesh uploads and improved allocation and submission failure cleanup.
- Prepared effect pipeline variants earlier and improved retry handling after temporary failures.
- Restored converted textures in the background, bounded queued restoration work and adjusted cache limits to available graphics memory.
- Reused bounded preparation workers and added optional parallel geometry preparation.
- Reused unchanged collision data during area changes and moved collision-data retirement into background work.
- Applied saved graphics settings before allocating rendering resources.
- Released unused motion-blur buffers while retaining temporal-upscaling inputs.
- Moved photo encoding into the background, bounded pending captures and improved save-error reporting and cleanup.

## Drivers and settings

- Added optional **ESMERINGHOY 27.0.0-devel**, available in the driver picker and as a separate importable package.
- Added Turnip rendering profiles: **Driver default, Prefer system memory, Prefer tile memory, Tune small passes, and Tile memory + small passes**. Restart the game after changing a profile.
- Simplified the ESMERINGHOY driver label and description.
- Made SGSR2 and Dynamic resolution follow their dependency visibly, avoiding silent setting overrides.
- Corrected render-target sizing when switching SGSR2 live, including when SGSR1 is enabled.
- Improved persistence of the added graphics and performance options.

## Reliability and diagnostics

- Finished already-submitted background jobs before returning an allocation failure.
- Preserved a valid shadow atlas when replacement initialization fails and cleaned up partial rendering resources.
- Added frame-linked diagnostic events for uploads, pipeline creation, texture residency and collision work.
- Expanded reports with actual driver revision, rendering profile, active upscaler, internal/output resolution, resolution transitions and individual GPU pass timings.
- Added live lamp-glare, ghost, bloom, SGSR2, dynamic-resolution, halo-strength and exposure information.
- Updated the README, build documentation, download links and performance-setting guidance.

Install the APK over your existing pt4droid installation to keep game files, saves and settings. Game files are not included. Requires 64-bit Android 10+ and Vulkan 1.3.

For a first performance comparison, use a **60 FPS** limit and repeat the same route with matching graphics and driver settings. Performance and driver compatibility vary by device; reduced output resolution and experimental rendering options are selectable.
