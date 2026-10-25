# Changelog

Full change history from Test19 onward.

## 0.2.1-test.24

- Improved shadow-cache reuse when shadow priorities change.
- Reduced unnecessary light shading while preserving the same area and range coverage.
- Reduced off-screen rain preparation and improved particle-data batching.
- Refined texture preparation during area loading and skipped redundant reads for missing texture headers while preserving material fallbacks.
- Added an estimated combined scene scale to explain how the two resolution controls work together.
- Added Normal and Verbose logging for easier troubleshooting.
- Made performance reports distinguish loading pauses from gameplay and show the final spatial upscaler separately.
- Added clearer frame-pacing guidance and simplified the README and release notes.

## 0.2.1-test.23

- Fixed a crash on the first Start after applying graphics settings from the launcher.
- Kept requested ray-tracing settings while the selected driver starts.
- Kept requested graphics changes available for retry if startup or saving fails.
- Preserved native crash reports across successful restarts and included them in diagnostic exports.
- Improved Java crash-report fallback when shared storage is unavailable, and included older reports from app storage.
- Made diagnostic export tolerate logs rotating or being truncated during collection.
- Included interrupted graphics requests, pending presets and driver startup failures in diagnostic exports.
- Added app version and time information to crash reports to make problems easier to investigate.

## 0.2.1-test.22

### Rendering performance

- Reduced SGSR2 memory use and image-processing work.
- Improved shadow-cache reuse when draw order changes, while keeping updates for moving objects, animated characters and material changes.
- Improved static-shadow caching by separating moving objects from reusable scenery.
- Reused particle refraction snapshots instead of copying the same scene again for compositing.
- Reduced repeated work when looking up cached room lighting on supported devices.
- Skipped the unused near-blur downsample in far-only depth of field and reduced sampling where the blur radius is zero.
- Released unused motion-blur buffers after the effect is disabled, while retaining inputs needed by temporal upscaling.
- Applied saved graphics settings before allocating rendering resources, avoiding unnecessary initial shadow-atlas allocations.

### Visual options

- Added **100%, 90% and 80% Scene output resolution**. Reduced output resolutions lower scene and post-processing detail while keeping menus and touch controls at display resolution. Full output resolution is the default.
- Added **Subtle, Balanced and Original Lamp halo strength** options. Broad halos can be reduced while compact lamp glows are retained.
- Kept output resolution and halo strength independent of the graphics preset and selected frame limit.

### Reliability and documentation

- Improved cleanup if background preparation runs out of memory.
- Kept working shadows available if a shadow-quality change cannot be completed.
- Renamed the bundled and downloadable driver to **ESMERINGHOY 27.0.0-devel** and simplified its selection description.
- Updated the README, download links, recommended starting settings and build documentation.

## 0.2.1-test.21

### Lighting controls

- Added a separate **Lens flares (lamp glare)** switch in Performance & advanced, independent of Lens flare ghosts.
- Kept room lighting, lamp geometry, shadows, world particles and screen effects active when lamp glare is disabled.
- Cleared the flare buffer when glare is disabled so an earlier bright frame cannot remain visible.
- Aligned post-processing flare depth with the resolved output when temporal upscaling is active.

### Settings and diagnostics

- Made the SGSR2 and Dynamic resolution dependency visible: enabling Dynamic resolution enables SGSR2; disabling SGSR2 disables Dynamic resolution.
- Prevented contradictory settings from silently turning SGSR2 back on during Apply.
- Corrected live render-target sizing when switching SGSR2 while SGSR1 remains enabled.
- Added live glare, ghost, bloom, SGSR2, dynamic-resolution and exposure information to diagnostic reports.

## 0.2.1-test.20

### Rendering and memory

- Reduced upscale-resolve work when motion blur is disabled while retaining motion information needed for temporal reconstruction.
- Reduced SGSR2 input-buffer allocations.
- Shared output-sized post-processing and history resources between dynamic-resolution scales, reducing duplicate allocations.
- Kept subsurface-lighting images and their bindings together when switching resolution, with cleanup for partial allocation failures.
- Expanded compatible grading and screen-effect pass optimizations while preserving separate paths for effects that need them.
- Added tighter, conservative shadow bounds to avoid drawing objects outside a light's range.

### Loading, collision and effects

- Reused unchanged collision data during area renames and removals while preserving collision-query ordering.
- Reused effect texture and cube-map lookups instead of rebuilding duplicate lookup tables every frame.
- Reused valid particle draw ordering while preserving transparency order, particle density and effect evaluation.

### Resolution and drivers

- Made the dynamic-resolution frame budget follow the selected FPS target, or display refresh rate when uncapped.
- Added **50%, 60% and 67% dynamic-resolution floors**, bounded by the selected normal scene scale.
- Added the optional bundled ESMERINGHOY driver and a separately importable driver package.
- Added **Driver default, Prefer system memory, Prefer tile memory, Tune small passes, and Tile memory + small passes** Turnip profiles, applied after a full game restart.
- Expanded diagnostics with the active upscaler, actual internal resolution, driver revision, rendering profile, resolution transitions and individual post-processing timings.

## 0.2.1-test.19

### Upscaling and performance options

- Added optional **SGSR2 temporal upscaling**, including motion, exposure, camera-cut and particle-reactivity handling.
- Added optional **Dynamic resolution**, using GPU frame times to switch between prepared resolution scales and reset temporal history during transitions.
- Added memory-budget checks before preparing additional dynamic-resolution resources.
- Added experimental **Full-screen pass optimizations** to reduce compatible scene-copy and post-processing work.
- Added optional **Parallel geometry preparation** using a bounded worker pool and a serial fallback.
- Added **Android performance hints** on Android 12 and later, following the render thread, frame target, pause/resume and disable state.
- Made the new advanced options available independently of graphics presets.

### Loading, textures and shadows

- Combined vertex and index mesh uploads into one submission and improved cleanup after allocation or upload failures.
- Prepared effect pipeline variants earlier, cached successful variants and delayed retries after temporary failures.
- Moved converted-texture cache reads into the background, bounded queued restoration work and adjusted cache limits to available graphics memory.
- Synchronized texture and image updates with completed rendering work.
- Updated cached alpha-tested shadows when their texture residency changes.
- Added a bounded static-shadow template pool for high-quality shadows without duplicating the full main shadow atlas.
- Reused preparation workers and moved collision-data retirement onto a persistent background worker.

### Photos and diagnostics

- Moved photo encoding into the background, limited pending captures and improved save-failure reporting and shutdown cleanup.
- Added frame-linked diagnostic events for mesh uploads, pipeline creation, texture residency and collision work to help locate frame-time spikes.

Performance depends on the device, driver, scene and temperature. Experimental options remain selectable in Performance & advanced.
