<p align="center">
  <picture>
    <source media="(prefers-color-scheme: dark)" srcset="docs/images/pt4droid-logo.png">
    <source media="(prefers-color-scheme: light)" srcset="docs/images/pt4droid-logo-light.png">
    <img src="docs/images/pt4droid-logo-light.png" alt="P.T. logo" width="200">
  </picture>
</p>

<h1 align="center">pt4droid</h1>

<p align="center">
  <strong>P.T. for Android</strong><br>
  Maintained by <a href="https://github.com/XHYN-PH">XHYN-PH</a>
</p>

<p align="center">
  <a href="https://github.com/XHYN-PH/pt4droid/releases">Download APK</a>
  &nbsp;·&nbsp;
  <a href="#getting-started">Getting started</a>
  &nbsp;·&nbsp;
  <a href="https://github.com/XHYN-PH/pt4droid/issues">Report an issue</a>
</p>

---

**pt4droid** is an unofficial Android fork of [LoreanXavier's native P.T. PC port](https://github.com/LoreanXavier/pt-pc), built on [StoneColdOS's Android port](https://github.com/StoneColdOS/pt-pc). It runs native C++ code with a Vulkan renderer.

This fork adds customizable touch controls, an Android settings menu, individual graphics options, custom Vulkan driver imports, display refresh-rate selection and further mobile performance work.

> **Current test build: 0.2.1-test.5.** Game files are not included. Device compatibility, visual quality and sustained performance still need testing.

## Features

| Feature | What you can change |
| --- | --- |
| **Customizable touch controls** | Button positions, sizes, mappings, visibility and opacity, plus stick preferences and saved layouts. |
| **Invisible movement** | Touch and drag on the left half without a visible movement stick; hold to walk and lift to stop. |
| **Button-free looking** | Swipe on the right side of the screen, or choose a visible or hidden look joystick. |
| **Look sensitivity** | Adjust swipe and joystick sensitivity from **0.25× to 3.00×**. |
| **More input options** | Separate mouse/controller sensitivity, invert vertical look, and optional gyro look with its own sensitivity. |
| **Graphics settings** | Choose a preset or adjust individual game settings from the launcher or during play. |
| **Performance choices** | Cheaper AO/reflection sampling, optional Android frame pacing, and optional sustained performance/heat management. Resolution stays fixed. |
| **Sharper upscaling** | Optional SGSR1 at reduced render scale, with adjustable sharpness. Off preserves the fast compositor path. |
| **Vulkan drivers** | Automatic selection, bundled Turnip, System, experimental bundled PanVK for Mali, or a compatible imported ZIP. |
| **Display refresh rate** | Request the highest supported rate automatically, or choose a rate reported by your device. |
| **Save management** | Delete game save slots without clearing the app's data. |
| **Return to menu** | Leave the game from the in-game settings or Android settings, then start again from the launcher. |
| **Optional logging** | App diagnostics are **Off by default** and can be enabled when reporting a problem. |

The launcher uses a dark theme with serif text and red highlights. Gamepad, keyboard and mouse input are also available; the touch overlay lets physical mouse input pass through.

**Game graphics** is inside **Settings**. The launcher keeps one Settings entry, and the logo has a transparent background.

## Device requirements

| Requirement | Details |
| --- | --- |
| **Android** | Android 10 or newer. |
| **Architecture** | ARM64; a 64-bit Android installation is required. |
| **Graphics** | The selected driver must provide Vulkan 1.3 and the engine's required features and texture formats. |
| **Storage** | Allow at least about 1.4 GB for extracted game files, plus space for your source package and setup files. |

Automatic driver selection uses bundled Turnip on **Adreno 6xx/7xx** and the **system driver** on other GPUs, including Adreno 8xx. Imported Turnip drivers must support your specific Adreno GPU and Android version.

**Mali support is experimental.** Bundled PanVK Kbase RC3 requires **Android 15+**, a compatible Mali v9–v13 GPU and `mali_kbase` kernel interface. It is never selected automatically. Driver loading and feature checks are implemented; pt4droid gameplay and performance on Mali still need device testing. The system driver or an imported package must also supply the required compressed-texture formats and other Vulkan features.

## Getting started

1. Download the `pt4droid-*.apk` from [Releases](https://github.com/XHYN-PH/pt4droid/releases) and install it.
2. Open **pt4droid**. The first launch offers storage access for shared Download folders. Android 11+ opens its **All files access** screen; Android 10 asks for runtime storage permission. You can decline and still use file-picker imports.
3. Choose **Choose your P.T. .pkg**.
4. Select a fake PKG made from your own game dump and wait for extraction to finish.
5. Open **Settings** to configure controls, graphics and drivers, then start the game.

If your package is inside a `.zip`, `.rar` or `.7z` archive, extract it with a file manager before choosing it.

Alternatively, choose **I already have the extracted files** and supply:

- `chunk1.psarc`
- `texture.qar`
- `pathid_list_ps4.bin`

Place these in your Download folder, either loose or inside a `CUSA01127` folder, and allow **All files access** when requested. The US release, **CUSA01127**, is the reference game data. Compatibility with other regions needs testing.

Encrypted PlayStation Store packages are not supported.

**Updating an existing test installation:** install over the previous P.T. Test/pt4droid build when Android accepts the matching package and signing certificate. This release uses `com.ptport.app.test`. Keeping the existing installation retains its game files, saves and settings.

## Controls

Use **Settings → Touch controls** to select **Auto**, **Always show** or **Off**.

For a clear screen, select **Movement control → Invisible left-half movement** and **Look control → Swipe to look • no button**, then tap **Apply**. Touch and drag on the left to move, hold to keep walking, and lift to stop. Swipe on the right to look around. Both work at the same time; action buttons and Settings take priority. Each finger keeps its original role even if dragged across the middle.

**Visible joystick** remains the default movement option. Invisible movement always starts at your touch position, including when **Floating sticks** is off. Joystick size and dead zone still affect movement. The layout editor shows both stick guides so they remain adjustable.

| Look mode | Behavior |
| --- | --- |
| **Swipe to look • no button** | Drag on the right side to look around without a visible look stick. |
| **Visible joystick** | A visible look stick controls turning. |
| **Hidden joystick** | An invisible stick keeps turning while held away from its touch origin. |

Adjust **Look sensitivity** in Settings. To rearrange buttons, start the game and open **Android settings → Customize touch layout**. Save the layout when finished, or cancel to restore it.

Mouse and controller sensitivity are separate multipliers. **Invert vertical look** also applies to touch and gyro. **Gyroscope look** is optional, requires a gyroscope, and runs only during focused gameplay; it stops in menus and when the app loses focus.

To leave a session, open the in-game settings page or **Android settings**, choose **Return to menu** and confirm. The game finishes its normal shutdown before returning to the launcher. Progress follows the game's existing save points; returning does not create an extra checkpoint. You can start the game again, change settings, delete saves or quit from the launcher.

A built-in, USB or Bluetooth gamepad can also be used. Allow the nearby-device permission when requested for Bluetooth controllers.

The ending's voice input uses on-device speech recognition. If recognition does not work, **hold both controller triggers**, or use the touch **Name** control. The app does not require an internet connection.

## Graphics and performance

Open **Game graphics** from the launcher or Android settings during play.

Available options include:

- Frame limit, VSync and letterbox.
- Texture detail, anisotropic filtering and shadow resolution.
- Ambient occlusion and local reflections.
- Bloom, depth of field, motion blur, lens distortion and flare ghosts.
- Film grain and clarity.
- Ray-traced effects and ray quality **when supported by the active Vulkan device**.

**Settings** also provides scene render scale and an optimized mobile bloom option.

Additional options:

- **AO / Screen reflections → Performance:** fewer samples or march steps, preserving the effects with reduced detail. These sampling choices are independent of the base graphics preset.
- **Android frame pacing:** optional Swappy presentation for capped gameplay. Uncapped gameplay uses the existing path, and unsupported pacing falls back to the existing limiter. Off by default.
- **SGSR1 sharper upscaling:** an optional quality pass at fixed reduced resolution. It adds GPU work and uses full-size output; leave it Off for maximum speed. Off by default.
- **Sustained performance / heat management:** requests Android's supported sustained mode and lowers the effective FPS ceiling when hot (45/30/20 FPS). It does not change your saved frame limit or resolution. Off by default.

**Dynamic resolution is not included.** Render scale changes only when you change it yourself. No new quality pass or thermal FPS limit is enabled automatically.

| Preset | Behavior |
| --- | --- |
| **Custom** | Keep your individual game graphics choices. |
| **Maximum performance** | Reduce texture detail, shadows and effects, turn VSync off, remove the gameplay frame cap and limit scene render scale to 50%. |
| **Balanced** | Apply the balanced graphics profile with a gameplay cap of up to 60 FPS. |
| **Original (PS4)** | Restore the game's Original graphics preset. Render scale, frame cap and touch settings are configured separately. |

For a starting performance configuration, use **Maximum performance**, keep **Optimized mobile bloom** enabled and leave logs Off. Increase render scale or effects gradually if your device has room.

Renderer changes reduce repeated Vulkan state updates, unnecessary touch-input updates, screenshot copies and diagnostic work. **These changes are not a measured FPS guarantee.** Compare the same scene, camera position and settings after warmup when testing.

### High-refresh displays

Open **Settings → Display refresh rate** and choose **Highest supported • automatic**, or a rate such as **120 Hz** when your device reports it. Set the gameplay frame limit separately in **Game graphics**.

A 120 Hz screen does not guarantee 120 rendered FPS. Menus and paused gameplay remain capped at 60 rendered FPS, and Android may lower the display rate for battery, temperature or vendor policy.

### Custom Vulkan drivers

1. Open **Settings → Vulkan drivers → Import driver ZIP**.
2. Choose a compatible AdrenoTools-style ZIP containing `meta.json` and its ARM64 driver library.
3. Wait for the import result. A successful import selects the driver.
4. **Fully close and reopen the game** to load it.

To switch drivers later, open **Vulkan drivers**, choose the driver and press **Select**. If an import times out, copy the ZIP to local Downloads and try again. Import progress can be cancelled.

Driver compatibility depends on the GPU, Android version and driver build. If a custom driver causes problems, switch back to **Automatic** or **System Vulkan driver** and restart.

For Mali, select **PanVK Kbase RC3 • experimental**, or import an Android/Bionic PanVK package. Linux/glibc packages are rejected. Missing imported files fall back to System rather than loading an Adreno-only driver on another GPU. A successful ZIP import confirms installation, not game compatibility; startup reports missing required driver features even with logs Off.

## Starting a new save

Close the game, then choose **Delete saves** on the launcher and confirm.

This deletes the **two game save slots**. Game files, settings, touch layouts, drivers, caches and Extras unlocks are retained.

## Logs and bug reports

**Enable logs** is Off by default. To report a problem:

1. Turn it on in **Settings**, press **Apply**, and reproduce the problem.
2. Attach the new reports from **Download/PT-logs-test**, or the app-data fallback if that folder is unavailable.
3. Include your device, Android version, GPU, APK version, selected driver, graphics settings and steps to reproduce.

Driver-import diagnostics use `driver-import.log` in the app's data folder. Older reports are retained when you turn logging off. Android's own crash reporting and vendor diagnostics are outside the app's logging switch.

[Open an issue](https://github.com/XHYN-PH/pt4droid/issues) with these details.

## Limitations

- This is an experimental build; individual devices, drivers and scenes can still have issues.
- Ray-traced options require supported Vulkan ray queries. Unsupported options are disabled.
- HDR, VR, desktop upscalers, enhanced texture generation and frame generation are unavailable in this Android build.
- Speech recognition may need the controller-trigger or touch-button fallback.
- On dual-screen handhelds, the extra display can show the clock and subtitles. A connected TV may be treated as that display; change **Display → Second screen** if needed.

See [the test.3 update details](docs/android-test3-update.md) for the fixes, verification and remaining device-testing limits.

## Building from source

The test-release build uses **JDK 17**, **Android SDK 35**, **NDK r27 or newer**, **CMake 3.28+**, **Ninja** and **glslc**.

Configure your Android SDK and `ANDROID_NDK_HOME`, then set `PT_KEYSTORE` to your persistent signing key and `PT_STORE_PASSWORD` to its password. From the repository root, run:

```bash
bash tools/android/build_test_release.sh
```

Keep the signing key for compatible future updates. Use a higher `PT_VERSION_CODE` for each new release; `PT_VERSION` sets its version name. Both can be supplied as environment variables.

The inherited GitHub Actions workflow targets the upstream Android branch/tag conventions; a push to `pt4droid` alone does not enable an automatic release. Build locally or adapt [the workflow](.github/workflows/android.yml) for your fork.

## Credits

- **[XHYN-PH](https://github.com/XHYN-PH)** — pt4droid fork maintainer.
- **[LoreanXavier](https://github.com/LoreanXavier/pt-pc)** — original native P.T. PC port and Vulkan renderer. [Support the original project](https://patreon.com/loreanxavier).
- **[StoneColdOS](https://github.com/StoneColdOS/pt-pc)** — original Android port and handheld integration.
- **[Mesa Turnip](https://docs.mesa3d.org/drivers/freedreno.html)** and **[KIMCHI's AdrenoTools drivers](https://github.com/K11MCH1/AdrenoToolsDrivers)** — bundled Adreno Vulkan driver.
- **[libadrenotools](https://github.com/bylaws/libadrenotools)** — custom driver loading.
- **[SDL3](https://github.com/libsdl-org/SDL)** — Android integration, input and audio.
- **[whisper.cpp](https://github.com/ggml-org/whisper.cpp)**, OpenAI Whisper and Silero VAD — on-device voice recognition.
- **[Special Elite](https://fonts.google.com/specimen/Special+Elite)** by Astigmatic — second-screen typeface.
- **[LibertyRecompAndroid](https://github.com/vaduur/LibertyRecompAndroid)** — reference for the original Android driver integration.

The original PC project README is preserved in [README_UPSTREAM.md](README_UPSTREAM.md). Dependency notices remain in the source and APK.

The port code is licensed under [MIT](LICENSE). P.T. and its artwork belong to their respective rights holders, including Konami and Kojima Productions. This project is unofficial and is not affiliated with or endorsed by them.
