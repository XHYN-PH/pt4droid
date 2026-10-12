if(NOT ANDROID)
  return()
endif()
# Stable AGDK Swappy 2.1.3, including Google's 16 KiB page-size fix.
FetchContent_Declare(pt_swappy_archive
  URL https://dl.google.com/android/maven2/androidx/games/games-frame-pacing/2.1.3/games-frame-pacing-2.1.3.aar
  DOWNLOAD_NAME games-frame-pacing.zip
  URL_HASH SHA256=a9f83e0cf47d694ed2cea0ae995d5cf7411060d9d4b680610430d03d6b20ea27)
FetchContent_GetProperties(pt_swappy_archive)
if(NOT pt_swappy_archive_POPULATED)
  FetchContent_Populate(pt_swappy_archive)
endif()
set(PT_SWAPPY_MODULE "${pt_swappy_archive_SOURCE_DIR}/prefab/modules/swappy_static")
add_library(pt_swappy STATIC IMPORTED)
set_target_properties(pt_swappy PROPERTIES
  IMPORTED_LOCATION "${PT_SWAPPY_MODULE}/libs/android.${ANDROID_ABI}/libswappy_static.a"
  INTERFACE_INCLUDE_DIRECTORIES "${PT_SWAPPY_MODULE}/include")
target_link_libraries(pt_engine PUBLIC pt_swappy android log)
