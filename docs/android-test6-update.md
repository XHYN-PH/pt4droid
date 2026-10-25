# test.6: sluggish look investigation and performance priorities

The supplied test.5 log does not show a fixed 30 FPS cap. It records an uncapped run at 1200×540 with VSync Off, an imported PurpleVK Adreno 750 driver, and no warning/error/crash entries. After startup, samples range from 51 to 97 FPS; the heavier sections mostly run at 51–58 FPS. These ten-second samples do not identify the exact moment of each look gesture or capture the capped presets separately.

Two source defects can increase perceived sluggishness:

1. The software limiter restarted its deadline at the end of an over-budget frame, then added a full new interval. With a 60 FPS cap, a 19 ms frame could therefore take approximately 35.7 ms, or 28 FPS. test.6 measures elapsed work from the start of the frame and sleeps only when time remains. This arithmetic example is not a measured phone benchmark.
2. The camera's direct-look rendering path required keyboard prompts. Touch movement/actions use gamepad prompts. The Android path now removes interpolation only from the direct delta consumed by the last simulation tick and applies pending direct input, while preserving interpolation for controller rotation. It remains gated by menu/pause state, camera locks, cutscenes, free camera and the ending.

The import screens previously started the game directly. test.6 separates import completion from Start: Enter menu opens the ready screen, and only Start begins the session. Existing save/session guards and microphone handling remain in place.

## What the log can tell us

The previous mobile GPU labels were shifted: the reported zero G-buffer time was the mirror pass, while the reported occlusion time was the main G-buffer. test.6 corrects the labels. Lighting timing also includes work such as occlusion, so this log cannot isolate SSAO cost.

In heavier sections, shadow samples are approximately 2.4–5.2 ms, main G-buffer work is about 1.0–2.4 ms, and lighting is about 1.6–3.5 ms. The separate VFX reports show approximately 2.4–5.8 ms of CPU preparation per rendered frame. Bloom and screen-reflection samples are generally smaller in this configuration. This suggests CPU particle work and shadows deserve profiling before adding new upscalers or lowering unrelated effects.

## Next performance work, in order

1. **Particle light selection and allocations.** `LightBlockAt` builds temporary candidate/probe vectors and stable-sorts them for effect draw objects, although only three lights/probes are used. Separate particle draws repeat this work. A fixed top-three selection could reduce allocations and sorting while preserving tie order, selected lights, probe weights and visuals. Compare the full light blocks with the existing implementation on randomized scenes before adopting it.
2. **Shadow reuse.** Investigate reusing tiles when their lights and casters are unchanged. Correct invalidation must cover doors, characters, skinned meshes, flashlight motion, changing light properties and stage transitions. Image comparisons and GPU measurements are necessary before enabling caching.
3. **Separate CPU and waiting costs.** Add bounded aggregate timing for simulation, scene build, VFX preparation, fence/acquire waits and presentation when diagnostics are enabled. Keep logs Off free of these extra measurements. This would explain the difference between roughly 11–12 ms of GPU work and approximately 18–20 ms rendered frame intervals.
4. **Profile Original separately.** Capture the same warmed-up camera/scene with Balanced and Original after the limiter fix. Original enables heavier effects and shadows; its actual costs should guide the next change. A frame cap, requested screen refresh rate and achieved FPS are separate values.

These items are investigation targets, not FPS promises. test.6 does not reduce preset graphics quality or enable a new GPU pass, dynamic resolution, heat management, or an upscaler automatically.
