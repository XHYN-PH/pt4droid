#!/usr/bin/env python3
"""Run real crash-file retention and ZIP export methods against filesystem fixtures."""
from pathlib import Path
import os
import resource
import shlex
import subprocess
import tempfile

repo = Path(__file__).resolve().parents[3]
java = Path(os.environ.get("JAVA_HOME", "/usr/lib/jvm/java-17-openjdk-amd64")) / "bin"
resource.setrlimit(resource.RLIMIT_CORE,(0,0))


def extract(path, signature):
    source = (repo / path).read_text()
    start = source.index(signature)
    end = source.index("{", start) + 1
    depth = 1
    while depth:
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    return source[start:end]


native = r'''
#include <cassert>
#include <filesystem>
#include <fstream>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <fcntl.h>
#include <signal.h>
#include <dlfcn.h>
#include <unwind.h>
#include <sys/wait.h>
#include <unistd.h>
int g_crash_fd=-1;struct sigaction g_previous[32];bool enabled=true;
char g_crash_identity[192]="unknown\n";
bool LogEnabled(){return enabled;}
constexpr int ANDROID_LOG_FATAL=7;int __android_log_write(int,const char*,const char*){return 0;}
/* CRASH_METHODS */
/* INSTALL */
std::string text(const std::filesystem::path& path){std::ifstream in(path);return {std::istreambuf_iterator<char>(in),{}};}
void closeLog(){if(g_crash_fd>=0)close(g_crash_fd);g_crash_fd=-1;}
int main(int argc,char**argv){
 assert(argc==2);const auto path=std::filesystem::path(argv[1])/"pt-crash.txt";
 enabled=false;InstallCrashLog(path);assert(g_crash_fd==-1&&!std::filesystem::exists(path));enabled=true;
 std::ofstream(path)<<"failed first launch\n";InstallCrashLog(path);assert(g_crash_fd>=0);
 assert(text(path)=="failed first launch\n");assert(fcntl(g_crash_fd,F_GETFD)&FD_CLOEXEC);
 const int first=g_crash_fd;InstallCrashLog(path);assert(g_crash_fd==first);
 closeLog();InstallCrashLog(path);closeLog();assert(text(path)=="failed first launch\n");
 InstallCrashLog(path);assert(write(g_crash_fd,"second failure\n",15)==15);closeLog();
 assert(text(path)=="failed first launch\nsecond failure\n");
 std::ofstream(path)<<std::string(128*1024,'x');InstallCrashLog(path);closeLog();
 auto previous=path;previous+=".previous";assert(text(previous)==std::string(128*1024,'x'));assert(text(path).empty());
 InstallCrashLog(path);closeLog();assert(text(previous)==std::string(128*1024,'x'));
 std::ofstream(path)<<std::string(128*1024,'y');InstallCrashLog(path);closeLog();assert(text(previous)==std::string(128*1024,'y'));
 const auto crash=std::filesystem::path(argv[1])/"metadata-crash.txt";
 const pid_t child=fork();assert(child>=0);
 if(child==0){signal(SIGABRT,SIG_DFL);SetCrashIdentity("source-fixture23","0.2.1-test.23");InstallCrashLog(crash);raise(SIGABRT);_exit(99);}
 int status=0;assert(waitpid(child,&status,0)==child);assert(WIFSIGNALED(status)&&WTERMSIG(status)==SIGABRT);
 const auto report=text(crash);assert(report.find("pt4droid 0.2.1-test.23\nbuild source-fixture23\n")!=std::string::npos);
 const auto at=report.find("timestamp_unix_utc ");assert(at!=std::string::npos);
 const auto seconds=std::stoll(report.substr(at+19));assert(std::llabs(static_cast<long long>(time(nullptr))-seconds)<30);
 assert(report.find("P.T. crashed: signal 6")!=std::string::npos);
 puts("Native crash reports: survive successful retries, append failures, bound history, retain previous reports, honor logs Off");
 puts("Native crash metadata: real SIGABRT report preserves original signal termination and includes version/build/current UTC time");
}
'''.replace("/* INSTALL */", extract("src/engine/platform/android.cpp", "void InstallCrashLog("))
crash_methods=[extract("src/engine/platform/android.cpp",signature) for signature in ["void WriteText(","struct Frames", "_Unwind_Reason_Code CollectFrame(","void OnCrash(","void SetCrashIdentity("]]
crash_methods[1]+=";"
native=native.replace("/* CRASH_METHODS */","\n".join(crash_methods))

