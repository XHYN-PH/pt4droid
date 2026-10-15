# pt4droid Test 14

Test 14 is a test build for player feedback. Install it over Test 13 to keep your game data.

## What's changed

- Settings are grouped into Graphics & camera, Controls, Performance & advanced, and Graphics driver.
- Third-person view is now available in Graphics & camera. It is experimental; scripted scenes can still use first person.
- Applying controls or other options keeps your graphics and FPS limit. Presets apply when you select a different preset.
- Delete saves is available only on the main menu.
- Graphics preparation and area loading now show progress. Texture preparation gives priority to the model being loaded next.
- Shadow and particle processing reuse more work without removing effects or lowering your chosen shadow quality.
- Refresh requests recover when Android changes the display rate. Your selected FPS limit stays unchanged, including Unlimited.
- A graphics startup failure offers Retry with system driver on the main menu.
- Mali uses a separate lighting path to reduce repeated image reads and fog calculations. Snapdragon keeps its existing lighting shaders and native texture path.

## Settings to try

Open Settings → Graphics & camera to adjust the FPS limit, shadows, effects or third-person view.

Open Settings → Performance & advanced for scene resolution, frame pacing and the Mali lighting option. Mali lighting and compatibility texture size need a game restart. Turn off the Mali lighting option to compare with Test 13's lighting path.

For Mali, start with Automatic or System Vulkan. PanVK remains experimental.

## Feedback

Compare with Test 13 using the same settings and route. Please check lighting, mirrors, shadows, loading pauses, pause/resume and a longer play session. Include your phone model, driver, settings and a log if you find a problem.

Performance gains still need device testing. This build is not labeled stable.
