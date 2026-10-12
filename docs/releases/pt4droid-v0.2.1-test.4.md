# pt4droid 0.2.1-test.4

This test update adds more performance and control choices, a refreshed launcher, and experimental Mali drivers. Install over test.3 to keep your game files, saves and settings. Game data is not included.

## Performance and graphics

- **Cheaper ambient occlusion and screen reflections:** new Performance modes reduce sampling work while keeping the effects visible. Original quality is still available. Shadow resolution remains adjustable in Game graphics, including the lighter 512 px option.
- **Optional Android frame pacing:** Swappy can handle presentation when you use a frame limit. Uncapped gameplay keeps the existing path, with a fallback if pacing is unavailable. Display refresh selection remains separate from rendered FPS.
- **Optional SGSR1 upscaling:** sharper output at a fixed reduced render scale, plus a sharpness slider. This is an image-quality choice that adds GPU work, so it is Off by default and uses a separate shader from the normal rendering path.
- **Optional heat management:** asks Android for sustained performance where supported and limits the effective frame rate as the device gets hot. Your saved graphics settings and frame limit remain intact.
- **Fixed resolution:** dynamic resolution is not included. The game does not automatically resize scene targets during play.

For maximum speed, keep SGSR1 and heat management Off, leave logs Off, and use Maximum performance or lower the expensive effects yourself. FPS gains depend on the device and scene; no phone benchmark is claimed for this build.

## Controls

- Separate mouse and controller sensitivity, alongside the existing touch sensitivity slider.
- An invert-vertical-look option.
- Optional gyro look with adjustable sensitivity. It turns off in menus, when focus is lost, and while the app is paused. Devices without a gyroscope can keep using touch, mouse or controller look.
- Existing button-free swipe look and customizable layouts remain available.

## Experimental Mali support

- Bundled **PanVK Kbase RC3** is available as an experimental choice for compatible Mali GPUs on **Android 15+**. It needs the supported `mali_kbase` kernel interface and the engine's Vulkan/texture features.
- Compatible Android/Bionic PanVK ZIPs can also be imported. Linux/glibc packages are rejected, and imported Mali packages are marked experimental.
- Automatic selection still uses Turnip for supported Adreno 6xx/7xx and System for other GPUs. PanVK requires an explicit selection.
- Missing imported drivers use System instead of an Adreno-only fallback. Startup reports missing required features even when app logging is Off.

**Mali gameplay is not yet verified on a physical device.** Driver import success means the files were installed; it does not guarantee that the game will run faster or launch on every Mali phone. Fully close and reopen the game after changing drivers.

## Launcher and setup

- Added the P.T. logo, cleaner menu styling and a GitHub link to this project. Removed the extra P.T./Android subtitle above the app name.
- Storage access is offered on first launch. Android 11+ opens the system All files access screen; Android 10 uses its runtime permission prompt. Denial and cancellation still leave file-picker imports available.
- Imports fall back to internal app storage when external app storage is unavailable. Existing imports can be found in either app folder.
- Save deletion, individual game graphics controls and the logging switch remain available.

## Checks before packaging

Native ARM64 and Android release builds, shader validation and release lint were checked, along with regression tests for imports, save deletion, presets, touch layouts, refresh selection, logging, storage permissions, sensor lifecycle and thermal settings. Physical-device FPS, visual quality and Mali compatibility still need testing.
