# pt4droid 0.2.1-test.16 candidate

This is a test candidate, package com.ptport.app.test, version code 58. It follows Test 15 and needs phone testing before stable. Save/update continuity and broad lifecycle/recovery remain outside the requested scope.

## Launcher and controls

- The teaser has a small text-only **Hold to skip** control. Hold continuously for three seconds; release, dragging away, focus loss and backgrounding cancel it. Controller B/Start and keyboard Enter/Space/Back use the same duration. Video completion or decoding failure still enters the launcher. Replay remains available.
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

The build pins **WinNative Turnip 1.19 Balanced**:

- https://github.com/WinNative-Emu/Drivers/releases/tag/v1.19
- WN-Turnip-1.19-b_Axxx.zip SHA-256: 1a4fbae36c0062826b93179e99be00b3679b945dcd7fc7b062ba1db54e3cf55c
- The unmodified upstream library has 4 KiB ELF LOAD alignment. Bundled/automatic selection uses System on 16 KiB phones. Explicitly imported selections are preserved.
- License/source notices are included under assets/notices/turnip. The upstream repository's tag LICENSE is GPL-3.0; Mesa component licenses also apply. The Performance variant's power-lock behavior is not the bundled default.

Optional Mali/Xclipse driver builds require GPU-specific kernel support. They are experimental, not a compatibility promise. The unified source currently targets Bifrost Mali through kbase and Xclipse 920/530; its Bifrost backend does not establish Immortalis support. The separately bundled PanVK Kbase RC3 has its own modern-GPU and Android requirements. Do not combine these support lists or assume every Mali/Immortalis/Xclipse phone works.

## FSR feasibility

A native Android temporal FSR backend is feasible in principle. The engine already provides motion, depth, exposure, jitter and reset inputs. Its current FSR backend loads a Windows DLL through Windows APIs and cannot be enabled by an Android settings toggle. A source-built Vulkan backend, Android portability fixes, shader compilation, feature checks and device image/performance validation are required. AMD's open-source FSR 2 Vulkan implementation is one possible starting point. No FSR 1 substitute is added, and no Android FSR implementation is claimed in this candidate. SGSR1 remains available.

## Verification and remaining phone checks

Focused host checks cover hold timing/cancellation, vibration scaling, MediaStore copy/publish/rollback, refresh votes and overrides, settings-page isolation, touch input and layout editing. The accompanying verification report records the final build, package identity, hash and checks.

On phones, test tap versus three-second hold, replay, camera opacity during edit/cancel, all toolbar modes, 0/50/100 percent rumble and the ending cue, gallery PNG visibility and failure feedback, selected caps with VSync/pacing, and background/focus/surface transitions affecting these new controls. Compare bundled WinNative 1.19 against System and the previous imported driver with matched settings. Complete a full playthrough and sustained memory/thermal checks before stable.