export = r'''
import java.io.*;import java.nio.file.*;import java.util.*;import java.util.zip.*;import android.content.Context;
public class CrashExportTest {
 /* METHODS */
 static void ok(boolean value){if(!value)throw new AssertionError();}
 static void write(File folder,String name,String contents)throws Exception{folder.mkdirs();Files.writeString(new File(folder,name).toPath(),contents);}
 public static void main(String[]args)throws Exception{
  Path root=Path.of(args[0]);Context c=new Context(root);File internal=new File(c.getFilesDir(),"data"),external=new File(c.getExternalFilesDir(null),"data");
  write(internal,"pt-crash.txt","native stack");write(internal,"pt-crash.txt.previous","older stack");
  write(internal,"pt-java-crash.txt","java internal");write(external,"pt-java-crash.txt","java external");
  write(c.getFilesDir(),"pt-java-crash.txt","legacy internal");write(c.getExternalFilesDir(null),"pt-java-crash.txt","legacy external");
  write(external,"driver-import.log","driver import");write(external,"driver-import.log.previous","previous import");
  write(external,"pt.log","x".repeat(2*1024*1024+32));
  for(String name:new String[]{"game_graphics_applying.ini","performance_profile_pending.txt","driver_startup_failed.txt"})write(c.getFilesDir(),name,"recover "+name);
  Path file=root.resolve("report.zip");try(ZipOutputStream zip=new ZipOutputStream(Files.newOutputStream(file))){writeLogs(zip,c);writeSettings(zip,c);}
  try(ZipFile zip=new ZipFile(file.toFile())){
   Map<String,String> expected=Map.of("logs/1-pt-crash.txt","native stack","logs/1-pt-crash.txt.previous","older stack",
    "logs/1-pt-java-crash.txt","java internal","logs/2-pt-java-crash.txt","java external",
    "logs/3-pt-java-crash.txt","legacy internal","logs/4-pt-java-crash.txt","legacy external",
    "logs/2-driver-import.log","driver import","logs/2-driver-import.log.previous","previous import");
   for(var e:expected.entrySet()){var entry=zip.getEntry(e.getKey());ok(entry!=null);ok(new String(zip.getInputStream(entry).readAllBytes(),java.nio.charset.StandardCharsets.UTF_8).equals(e.getValue()));}
   ok(zip.getEntry("logs/2-pt.log").getSize()==2*1024*1024);
   for(String name:new String[]{"game_graphics_applying.ini","performance_profile_pending.txt","driver_startup_failed.txt"}){
    var entry=zip.getEntry("settings/"+name);ok(entry!=null);ok(new String(zip.getInputStream(entry).readAllBytes()).equals("recover "+name));
   }
  }
  c.external=null;try(ZipOutputStream zip=new ZipOutputStream(new ByteArrayOutputStream())){writeLogs(zip,c);}
  System.out.println("Crash ZIP export: native current/previous, Java internal/external/legacy roots, actual import names, bounded tails and null external storage passed");
 }
}
'''
methods = [extract("android/app/src/main/java/com/ptport/app/Diagnostics.java", signature)
           for signature in ["private static void writeLogs(", "private static void entry(", "private static void copy("]]
methods.append(extract("android/app/src/main/java/com/ptport/app/Diagnostics.java","private static void writeSettings("))
export = export.replace("/* METHODS */", "\n".join(methods))

with tempfile.TemporaryDirectory() as directory:
    root = Path(directory)
    cpp = root / "crash.cpp"
    cpp.write_text(native)
    compiler = [os.environ.get("CXX", "g++"), "-std=c++20", "-O1"]
    compiler += shlex.split(os.environ.get("PT_TEST_CXX_FLAGS", ""))
    subprocess.run(compiler + [str(cpp), "-ldl", "-o", str(root / "crash")], check=True, timeout=90)
    subprocess.run([str(root / "crash"), str(root)], check=True, timeout=15)
    context = root / "android/content/Context.java"
    context.parent.mkdir(parents=True)
    context.write_text('''package android.content;import java.io.File;import java.nio.file.Path;
public class Context {public File internal,external;public Context(Path p){internal=p.resolve("internal").toFile();external=p.resolve("external").toFile();}
public File getFilesDir(){return internal;}public File getExternalFilesDir(String ignored){return external;}public String getPackageName(){return "com.ptport.app.test";}}''')
    fixture = root / "CrashExportTest.java"
    fixture.write_text(export)
    subprocess.run([str(java / "javac"), "-d", str(root / "classes"), str(context), str(fixture)], check=True, timeout=30)
    subprocess.run([str(java / "java"), "-cp", str(root / "classes"), "CrashExportTest", str(root)], check=True, timeout=15)
