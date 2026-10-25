# P.T. Android 0.2.1-test.2

This update uses package `com.ptport.app.test`, version code 44, and the persistent test signing key. It can update test.1 without clearing app data. The original application's package is separate. Game files are not included.

## Performance changes

Android settings provides Custom, Maximum performance and Balanced presets. Maximum performance removes the rendered-frame cap, disables v-sync, uses 512-pixel shadow tiles, reduces texture detail/filtering and disables ambient occlusion, reflections, bloom, depth of field, motion blur, lens ghosts/distortion and grain. The selected render scale is capped at 50%. Balanced limits gameplay to 60 FPS, uses 1024-pixel shadows and retains bloom/reflections. Custom keeps the current game settings. Menus remain capped at 60 FPS.

For bloom, grouped Android kernels preserve cumulative blur variance and energy weights while approximating the original appearance. The usual extraction/blur chain falls from 41 to 13 render passes. Disable Optimized mobile bloom to restore the original filter. Fully overwritten post-process targets discard old contents; additive destinations retain their accumulated contents.

Android screenshots render on demand instead of copying the screen every frame. Swapchain images request color-attachment usage only. An unchanged render scale does not rebuild targets. Uploads reuse a command buffer and fence; the existing pipeline cache is checkpointed on focus loss. Stable Android SUBOPTIMAL results retain the swapchain, with recreation for window, extent, transform, capability changes or surface loss. The log includes GPU pass timings and renderer CPU time.

The supplied test.1 log showed a 30 FPS cap, most gameplay GPU samples around 19–23 ms, and switches to an 8192×8192 shadow atlas. This is a different workload from other ports. Removing the cap alone does not guarantee 60 FPS; the renderer must also finish frames faster. No FPS result for this update has been measured on a phone.

## Controls

Choose Swipe to look • no button for an invisible gesture area on the right half. Movement stays on the left. Buttons take priority. Swipe turning follows distance, stops with a stationary finger, and includes final lift movement. Hidden joystick keeps turning while held away from the initial touch point. Visible joystick restores the old stick.

Button positions, size, bindings, visibility, opacity, sensitivity, dead zone and floating-stick preferences retain their existing keys. Customize touch layout is available in-game. Auto, Off and Always show modes persist. Focus loss, cancellation and lifecycle transitions release controls; physical mouse events pass through. Back/Escape cancels layout editing. Android settings is available from the launcher, touch overlay and the game's PC settings.

## Driver import

Import an ARM64 AdrenoTools ZIP containing one meta.json and its libraryName shared library. The application-owned job survives activity recreation. Import displays read/extract/check/install progress, supports cancellation and has an absolute 60-second deadline. Errors are shown, including provider, archive, metadata and linkage errors. Cancellation/failure retains the previously selected driver.

Canonical private-data paths fix relative-library calculations through Android's /data/data and /data/user/0 aliases. Reimporting the same ZIP repairs its metadata and reuses its package ID. Compressed/expanded sizes, file counts and paths are bounded; ELF architecture and minimum Android API are checked before activating the driver. Runtime driver compatibility needs testing on the phone. Fully close and reopen the game after import or driver selection.

Import diagnostics: Android/data/com.ptport.app.test/files/data/driver-import.log. Game logs: Download/PT-logs-test/pt.log, with an application-data fallback.

## Delete saves

The launcher has Delete saves, with confirmation while the game is closed. It deletes only PT_Save_Data0 and PT_Save_Data1 in the internal/external data directories. Game files, settings, touch layout, drivers, caches and Extras unlocks stay.
