#pragma once
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <string>
#include <string_view>

namespace pt::android {
// WinNative 1.19 is an unmodified 4 KiB-aligned ELF. Do not load it on 16 KiB devices.
inline bool BundledTurnipSupportsPageSize(long bytes){return bytes>0&&bytes<=4096;}
inline bool IsMaliGpu(std::string_view device_name){
    std::string name(device_name);std::transform(name.begin(),name.end(),name.begin(),[](unsigned char c){return static_cast<char>(std::tolower(c));});
    return name.find("mali")!=std::string::npos||name.find("immortalis")!=std::string::npos;
}
inline bool PreferBundledTurnip(std::string_view device_name) {
    std::string name(device_name);
    std::transform(name.begin(),name.end(),name.begin(),[](unsigned char c){return static_cast<char>(std::tolower(c));});
    const auto label=name.find("adreno");if(label==std::string::npos)return false;
    const auto number=name.find_first_of("0123456789",label+6);if(number==std::string::npos)return false;
    const auto model=std::strtoul(name.c_str()+number,nullptr,10);
    return model>=600 && model<800;
}
inline bool BundledTurnipCompatible(std::string_view device_name) {
    // A failed system probe must not override an explicit driver choice.
    if (device_name.empty()) return true;
    std::string name(device_name);
    std::transform(name.begin(), name.end(), name.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return name.find("adreno") != std::string::npos;
}
}
