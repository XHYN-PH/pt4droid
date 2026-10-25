# pt4droid 0.2.1-test.6

This update fixes an extra frame-limit wait that could make Balanced and other capped settings feel much slower than expected, and improves direct touch look.

## Smoother looking and frame limits

- Fixed the software frame limiter adding another full frame interval when rendering was already late. A slow frame now continues immediately; faster frames wait only for the unused part of their frame budget.
- Fixed direct swipe and gyro look depending on keyboard button prompts. Touch controls can keep their gamepad prompts while the camera follows direct look at the render rate.
- Direct look no longer inherits the previous tick's interpolation delay. Controller look keeps its existing interpolation, including when mixed with gyro or swipe input.
- Camera locks, cutscenes, menus, pitch limits and sensitivity options remain respected. Resolution and graphics quality stay under your control.

## Launcher and import flow

- Renamed the launcher's **Continue** button to **Start**.
- After a PKG import, **Enter menu** opens the launcher so you can change settings before starting.
- **Delete the .pkg and enter menu** also returns to the launcher. A refused deletion still leaves Enter menu available.
- Finding extracted files after granting storage access now opens the launcher instead of starting the game automatically.

## Diagnostics and build fixes

Install over a matching test.5 installation to retain your game files, saves, settings and imported drivers. Please compare Balanced and Original in the same scene on your phone. Physical-device FPS gains have not been measured for this build. Mali/PanVK support remains experimental; dynamic resolution is not included.
