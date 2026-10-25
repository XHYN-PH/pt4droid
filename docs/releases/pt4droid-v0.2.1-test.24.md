# pt4droid 0.2.1-test.24

## What’s new

- Improved shadow-cache reuse to avoid unnecessary rebuilding when cached shadows change priority.
- Reduced unnecessary lighting work where a light’s area and range overlap, preserving its lighting coverage.
- Reduced particle preparation work for off-screen rain and improved particle-data batching.
- Refined texture preparation during area loading and avoided redundant reads for missing texture headers while keeping material fallbacks.
- Added a combined scene-resolution estimate so it is easier to understand how Scene resolution and Scene output resolution work together.
- Added Normal and Verbose logging, clearer upscaling information, and separate reporting for loading pauses and gameplay frame times.
- Updated performance guidance and simplified the README and release notes.

## Installing or updating

Download **pt4droid-v0.2.1-test.24-arm64.apk** below. Install it over your existing pt4droid app to keep your game files, saves and settings.

You need a compatible **64-bit Android 10+ phone with Vulkan 1.3** and your own P.T. game files. Game files are not included.

## Suggested starting settings

Start with **Balanced**, **60 FPS**, and **100% Scene output resolution**. Adjust Scene resolution to suit your phone. Android frame pacing is optional: compare On and Off at the same FPS limit if motion feels uneven.

The bundled driver remains **ESMERINGHOY 27.0.0-devel**. Performance depends on the phone, scene, driver and temperature.

[Getting started](https://github.com/XHYN-PH/pt4droid#getting-started) · [Full changelog](https://github.com/XHYN-PH/pt4droid/blob/pt4droid/CHANGELOG.md) · [Report a problem](https://github.com/XHYN-PH/pt4droid/issues)
