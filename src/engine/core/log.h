#pragma once

#include <cstdint>
#include <filesystem>
#include <format>
#include <string_view>
#include <utility>
#include <atomic>
#include <cstdlib>

namespace pt {

enum class LogLevel { Debug, Info, Warn, Error };

void LogWrite(LogLevel level, std::string_view text);
bool LogEnabled();
inline std::atomic<bool> g_log_verbose{[] {
    if(const char* value=std::getenv("PT_LOG_VERBOSE"))return std::string_view(value)=="1";
#ifdef __ANDROID__
    return false;
#else
    return true;
#endif
}()};
inline void LogSetVerbose(bool enabled){g_log_verbose.store(enabled,std::memory_order_relaxed);}
inline bool LogAccept(LogLevel level,std::string_view text){
    if(!LogEnabled())return false;
    if(g_log_verbose.load(std::memory_order_relaxed))return true;
    if(level==LogLevel::Debug)return false;
    if(level!=LogLevel::Info)return true;
    for(auto prefix:{"sound:","body:","demo:","lua:","message:","input:"})
        if(text.starts_with(prefix))return false;
    return true;
}
void LogSetEnabled(bool enabled);
void LogSetFile(const std::filesystem::path& path, bool append = false);
// The main loop's frame; with PT_LOG_TICKS=1 every line carries it ("#2444" after the level), for timing events against captures
void LogSetTick(uint64_t tick);

template <typename... Args>
void LogDebug(std::format_string<Args...> fmt, Args&&... args) {
    if (!LogAccept(LogLevel::Debug,fmt.get())) return;
    LogWrite(LogLevel::Debug, std::format(fmt, std::forward<Args>(args)...));
}

template <typename... Args>
void LogInfo(std::format_string<Args...> fmt, Args&&... args) {
    if (!LogAccept(LogLevel::Info,fmt.get())) return;
    LogWrite(LogLevel::Info, std::format(fmt, std::forward<Args>(args)...));
}

template <typename... Args>
void LogWarn(std::format_string<Args...> fmt, Args&&... args) {
    if (!LogEnabled()) return;
    LogWrite(LogLevel::Warn, std::format(fmt, std::forward<Args>(args)...));
}

template <typename... Args>
void LogError(std::format_string<Args...> fmt, Args&&... args) {
    if (!LogEnabled()) return;
    LogWrite(LogLevel::Error, std::format(fmt, std::forward<Args>(args)...));
}

}
