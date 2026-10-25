#pragma once
#include <atomic>
namespace pt {
inline std::atomic<bool> diagnostic_fresh_shadows{false};
inline std::atomic<bool> diagnostic_shadow_trace{false};
}
