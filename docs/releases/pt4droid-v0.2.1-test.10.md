# pt4droid 0.2.1-test.10

Test 10 adds refinements 1–6 on top of Test 9. Existing graphics settings and render scale are retained.

- **Shadow reuse:** prepares caster metadata once per frame, reducing repeated tile checks while preserving updates for moving geometry and alpha changes.
- **Lighting:** limits probe-volume work to conservative screen bounds. AO, probes and direct-light timings are logged separately.
- **Particles:** batches first-use texture uploads and prepares constant effect expressions once. Simulation and transparency ordering are preserved.
- **Loading:** overlaps bounded texture preparation with model uploads and reclaims prepared data sooner. Cached stages skip empty preparation work.
- **Composition and buffers:** specializes common composition and tonemap modes using the same rendering equations and pass order.
- **Diagnostics:** adds frame-interval percentiles, hitch counts and CPU timing splits to help investigate slow or uneven gameplay. Logs remain Off by default.

## Experimental texture compatibility

The Test 8/9 texture fallback remains included. Supported BC formats keep native compressed uploads. Mali/PanVK and Xclipse remain experimental; Vulkan 1.3 and required device features still apply. Bundled PanVK requires Android 15+ and compatible hardware.

Test 10 does not fix the reported Mali Android 14 startup failure caused by missing Vulkan features.

## Updating

Install over the matching previous test app to retain game files, saves, settings and layouts. Package `com.ptport.app.test`, version code **52**, ARM64, Android 10+. The existing test signing key is reused. Game files are not included.
