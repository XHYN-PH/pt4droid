# Bundled WinNative Turnip 1.19 Balanced

Binary package: https://github.com/WinNative-Emu/Drivers/releases/download/v1.19/WN-Turnip-1.19-b_Axxx.zip

Package SHA-256: 1a4fbae36c0062826b93179e99be00b3679b945dcd7fc7b062ba1db54e3cf55c

Corresponding build scripts and patches: https://github.com/WinNative-Emu/Drivers/tree/v1.19

Mesa source revision: 30da39187 (26.3.0-devel), as recorded by the upstream v1.19 release. Source: https://gitlab.freedesktop.org/mesa/mesa/-/tree/30da39187

Upstream package metadata: minApi 29, Android ARM64, library libvulkan_freedreno.so.

This app loads the unmodified driver as a separate optional Vulkan implementation through Adrenotools. The Balanced variant is bundled; the Performance variant forces GPU power constraints and is not the default. Existing explicitly imported driver choices are preserved. The upstream ELF LOAD segments have 4 KiB alignment; this exact binary is not claimed to support 16 KiB memory pages. The app uses System instead for its bundled/automatic selection on 16 KiB phones.

The WinNative repository LICENSE at this tag is GPL-3.0 and is reproduced alongside this notice. Mesa contains component-specific licenses; consult the corresponding source above. Upstream README and repository license labels differ; retaining the repository license does not relicense the port's MIT source code. No claim of device validation is made.
