#pragma once
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <string>
#include <string_view>

namespace pt::android {
// Mr. Purple T30 is 16 KiB aligned and declares Android 11 (API 30) minimum.
inline bool BundledTurnipSupportsPageSize(long bytes){return bytes>0&&bytes<=16384;}
inline bool BundledTurnipSupportsRuntime(int sdk,long bytes){return sdk>=30&&BundledTurnipSupportsPageSize(bytes);}
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
    // T30 upstream partially dropped A710/A720 because of device bugs.
    return model>=600 && model<800 && model!=710 && model!=720;
}
inline bool BundledTurnipCompatible(std::string_view device_name) {
    // A failed system probe must not override an explicit driver choice.
    if (device_name.empty()) return true;
    std::string name(device_name);
    std::transform(name.begin(), name.end(), name.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    const auto label=name.find("adreno");if(label==std::string::npos)return false;
    const auto number=name.find_first_of("0123456789",label+6);
    const auto model=number==std::string::npos?0:std::strtoul(name.c_str()+number,nullptr,10);
    return model!=710 && model!=720;
}
}
