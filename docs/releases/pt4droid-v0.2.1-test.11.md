# pt4droid 0.2.1-test.11

Performance refinements for rendering and loading, plus simpler frame-rate settings.

- Reuses unchanged shadow poses and static backgrounds under stable lights.
- Precomputes probe lighting maps and caches shared VFX work.
- Uploads textures asynchronously and consumes prepared textures individually.
- Removes an unnecessary composition depth load and separates submission/presentation timings in logs.
- Combines **FPS limit / display refresh** in Game graphics. Requests a matching rate or a supported multiple; Unlimited requests the highest display rate.
- **Sustained performance** keeps your selected FPS limit and graphics settings. Removes the old automatic 45/30/20 FPS thermal caps.

Install over Test 10 to keep game files, saves and settings. ARM64, Android 10+, version code **53**.

Focused checks cover shadow invalidation, VFX output/order, texture readiness and upload lifetimes, and display selection. Phone performance and visual behavior still need device testing. Android may override display requests or sustained performance hints. Existing Mali/driver requirements still apply.
