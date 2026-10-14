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
  <a href="https://github.com/XHYN-PH/pt4droid/releases/tag/pt4droid-v0.2.1-test.12">Download APK</a>
  &nbsp;·&nbsp;
  <a href="#getting-started">Getting started</a>
  &nbsp;·&nbsp;
  <a href="https://github.com/XHYN-PH/pt4droid/issues">Report an issue</a>
</p>

---

An unofficial native Android port of **P.T.**, with customizable touch controls, graphics settings and Vulkan driver support.

**Current build: [0.2.1-test.12](https://github.com/XHYN-PH/pt4droid/releases/tag/pt4droid-v0.2.1-test.12)** · ARM64 · Android 10+

Game files are not included. You need your own P.T. game dump. This is an experimental test build; compatibility and performance vary by device.

## Getting started

1. Download **`pt4droid-v0.2.1-test.12-arm64.apk`** from the [Test 12 release](https://github.com/XHYN-PH/pt4droid/releases/tag/pt4droid-v0.2.1-test.12) and install it.
2. Open **pt4droid** and choose **Choose your P.T. .pkg**.
3. Select a fake PKG made from your own game dump. Extract ZIP/RAR/7z archives first; encrypted PlayStation Store packages are unsupported.
4. Wait for extraction, then tap **Enter menu**.
5. Open **Settings** to configure controls and graphics, then choose **Start**.

Already have extracted files? Choose **I already have the extracted files**, place `chunk1.psarc`, `texture.qar` and `pathid_list_ps4.bin` in Downloads, loose or inside `CUSA01127`, and allow storage access. The US release **CUSA01127** is the reference data.

**Updating:** install over the matching Test 11 or earlier test app to keep game files, saves, settings and touch layouts. Avoid uninstalling first.

## Device compatibility

| Requirement | Details |
| --- | --- |
| Android | Android 10 or newer, with a 64-bit ARM64 installation |
| Graphics | Vulkan 1.3 plus the renderer's required features |
| Storage | About 1.4 GB for extracted game data, plus space for the APK and source package |

**Automatic** uses bundled Turnip on Adreno 6xx/7xx and the system driver on other GPUs, including Adreno 8xx.

**Mali and Xclipse support is experimental.** Test 12 adds clipping fallbacks for drivers without `shaderClipDistance` and no longer requires unused wireframe support. For Mali on Android 14, start with **Automatic** or **System Vulkan driver**. Other Vulkan requirements still apply. Bundled PanVK requires Android 15+ and compatible hardware.

To import a compatible driver, use **Settings → Vulkan drivers → Import driver ZIP**, then fully close and reopen the game. If it causes problems, select **Automatic** or **System Vulkan driver** and restart.

## Controls

- **Touch:** configure movement, look mode and sensitivity in **Settings**. Invisible left-half movement and right-side swipe look are available.
- **Layout:** during play, open **Android settings → Customize touch layout** to move, resize or hide buttons.
- **Gamepad, keyboard and mouse:** supported; mouse and controller sensitivity are separate settings.
- **Ending voice input:** uses on-device recognition. If needed, use the touch **Name** control or hold both controller triggers.
- **Return to menu:** available in the in-game settings. **Delete saves** on the launcher clears the game save slots.

## Graphics and performance

Use **Settings → Game graphics** for presets, FPS/display refresh, textures, shadows and effects. Start with **Balanced**, or choose **Maximum performance** for lower detail and render scale.

The FPS setting requests a matching display refresh rate or a supported multiple. Render scale and optional Android frame pacing are in Settings. Sustained performance keeps your chosen FPS limit; Android still controls temperature and clocks. Keep logs Off during normal play.

Test 12 refines pacing, prepares player assets earlier, reduces repeated lighting work, and adds Mali feature fallbacks. See the [changelog](docs/releases/pt4droid-v0.2.1-test.12.md). Phone performance comparisons still need testing.

## Reporting a problem

Enable logs in **Settings**, tap **Apply**, and reproduce the issue. [Open an issue](https://github.com/XHYN-PH/pt4droid/issues) with:

- Device, Android version, GPU and APK version.
- Selected driver, graphics settings and steps to reproduce.
- New logs from **Download/PT-logs-test** (or the app-data folder if shared storage is unavailable).

## For developers

[Android build instructions](docs/android.md) · [Test 12 changes](docs/releases/pt4droid-v0.2.1-test.12.md) · [Build validation](docs/android-test12-build-validation.md) · [Earlier releases](https://github.com/XHYN-PH/pt4droid/releases)

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
