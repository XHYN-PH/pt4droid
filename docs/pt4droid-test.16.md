# pt4droid 0.2.1-test.16 candidate

This is a test candidate, package com.ptport.app.test, version code 58. It follows Test 15 and needs phone testing before stable. Save/update continuity and broad lifecycle/recovery remain outside the requested scope.

## Launcher and controls

- The teaser has a small text-only **Hold to skip** control. Hold continuously for three seconds; release, dragging away, focus loss and backgrounding cancel it. Controller A/B/Start and keyboard Enter/Space/Back use the same duration. Video completion or decoding failure still enters the launcher. Replay remains available.
- FSR settings are inside **Settings → Game graphics → FSR settings** in the launcher. The separate launcher-home button is removed. This relocation does not introduce a backend.
- The camera toolbar uses the touch controls' pale outline, pressed fill and live opacity, including restoring opacity when an edit is cancelled. The opaque toolbar backing and menu-style button rectangles are removed.
- Controls has **Phone vibration strength**, from 0 to 100 percent. The game's master rumble must also be enabled. Phones without amplitude control can only use hardware-default vibration or Off; the page explains this.
- External PanVK/Mali/Xclipse project-link buttons are removed from the driver page. Driver import remains available.
- Area preparation no longer posts the bottom progress panel during play. Initial graphics preparation still has progress feedback.

## Photo saving

Android no longer relies on HOME/USERPROFILE and a relative Pictures directory. Photo mode first writes a PNG in the app's writable photos directory, then copies it into **Pictures/PT Photos** using Android MediaStore scoped storage. Gallery work uses a bounded worker queue instead of blocking the UI thread. The pending gallery item is published only after the copy succeeds; failures remove the partial gallery item and retain the original PNG. Toast feedback distinguishes gallery success, failure and a busy queue. Native GPU readback and PNG encoding remain synchronous; making those asynchronous is a separate performance improvement.

## Refresh requests

Surface requests may now use Android's non-seamless change strategy when switching to the selected rate. Recovery considers the app-visible rate as well as the physical display mode, and surface recreation/resize invalidates the cached request. Retry bounds remain in place; unrelated changes do not alter the selected FPS cap. Android power/game policies and actual GPU frame time can still limit delivered FPS.

## Bundled driver

The build pins **Mr. Purple Turnip T30** (Mesa 26.3.0-devel / Vulkan 1.4.359):

- https://github.com/MrPurple666/purple-turnip/releases/tag/vturnip_mrpurple_T30-toasted.adpkg
- Archive SHA-256: f65b2d3353fd4aa7190bb5426b94468e99ffea7a58a830bc0c4651db89353227, matching the official release asset digest.
- All unmodified upstream ELF LOAD segments have 16 KiB alignment. Android 11+ is required. Android 10 uses System.
- Upstream partially dropped Adreno 710/720 because of device bugs; those models use System. Automatic remains conservative on other Adreno 6xx/7xx; Adreno 8xx needs an explicit selection. Imported selections are preserved.
- Author attribution, upstream metadata and Mesa license/source information are included under assets/notices/turnip. The binary release does not specify an exact reproducible source revision.

Optional Mali/Xclipse driver builds require GPU-specific kernel support. They are experimental, not a compatibility promise. The unified source currently targets Bifrost Mali through kbase and Xclipse 920/530; its Bifrost backend does not establish Immortalis support. The separately bundled PanVK Kbase RC3 has its own modern-GPU and Android requirements. Do not combine these support lists or assume every Mali/Immortalis/Xclipse phone works.

## FSR feasibility

A native Android temporal FSR backend is feasible in principle. The engine already provides motion, depth, exposure, jitter and reset inputs. Its current FSR backend loads a Windows DLL through Windows APIs and cannot be enabled by an Android settings toggle. A source-built Vulkan backend, Android portability fixes, shader compilation, feature checks and device image/performance validation are required. AMD's open-source FSR 2 Vulkan implementation is one possible starting point. No FSR 1 substitute is added, and no Android FSR implementation is claimed in this candidate. SGSR1 remains available.

## Verification and remaining phone checks

Focused host checks cover hold timing/cancellation, vibration scaling, MediaStore copy/publish/rollback, refresh votes and overrides, settings-page isolation, touch input and layout editing. The accompanying verification report records the final build, package identity, hash and checks.

On phones, test tap versus three-second hold, replay, camera opacity during edit/cancel, all toolbar modes, 0/50/100 percent rumble and the ending cue, gallery PNG visibility and failure feedback, selected caps with VSync/pacing, and background/focus/surface transitions affecting these new controls. Compare bundled Mr. Purple T30 against System and the previous imported driver with matched settings. Complete a full playthrough and sustained memory/thermal checks before stable.
