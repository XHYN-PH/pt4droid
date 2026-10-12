#ifdef __ANDROID__
#include <android/log.h>
#include <cstdarg>
#include "engine/core/log.h"
// Static third-party code (including Swappy) respects the same app logging switch.
extern "C" int __real___android_log_vprint(int,const char*,const char*,va_list);
extern "C" int __real___android_log_write(int,const char*,const char*);
extern "C" int __wrap___android_log_vprint(int priority,const char* tag,const char* format,va_list args){
    return pt::LogEnabled()?__real___android_log_vprint(priority,tag,format,args):0;
}
extern "C" int __wrap___android_log_print(int priority,const char* tag,const char* format,...){
    if(!pt::LogEnabled())return 0;
    va_list args;va_start(args,format);int result=__real___android_log_vprint(priority,tag,format,args);va_end(args);return result;
}
extern "C" int __wrap___android_log_write(int priority,const char* tag,const char* text){
    return pt::LogEnabled()?__real___android_log_write(priority,tag,text):0;
}
#endif
