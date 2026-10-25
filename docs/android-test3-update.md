# pt4droid 0.2.1-test.3

This update keeps package `com.ptport.app.test` and the existing test signing key, with version code 45. Install over P.T. Test to retain game files, saves, controls and drivers. The launcher is now named **pt4droid**, with the supplied P.T. icon. Game files are not included.

## Fixes from the audit

- **Layout editor freeze/crash:** test.2's touch handler entered an endless loop whenever a touch missed the toolbar. Selection and dragging now use bounded loops. Pointer cancellation, toolbar release, Save, Cancel, Reset, drawing and mode restoration are covered by production-code checks. The test reproduces the hang in test.2 before exercising the fixed code.
- **Custom preset stuck:** the game preset row previously clamped at Custom, so confirm/tap could not cycle to another preset. It now wraps. Original (PS4) is also selectable directly from Android settings.
- **Presets reverting after restart:** mobile presets are now applied once when requested; `pt.ini` preserves the result. Later game-menu changes supersede the mobile request. An old Maximum/Balanced profile label no longer overwrites graphics at every start.
- **Graphics-save resilience:** Android settings write a temporary INI and replace the previous file after closing successfully. Individual graphics changes have a recoverable pending file, so applying a new request cannot delete a newer request. Audio, input, progress and other settings are preserved.
- **Vulkan partial failures:** swapchain image enumeration, view creation and semaphore creation are checked; partial resources are cleaned up. Unsupported or failed timestamp-query allocation disables diagnostic timing instead of requiring it for gameplay.
- **Driver selection across GPUs:** Automatic chooses the existing bundled Turnip on Adreno 6xx/7xx and the system driver on other GPUs, including Adreno 8xx and Mali. Existing explicit selections and imported drivers remain available. This policy does not make an incompatible driver compatible.
- **Bluetooth controllers:** the manifest now declares the permission that SDL already requests. Denied or revoked permission is handled at Bluetooth calls, queued operations stop on failure, and shutdown cancels pending controller work. USB receiver registration includes the required export flag. Release lint now fails the build on errors instead of silently accepting them.

The audit and build checks reduce specific known risks; they are not a guarantee that every device, scene or vendor driver is bug-free.

## Menus and controls

The launcher and Android dialogs use a dark background, restrained serif text and red selection highlights inspired by the supplied menu reference. Native settings receive a dark overlay and red focus row. The reference screenshot is not shipped as a game asset. Launcher and settings content scroll on shorter landscape displays.

**Settings → Look sensitivity** adjusts from **0.25× to 3.00×**, with a numerical value. It uses the existing saved preference and applies to swipe and joystick modes. The editor also retains sensitivity, control positions, sizes, mappings, visibility, opacity, dead zone and floating-stick preferences.

Choose **Swipe to look • no button** for an invisible right-side look area, **Hidden joystick** for continuous turning while displaced, or **Visible joystick** for a drawn stick. Buttons have priority. Physical mouse input passes through the overlay. Back/Escape cancels editing.

## Individual game graphics

**Game graphics** is available directly on the launcher and inside Android settings during play. It reads the game's current settings and exposes:

- Frame limit, VSync and letterbox.
- Texture detail, anisotropic filtering and shadow resolution.
- Ambient occlusion and local reflections.
- Bloom, depth of field, motion blur, lens distortion and flare ghosts.
- Film grain and clarity amounts.
- Ray-traced shadows, ambient occlusion, contact shadows, reflections and ray quality when the active Vulkan device supports ray queries.

Changes apply through the game thread and persist for the next start. Selecting an individual configuration sets the mobile profile to Custom; configurations matching Original are identified accordingly. Unsupported ray tracing is disabled with its reason. HDR, enhanced texture generation, desktop upscalers and frame generation remain unavailable in this Android build. Render scale and the optimized mobile bloom filter are in the main Android settings.

Original (PS4) restores the game's graphics preset; it does not reset render scale, frame cap or touch settings. Maximum performance keeps the existing reduced graphics profile, turns VSync off, removes the gameplay frame cap and caps scene resolution at 50%. Balanced uses a 60 FPS cap. Menus and paused gameplay remain capped at 60 rendered FPS.

## Display refresh rate

**Settings → Display refresh rate** offers Automatic (highest supported at the current physical resolution) and the rates reported by the device. The application requests this rate for its window and SDL game surface. Requests are reapplied after resume or surface recreation, without polling or changing global display settings. Android 10 uses window mode selection; Android 11+ uses the Surface frame-rate API.

This addresses the absent high-refresh request that can leave games at Android 15's default 60 Hz. Choose 120 Hz when reported by the device, and use Unlimited or 120 FPS in Game graphics if desired. A 120 Hz panel does not imply 120 rendered FPS. Android can override the request for battery, temperature or vendor policy; check the current display rate shown in settings.

Android references: [game refresh rates](https://developer.android.com/games/optimize/display-refresh-rate-change) and [frame-rate API behavior](https://developer.android.com/media/optimize/performance/frame-rate).

## Further performance changes

- Mesh recording caches unchanged vertex/index bindings, cull mode and front-face state in geometry, shadow and motion passes. The cache resets at pass boundaries and other state owners.
- Native touch input skips redundant virtual-gamepad state updates.
- Logs Off avoids formatting, file writes, log-tick locking and GPU timestamp collection. Diagnostic query pools are allocated only when needed, on queues with timestamp support. With logs On, Android info logs flush periodically; warnings/errors flush immediately.
- The previous update's mobile bloom approximation, discarded overwritten targets, on-demand screenshot copy, reusable upload command buffer/fence, pipeline-cache checkpointing and stable-SUBOPTIMAL handling remain.

The [Bloodborne Android 0.5.2 reference](https://github.com/ducvd89/Bloodborne-Android/releases/tag/android-0.5.2) was examined at commit `d58769c8ae2f36b4f22db864b68e9bffc21a04d2`. Its game-profile fix and renderer/swapchain changes concern a different engine and workload. pt4droid already created a replacement swapchain before destroying the old one. Its driver-policy idea informed the original device selection here; no Bloodborne source or frame-generation assets were copied. Its FEX and frame-generation path cannot simply be transplanted into this native port.

No phone FPS improvement has been measured for test.3. Compare the same room and camera position after warmup, at the same resolution and thermal state. Supported devices still need ARM64 Android 10+ and the Vulkan capabilities required by this engine, including Vulkan 1.3 through the selected driver.

## Logging, imports and saves

**Enable logs** is Off by default. Apply the setting to enable or disable app/native/SDL/import diagnostics and app crash-report files. Android 11+ also filters process liblog output; Android 10 has no equivalent process filter for arbitrary driver libraries. Android's own crash reporting, OS and vendor diagnostics are outside the app switch. Existing reports are retained when logging is disabled.

For a new report, enable logs before reproducing the problem. Game reports use `Download/PT-logs-test` with an app-data fallback. Import diagnostics use `Android/data/com.ptport.app.test/files/data/driver-import.log`.

The tested driver importer retains progress, cancellation, an absolute 60-second deadline, safe archive extraction, ARM64 validation, canonical paths and reimport repair. Fully close and reopen after selecting a driver. Imported Turnip is for compatible Adreno GPUs; runtime compatibility needs testing on the phone.

Delete saves remains on the launcher and in Android settings, with confirmation while the game is closed. It deletes the two save slots only, retaining game files, settings, layouts, drivers, caches and Extras unlocks.
