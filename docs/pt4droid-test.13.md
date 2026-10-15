# pt4droid — Test 13

Test 13 improves texture support, loading and display settings. It is still a test release, so please report problems on your phone.

## What's changed

- Improved texture support for Mali phones, including some missing surfaces and effects.
- Added **Compatibility texture size** options for phones that need to use less texture memory. Smaller settings may look less sharp; restart after changing them.
- Improved loading of new areas and cutscenes.
- Improved recovery of FPS and screen refresh settings after returning to the game, including **Unlimited** mode.
- Performance improvements for lighting and visual effects.
- Added clearer messages when a graphics driver cannot start the game.

## Install or update

Download **pt4droid-v0.2.1-test.13-arm64.apk** from the [Test 13 release](https://github.com/XHYN-PH/pt4droid/releases/tag/pt4droid-v0.2.1-test.13) and install it.

Install over Test 12 to keep your game files, saves and settings. Game files are not included; you need your own P.T. game dump.

## Compatibility

- Android 10 or newer on a compatible 64-bit phone, with Vulkan 1.3 support.
- For Mali and Xclipse, start with **Automatic** or **System Vulkan driver**. Support is experimental.
- If a Mali phone runs out of texture memory, try **Compatibility texture size → 1024 or 512**, then restart.
- The optional **PanVK** driver needs Android 15 or newer and compatible hardware. If it cannot start the game, switch to **System Vulkan driver** and restart.

Please report crashes, missing textures or other problems with your phone model and selected driver.
