# pt4droid 0.2.1-test.12

Pacing recovery, earlier loading, and experimental Mali compatibility.

- Connects Android frame pacing to the game window. Uses the native limiter if presentation repeatedly blocks the requested rate despite GPU headroom.
- Requests a compatible display mode from the combined **FPS / display refresh** setting and updates the observed rate after display changes.
- Prepares player models, skeletons, and simulation during loading. Prepares cutscene models on two CPU workers, with a bounded cache.
- Starts stage requests earlier and overlaps the first texture wave with model and collision preparation, within the existing texture memory budget.
- Reduces VFX scratch clearing and repeated light calculations. Skips probe-map generation for small views and specializes composition without changing graphics settings.
- Adds fragment clipping for drivers without `shaderClipDistance`; removes the unused `fillModeNonSolid` requirement. Mirrors and point shadows keep their clipping.
- Uses the system driver when bundled Turnip is selected on a known non-Adreno GPU.

**Mali on Android 14:** start with **Automatic** or **System Vulkan driver**. Other Vulkan requirements still apply. Bundled PanVK still needs Android 15+ and compatible hardware.

Install over Test 11 to keep game files, saves, and settings. ARM64, Android 10+, version code **54**. Sustained performance preserves the selected FPS limit and graphics settings.

Focused checks cover pacing fallback, display lifecycle, memory limits and cancellation, VFX output and generation wrap, shadow invalidation, and shader validity. Hardware performance, Mali startup, and visuals still need phone testing.
