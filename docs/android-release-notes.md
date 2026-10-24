# pt4droid 0.2.1-test.22

## Performance

- Added SGSR2 temporal upscaling and dynamic resolution across all graphics presets.
- Reduced SGSR2 memory use and output bandwidth.
- Improved shadow performance while preserving moving shadows.
- Reduced rendering work for particle effects and lighting.
- Streamlined depth-of-field processing and released unused motion-blur buffers.
- Applied saved graphics settings before allocating rendering resources.
- Added 100%, 90% and 80% scene output resolution, with menus and touch controls at display resolution.
- Improved frame pacing, Android performance hints, collision updates and background loading.

## Visuals and controls

- Added Subtle, Balanced and Original lamp-halo strengths, plus a separate lamp-glare switch.
- Corrected flare depth alignment with temporal upscaling.
- Made the SGSR2 and dynamic-resolution settings follow their dependency visibly.
- Improved live resolution switching and graphics-setting persistence.

## Drivers

- Added **ESMERINGHOY 27.0.0-devel** and selectable Turnip rendering profiles.
- Simplified driver selection labels and descriptions.

## Reliability

- Improved cleanup after allocation, upload and shadow-initialization failures.
- Improved background work, texture loading, photo saving and diagnostic reporting.

Install the APK over your existing pt4droid installation to keep game files, saves and settings. Game files are not included. Requires 64-bit Android 10+ and Vulkan 1.3. Performance and driver compatibility vary by device.

For a first performance comparison, use a 60 FPS limit and repeat the same route with matching graphics and driver settings. Full scene output resolution remains the default; reduced output resolution is optional.
