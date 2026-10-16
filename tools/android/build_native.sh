#!/usr/bin/env bash
# Builds the Android native part (docs/android.md) and lays it out for Gradle:
#   android/app/libs/arm64-v8a/  libmain.so, libSDL3.so, libadrenotools' hooks, the Turnip driver
#   android/app/libs/arm64-v8a/  also whisper.cpp's libwhisper.so, libggml*.so (the speech recognizer for the ending)
#   android/app/assets/          the compiled shaders, the fonts, the speech models and pt_assets.txt (what the game unpacks
#                                on first start)
# Needs ANDROID_NDK_HOME (NDK r27 or newer), cmake 3.28+, ninja and glslc on the PATH.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
BUILD="${BUILD_DIR:-$ROOT/build/android}"
APP="$ROOT/android/app"
NDK="${ANDROID_NDK_HOME:?set ANDROID_NDK_HOME to the NDK folder}"
GLSLC="${GLSLC:-$(command -v glslc)}"

# WinNative Turnip 1.19 Balanced, Android/Bionic Adrenotools package.
# Preserve upstream notices/source information in assets/notices/turnip/.
TURNIP_URL="https://github.com/WinNative-Emu/Drivers/releases/download/v1.19/WN-Turnip-1.19-b_Axxx.zip"
TURNIP_SHA256="1a4fbae36c0062826b93179e99be00b3679b945dcd7fc7b062ba1db54e3cf55c"
PANVK_URL="https://github.com/zenithblue-oss/panvk-kbase-android/releases/download/panvk-kbase-v0.1.0-rc3/libvulkan_panfrost-android-aarch64.so"
PANVK_SHA256="b46b9f0416f207a05482349dffb37b6f3d84c20762151c623774831ce3939445"

cmake -G Ninja -S "$ROOT" -B "$BUILD" \
  -DCMAKE_TOOLCHAIN_FILE="$NDK/build/cmake/android.toolchain.cmake" \
  -DANDROID_ABI=arm64-v8a \
  -DANDROID_SUPPORT_FLEXIBLE_PAGE_SIZES=ON \
  -DANDROID_PLATFORM=android-29 \
  -DANDROID_STL=c++_static \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_CXX_SCAN_FOR_MODULES=OFF \
  -DVulkan_GLSLC_EXECUTABLE="$GLSLC" \
  -DPT_UPSCALERS=OFF -DPT_OPENXR=OFF
# -k 0: every compile error of a run, not just the first
cmake --build "$BUILD" --target pt pt_shaders ptsetup pt_voice_runtime -- -k 0

LIBS="$APP/libs/arm64-v8a"
rm -rf "$LIBS" "$APP/assets"
mkdir -p "$LIBS" "$APP/assets/shaders" "$APP/assets/fonts" "$APP/assets/voice/licenses"

cp "$BUILD/libmain.so" "$LIBS/"
for lib in libSDL3.so libptsetup.so libmain_hook.so libfile_redirect_hook.so libgsl_alloc_hook.so libhook_impl.so; do
  found="$(find "$BUILD" -ignore_readdir_race -name "$lib" -type f -print -quit)"
  if [ -z "$found" ]; then
    echo "missing $lib in $BUILD" >&2
    exit 1
  fi
  cp "$found" "$LIBS/"
done
# the speech recognizer (src/engine/voice): whisper, ggml and ggml's CPU code, one library per ARM level it was built for
for lib in libwhisper.so libggml.so libggml-base.so; do
  cp "$BUILD/voice/$lib" "$LIBS/"
done
cpu_libs=$(find "$BUILD/voice" -maxdepth 1 -name 'libggml-cpu*.so' | wc -l)
if [ "$cpu_libs" -eq 0 ]; then
  echo "no libggml-cpu*.so in $BUILD/voice" >&2
  exit 1
fi
cp "$BUILD"/voice/libggml-cpu*.so "$LIBS/"
"$NDK"/toolchains/llvm/prebuilt/linux-x86_64/bin/llvm-strip --strip-unneeded "$LIBS"/*.so

TURNIP_ZIP="$BUILD/turnip.zip"
if [ ! -f "$TURNIP_ZIP" ] || ! echo "$TURNIP_SHA256  $TURNIP_ZIP" | sha256sum -c --status; then
  curl -fsSL -o "$TURNIP_ZIP" "$TURNIP_URL"
fi
echo "$TURNIP_SHA256  $TURNIP_ZIP" | sha256sum -c
driver="$(unzip -p "$TURNIP_ZIP" meta.json | python3 -c 'import json,sys; print(json.load(sys.stdin)["libraryName"])')"
# the package manager only unpacks lib*.so; the game asks libadrenotools for this name (src/engine/platform/android.cpp)
unzip -p "$TURNIP_ZIP" "$driver" > "$LIBS/libvulkan_turnip.so"
PANVK_LIB="$BUILD/panvk-rc3.so"
if [ ! -f "$PANVK_LIB" ] || ! echo "$PANVK_SHA256  $PANVK_LIB" | sha256sum -c --status; then
  curl -fsSL -o "$PANVK_LIB" "$PANVK_URL"
fi
echo "$PANVK_SHA256  $PANVK_LIB" | sha256sum -c
cp "$PANVK_LIB" "$LIBS/libvulkan_panfrost.so"

mkdir -p "$APP/assets/notices"
cp -R "$ROOT"/assets/notices/* "$APP/assets/notices/"
cp "$BUILD"/shaders/*.spv "$APP/assets/shaders/"
cp "$ROOT"/assets/fonts/*.ttf "$ROOT"/assets/fonts/OFL-*.txt "$APP/assets/fonts/"
cp "$BUILD/voice/ggml-base.en-q5_1.bin" "$BUILD/voice/ggml-silero-v6.2.0.bin" "$APP/assets/voice/"
cp "$BUILD"/voice/licenses/* "$APP/assets/voice/licenses/"
(
  cd "$APP/assets"
  {
    echo "# $(git -C "$ROOT" rev-parse --short HEAD 2>/dev/null || echo local) $(date -u +%Y%m%dT%H%M%SZ)"
    find shaders fonts voice notices -type f | sort
  } > pt_assets.txt
)
echo "native build laid out in $APP"
ls -la "$LIBS"
