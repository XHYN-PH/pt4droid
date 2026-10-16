# pt4droid 0.2.1-test.15 candidate

Test 15 is a device-test candidate, not a stable release. It follows Test 14 on Android 10+ ARM64, with package com.ptport.app.test and version code 57. Save/update continuity and broad lifecycle/recovery work are excluded as requested. Necessary cleanup for the new video, camera input and phone vibration is included.

## Launcher and controls

The supplied teaser is transcoded to H.264/AAC for wider Android decoding support. Cold launcher starts play it before the menu. Skip, Back or controller B/Start reaches the menu; playback failure also opens it. Replay teaser returns to the existing launcher. Version sits bottom left, opposite clickable GitHub (XHYN). The footage remains third-party game artwork, outside the port code's MIT license.

Controls and Performance & advanced build separate pages. Controls merges its input keys without writing graphics, render scale or pending presets. Both pages preserve unowned configuration fields. Immediate file or preference failures roll back the affected writes; this is not power-loss recovery or save migration. Explicitly changing a preset retains its documented graphics/cap behavior. Unrelated changes preserve every selected cap, including Unlimited and rates unsupported by the display.

Customize touch layout appears only in Controls. Auto-mode button timeout is adjustable from Never to 60 seconds. A touch that reveals hidden controls cannot accidentally press an invisible action or Settings button. Invisible movement/look remain usable. Optional phone vibration follows the existing SDL virtual-controller rumble callback, with bounded pulses refreshed by the original feedback stream. Master game rumble still applies. Controls explains microphone permission and the existing Name-button alternative. Puzzle timing is unchanged.

A compact scrolling camera toolbar sits beside Settings, with Freecam, Photo, Third/First person, Shutter, Up/Down, Panel and Hide/Camera. Movement/swipe look fly the detached camera, including while the photo panel has pointer focus. Native controls retain the existing menu/ending/VR gates. Actions run on the game thread and actual mode state returns to the toolbar. Duplicate Android camera settings entries are removed. Graphics driver management appears only in the launcher.

## FSR and drivers

The launcher reads the same upscaling fields as the game menu. The current FSR 3 backend loads amd_fidelityfx_vk.dll through Windows APIs and is not built for Android. Test 15 shows FSR as unavailable. Porting that backend is separate work; no FSR 1 backend is added. Existing Android SGSR1 and fixed render scale remain available.

Android/Bionic Vulkan imports use the existing generic custom-driver path. PanVK/Mali and RADV/Xclipse packages are labeled experimental; OpenGL-only packages are rejected. Metadata/API/ELF checks do not prove phone compatibility. No bundled driver is upgraded. The Mesa unified project's current compatibility list covers Xclipse 920/530 and tested Bifrost Mali devices. Bundled PanVK Kbase has separate GPU/Android requirements. Do not infer support for every Mali or Xclipse model.

## Loading, rendering and diagnostics

- Texture jobs rejected by temporary preparation memory pressure wait for release and retry within a bound. A demanded job can return Budget for the safe direct-read path, avoiding deadlock behind memory held by another ready result. Stop wakes/cancels deferred work. The 96 MiB bound is unchanged.
- Removed collision owners can be deactivated while retaining the remaining world/grid. Matching uses live piece identity, stage generation and transform metadata without dereferencing freed stages. New/changed geometry still rebuilds. Street-walk detail preparation remains a measured follow-up.
- Ending prefetch starts at the existing pre-ending handoff. Early scheduling counts new model definitions instead of repeated/already loaded placements. Authored events and audio order are preserved.
- Reusable VFX build storage is reserved during effect loading. Particle payload/order and simulation remain unchanged.
- Main/mirror draw visibility is memoized per freshly prepared Android frame. Other views retain their original query. Shared direct-light shaders skip shadow work when its authored contribution is exactly zero.
- Mali safe lighting is vendor-gated. Supported probe atlases use RGBA16F, explicit four-tap lookup, finite-value checks and a direct lookup for invalid cached values. Disabling safe lighting restores the hardware-filtered Mali choice; disabling Mali optimization selects original lighting. Adreno does not enter the Mali shaders/format selection. This is not a demonstrated cure for all black squares/darkness; phone comparisons remain required.
- QAR validates footer/table/entry bounds and clears old entries on reopen. FTEX validates dimensions/mip tables and rejects textures with no usable mip. Diagnostics distinguish a missing archive header from invalid headers/streams. Missing game assets are not fabricated or silently repaired.
- Startup registration logs split demos, puzzles and messages. Expected optional second voice-model fallback is informational.
- Native logs rotate at 8 MiB with one previous file; app logs rotate near 1 MiB. Diagnostic export creates a bounded ZIP of logs, device/refresh identity and selected configuration files. It excludes saves and microphone audio, and opens Android's share chooser. Nothing is sent automatically.
- Refresh reporting separates physical mode refresh from Android's app-visible rate. It preserves the selected FPS cap. Local changes receive a source fingerprint instead of an ambiguous dirty suffix.

## Validation and limitations

The accompanying verification report records build/source identity, APK hash and checks. Host checks cover queue release/demand/cancellation, collision retirement with actual queries, archive bounds, settings page isolation/rollback, touch behavior, graphics/refresh targets, shaders, compilation and packaging. They complement device testing.

Before stable, compare matched Test 14/Test 15 routes on Adreno 750, Adreno 650 and Mali: dark hallways, mirrors, flashlight shadows, fog/particles, ending and street walk. Verify microphone/manual progression and phone rumble timing in a fresh playthrough, camera controls/teaser on a phone, sustained memory/thermal behavior, each cap/pacing choice, and supported imported drivers. No phone benchmarks or performance-percentage claims are made here.

The 27 ending texture failures require the actual game dump to classify missing versus unreadable assets. Parser safeguards do not establish their repair. GPU-pass profiling, the startup registration delay and street-walk collision preparation remain follow-ups. Save/update continuity and broad lifecycle/recovery testing remain outside the requested scope.

## Build

Use the normal Android build instructions and persistent test signing environment. tools/android/build_test_release.sh defaults to Test 15/version code 57. If Gradle dependency downloads are unavailable, configure/build/stage native code first, then run tools/android/package_sdk.py with ANDROID_HOME, JAVA_HOME and the same signing environment. The fallback includes Swappy's Java runtime and native integration. Never commit signing material.

Primary driver references:

- https://github.com/JimVulkan/mesa-unified-drivers
- https://github.com/zenithblue-oss/panvk-kbase-android
