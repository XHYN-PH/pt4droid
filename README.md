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
  <a href="https://github.com/XHYN-PH/pt4droid/releases/tag/pt4droid-v0.2.1-test.14">Download APK</a>
  &nbsp;·&nbsp;
  <a href="#getting-started">Getting started</a>
  &nbsp;·&nbsp;
  <a href="https://github.com/XHYN-PH/pt4droid/issues">Report an issue</a>
</p>

---

Play **P.T. on Android** with touch controls, graphics settings and controller support. This is an unofficial port maintained by **XHYN-PH**.

**Latest test release: [Test 14](https://github.com/XHYN-PH/pt4droid/releases/tag/pt4droid-v0.2.1-test.14)** · Android 10+

Game files are not included. You need your own P.T. game dump. This is still a test release, so some phones may have problems.

## Getting started

1. Download **pt4droid-v0.2.1-test.14-arm64.apk** from the [Test 14 release](https://github.com/XHYN-PH/pt4droid/releases/tag/pt4droid-v0.2.1-test.14) and install it.
2. Open the app, tap **Choose your P.T. .pkg**, and select your game package. See the game-file help below if it will not import.
3. Wait for the files to finish importing, then tap **Enter menu**.
4. Adjust your settings and tap **Start**.

**Updating from Test 13?** Install the new APK over the existing app to keep your game files, saves and settings. Avoid uninstalling first.

<details>
<summary>Game files and importing help</summary>

The importer supports fake PKGs made from your own game dump. Extract ZIP, RAR or 7z archives first; encrypted PlayStation Store packages are unsupported.

Already have extracted files? Choose **I already have the extracted files**. Place these files in Downloads, loose or inside a folder named `CUSA01127`, then allow storage access:

- `chunk1.psarc`
- `texture.qar`
- `pathid_list_ps4.bin`

The US game dump, **CUSA01127**, is the reference version.

</details>

## What's new in Test 14

- Simpler settings, with separate pages for graphics and camera, controls, performance, and drivers.
- An experimental **third-person view** option you can use with touch controls.
- Changing controls or logging settings keeps your graphics settings and FPS limit.
- **Delete saves** is available only on the main menu.
- Loading progress during graphics preparation and area loading.
- Further shadow, particle and loading improvements.
- Better recovery when Android changes the screen refresh rate, while keeping your selected FPS limit, including **Unlimited**.
- **Retry with system driver** if the game cannot start with your selected graphics driver.
- A separate Mali lighting option; Snapdragon/Adreno keeps its existing lighting shaders and native textures.

[Read the full Test 14 changes](docs/pt4droid-test.14.md).

## Controls and settings

- **Touch controls:** move, resize or hide buttons with **Customize touch layout** in the in-game Android settings.
- **Movement and looking:** use the invisible movement area and swipe-look controls. Adjust sensitivity in Settings.
- **Controllers, keyboard and mouse:** supported, with separate sensitivity settings.
- **Return to menu:** available in the in-game settings.
- **Delete saves:** available only on the main menu.
- **Ending voice input:** uses on-device recognition. The touch **Name** button or holding both controller triggers is also available.

Open **Settings → Graphics & camera** for your FPS limit, effects, shadows and third-person view. Third-person view is experimental; scripted scenes can still use first person.

Open **Controls** for touch, mouse and controller options, and **Performance & advanced** for scene resolution and frame pacing. Options marked **Restart required** take effect after restarting the game.

For graphics, start with **Balanced**. Try **Maximum performance** if the game feels slow. Keep logs Off during normal play.

## Will it work on my phone?

You need Android 10 or newer on a compatible 64-bit phone, Vulkan 1.3 support, and about 1.4 GB for the game files plus room for the app and cache.

Start with the **Automatic** graphics driver. **Mali and Xclipse support is experimental**; try **Automatic** or **System Vulkan driver** first.

If a custom driver cannot start the game, use **Retry with system driver** on the main menu. You can also switch back to **Automatic** or **System Vulkan driver** in **Settings → Graphics driver** and restart. The optional **PanVK** driver needs Android 15 or newer and compatible hardware; it may still fail on some phones.

On Mali, if the game runs out of texture memory, try **Compatibility texture size → 1024 or 512** in **Settings → Performance & advanced**, then restart. Smaller textures use less memory and may look less sharp. The **Mali lighting** option is also there; restart after changing it, and turn it off to compare with Test 13's lighting.

## Need help?

[Report a problem](https://github.com/XHYN-PH/pt4droid/issues) with your phone model, Android version, game version, selected driver and what happened.

If you can, turn logs on in Settings, tap **Apply**, and reproduce the problem. Attach the new log from **Download/PT-logs-test**; if shared storage is unavailable, check the app's data folder. Turn logs off afterwards.

<details>
<summary>Build the app from source</summary>

[Android build instructions](docs/android.md).

</details>

## Credits

- **[XHYN-PH](https://github.com/XHYN-PH)** — pt4droid fork maintainer.
- **[LoreanXavier](https://github.com/LoreanXavier/pt-pc)** — original native P.T. PC port and Vulkan renderer. [Support the original project](https://patreon.com/loreanxavier).
- **[StoneColdOS](https://github.com/StoneColdOS/pt-pc)** — original Android port and handheld integration.
<details>
<summary>Additional credits</summary>

- **[Mesa Turnip](https://docs.mesa3d.org/drivers/freedreno.html)** and **[KIMCHI's AdrenoTools drivers](https://github.com/K11MCH1/AdrenoToolsDrivers)** — bundled Adreno Vulkan driver.
- **[libadrenotools](https://github.com/bylaws/libadrenotools)** — custom driver loading.
- **[SDL3](https://github.com/libsdl-org/SDL)** — Android integration, input and audio.
- **[whisper.cpp](https://github.com/ggml-org/whisper.cpp)**, OpenAI Whisper and Silero VAD — on-device voice recognition.
- **[Special Elite](https://fonts.google.com/specimen/Special+Elite)** by Astigmatic — second-screen typeface.
- **[LibertyRecompAndroid](https://github.com/vaduur/LibertyRecompAndroid)** — reference for the original Android driver integration.

The original PC project README is preserved in [README_UPSTREAM.md](README_UPSTREAM.md). Dependency notices remain in the source and APK.

</details>

The port code is licensed under [MIT](LICENSE). P.T. and its artwork belong to their respective rights holders, including Konami and Kojima Productions. This project is unofficial and is not affiliated with or endorsed by them.

