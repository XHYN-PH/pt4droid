# Bundled Mr. Purple Turnip T30

Author: Mr_Purple_666. This is an unmodified upstream Adrenotools binary.

Release: https://github.com/MrPurple666/purple-turnip/releases/tag/vturnip_mrpurple_T30-toasted.adpkg

Binary: https://github.com/MrPurple666/purple-turnip/releases/download/vturnip_mrpurple_T30-toasted.adpkg/turnip_mrpurple_T30-toasted.adpkg.zip

Archive SHA-256: f65b2d3353fd4aa7190bb5426b94468e99ffea7a58a830bc0c4651db89353227 (matches GitHub release asset digest).

vulkan.purple.so SHA-256: 1d80dfa019659b008e4669311db5b1e4a02af59ff1d5458a98e2f5fe18ed013b

Upstream metadata is preserved in meta.json: Mesa 26.3.0-T30, Vulkan 1.4.359, min API 30. All ELF LOAD segments have 16 KiB alignment. The file is packaged as libvulkan_turnip.so because Android extracts lib*.so names; its bytes are unchanged.

Upstream partially dropped Adreno 710/720 support; the bundled selector uses System on those models and Android 10. Automatic remains conservative on Adreno 6xx/7xx. Other GPUs use System; Adreno 8xx is an explicit selection. Imported choices remain unchanged.

Mesa component license information is reproduced in Mesa-license.rst; source and author acknowledgments are linked by the release above and Mesa: https://gitlab.freedesktop.org/mesa/mesa
The release repository distributes binaries and does not identify an exact corresponding Mesa revision/build recipe. Do not claim that this package can be reproduced bit-for-bit from that repository. No device validation or performance improvement is claimed.
