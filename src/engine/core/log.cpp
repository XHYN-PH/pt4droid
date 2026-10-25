#include "engine/core/log.h"
#include "engine/platform/os.h"

#include <chrono>
#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <mutex>
#include <ctime>

#ifdef __ANDROID__
#include <android/log.h>
#endif

namespace pt {
namespace {

std::mutex g_log_mutex;
std::atomic<bool> g_enabled{[] {const char* value=std::getenv("PT_LOGS");return !value || std::string_view(value)!="0";}()};
FILE* g_log_file = nullptr;
std::filesystem::path g_log_path;
uint64_t g_log_bytes=0;
constexpr uint64_t kLogLimit=8ull*1024*1024;
void SessionHeader(bool continued){
    if(!g_log_file)return;
    const int written=std::fprintf(g_log_file,"\n[session] timestamp_unix_utc %lld; %s; verbosity %s\n",
        static_cast<long long>(std::time(nullptr)),continued?"continued after rotation":"opened",
        g_log_verbose.load(std::memory_order_relaxed)?"verbose":"normal");
    if(written>0)g_log_bytes+=uint64_t(written);
}
void RotateLog(){
    if(g_log_file)std::fclose(g_log_file);
    std::error_code ec;auto previous=g_log_path;previous+=".previous";std::filesystem::remove(previous,ec);ec.clear();std::filesystem::rename(g_log_path,previous,ec);
    g_log_file=os::OpenFile(g_log_path,"w");g_log_bytes=0;SessionHeader(true);
}
std::chrono::steady_clock::time_point g_last_flush{};
uint64_t g_tick = 0;
const bool g_log_ticks = [] {
    const char* value = std::getenv("PT_LOG_TICKS");
    return value && *value == '1';
}();

const char* LevelTag(LogLevel level) {
    switch (level) {
    case LogLevel::Debug: return "debug";
    case LogLevel::Info: return "info";
    case LogLevel::Warn: return "warn";
    case LogLevel::Error: return "error";
    }
    return "?";
}

}

bool LogEnabled() {return g_enabled.load(std::memory_order_relaxed);}

void LogSetEnabled(bool enabled) {
    std::lock_guard lock(g_log_mutex);
    g_enabled.store(enabled,std::memory_order_relaxed);
    if(!enabled && g_log_file){std::fclose(g_log_file);g_log_file=nullptr;}
}

void LogSetFile(const std::filesystem::path& path, bool append) {
    std::lock_guard lock(g_log_mutex);
    if (g_log_file) {
        std::fclose(g_log_file);
        g_log_file=nullptr;
    }
    g_log_path=path;std::error_code size_error;g_log_bytes=append?std::filesystem::file_size(path,size_error):0;if(size_error)g_log_bytes=0;
    if(LogEnabled()){if(g_log_bytes>=kLogLimit)RotateLog();else {g_log_file = os::OpenFile(path, append ? "a" : "w");SessionHeader(false);}}
    g_last_flush=std::chrono::steady_clock::now();
}

void LogWrite(LogLevel level, std::string_view text) {
    if(!LogAccept(level,text))return;
    text=text.substr(0,16*1024);
    using namespace std::chrono;
    static const auto start = steady_clock::now();
    const double seconds = duration<double>(steady_clock::now() - start).count();
    std::lock_guard lock(g_log_mutex);
    if(!LogEnabled())return;
    char tick[32] = "";
    if (g_log_ticks) {
        std::snprintf(tick, sizeof(tick), "#%llu ", static_cast<unsigned long long>(g_tick));
    }
#ifdef __ANDROID__
    // stderr goes nowhere on Android: logcat (adb logcat -s pt) instead
    const int priority = level == LogLevel::Error ? ANDROID_LOG_ERROR : level == LogLevel::Warn ? ANDROID_LOG_WARN
                       : level == LogLevel::Debug ? ANDROID_LOG_DEBUG : ANDROID_LOG_INFO;
    __android_log_print(priority, "pt", "%s%.*s", tick, static_cast<int>(text.size()), text.data());
#else
    std::fprintf(stderr, "[%9.3f] %-5s %s%.*s\n", seconds, LevelTag(level), tick, static_cast<int>(text.size()), text.data());
#endif
    if (g_log_file) {
        if(g_log_bytes+text.size()+80>kLogLimit)RotateLog();
        if(!g_log_file)return;
        const int written=std::fprintf(g_log_file, "[%9.3f] %-5s %s%.*s\n", seconds, LevelTag(level), tick, static_cast<int>(text.size()), text.data());
        if(written>0)g_log_bytes+=static_cast<uint64_t>(written);
#ifdef __ANDROID__
        const auto now=steady_clock::now();
        if(level==LogLevel::Warn || level==LogLevel::Error || now-g_last_flush>=std::chrono::seconds(1)) {
            std::fflush(g_log_file);g_last_flush=now;
        }
#else
        std::fflush(g_log_file);
#endif
    }
}

void LogSetTick(uint64_t tick) {
    if(!LogEnabled() || !g_log_ticks)return;
    std::lock_guard lock(g_log_mutex);
    g_tick = tick;
}

}
