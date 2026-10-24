#pragma once
#include <array>
#include <string_view>
namespace pt::android {
struct TurnipProfile {const char* label;const char* algorithm;const char* flags;};
inline constexpr std::array<TurnipProfile,5> kTurnipProfiles{{
    {"Driver default",nullptr,nullptr},
    {"Prefer system memory","prefer_sysmem",nullptr},
    {"Prefer tile memory","prefer_gmem",nullptr},
    {"Tune small passes","bandwidth","tune_small"},
    {"Tile memory + small passes","prefer_gmem","tune_small"},
}};
inline const TurnipProfile& SelectTurnipProfile(int index){return kTurnipProfiles[index>=0&&size_t(index)<kTurnipProfiles.size()?index:0];}
}
