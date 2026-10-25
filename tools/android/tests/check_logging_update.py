#!/usr/bin/env python3
"""Run the actual Java logging gate with file, preference and native boundary stubs."""
from pathlib import Path
import os, subprocess, tempfile, shutil

repo=Path(__file__).resolve().parents[3]
java_home=Path(os.environ.get('JAVA_HOME',str(repo.parent/'toolchain/host/usr/lib/jvm/java-17-openjdk-amd64')))
stubs={
'android/content/SharedPreferences.java':'''package android.content;import java.util.*;public class SharedPreferences{public Map<String,Boolean> data=new HashMap<>();public boolean fail;public boolean getBoolean(String key,boolean fallback){return data.getOrDefault(key,fallback);}public Editor edit(){return new Editor();}public class Editor{String key;boolean value;public Editor putBoolean(String k,boolean v){key=k;value=v;return this;}public boolean commit(){if(fail)return false;data.put(key,value);return true;}}}''',
'android/content/Context.java':'''package android.content;import java.io.File;public class Context{public File dir;public SharedPreferences prefs=new SharedPreferences();public Context(File d){dir=d;}public Context getApplicationContext(){return this;}public SharedPreferences getSharedPreferences(String key,int mode){return prefs;}public String getPackageName(){return "com.ptport.app.test";}public File getFilesDir(){return dir;}public File getExternalFilesDir(String ignored){return dir;}}''',
'android/util/Log.java':'''package android.util;public class Log{public static int calls;public static int v(String t,String s){return ++calls;}public static int v(String t,String s,Throwable e){return ++calls;}public static int d(String t,String s){return ++calls;}public static int d(String t,String s,Throwable e){return ++calls;}public static int i(String t,String s){return ++calls;}public static int i(String t,String s,Throwable e){return ++calls;}public static int w(String t,String s){return ++calls;}public static int w(String t,String s,Throwable e){return ++calls;}public static int e(String t,String s){return ++calls;}public static int e(String t,String s,Throwable e){return ++calls;}public static String getStackTraceString(Throwable e){calls++;return "trace";}}''',
'com/ptport/app/PTApplication.java':'''package com.ptport.app;public class PTApplication{static int calls;static boolean enabled;static void nativeConfigureLogging(boolean v){calls++;enabled=v;}}''',
'com/ptport/app/PTActivity.java':'''package com.ptport.app;public class PTActivity{static int calls;static boolean running,enabled;static boolean gameRunning(){return running;}static void nativeSetLoggingEnabled(boolean v){calls++;enabled=v;}}''',
'com/ptport/app/CrashMetadata.java':'''package com.ptport.app;import android.content.Context;final class CrashMetadata{static String report(Context c){return "build source-fixture23\\ntimestamp_utc 2026-10-24T00:00:00Z\\n";}}''',
'com/ptport/app/LogTest.java':r'''package com.ptport.app;
import java.io.*;import java.nio.file.*;import java.util.concurrent.*;import android.content.*;import android.util.Log;
public class LogTest {
 static int checks,crashDelegations;static void ok(boolean b,String why){if(!b)throw new AssertionError(why);checks++;}
 static void all(){Throwable e=new Exception("test");AppLog.v("pt","test");AppLog.v("pt","test",e);AppLog.d("pt","test");AppLog.d("pt","test",e);AppLog.i("pt","test");AppLog.i("pt","test",e);AppLog.w("pt","test");AppLog.w("pt","test",e);AppLog.e("pt","test");AppLog.e("pt","test",e);AppLog.getStackTraceString(e);}
 public static void main(String[] args)throws Exception {
  Path root=Files.createTempDirectory("pt-log-test");Context ctx=new Context(root.toFile());
  Thread.setDefaultUncaughtExceptionHandler((t,e)->crashDelegations++);AppLog.initialize(ctx);
  ok(!AppLog.enabled() && !PTApplication.enabled,"default Off configured before game");all();AppLog.append(ctx,"driver-import.log","hidden");
  ok(Log.calls==0 && !Files.exists(root.resolve("data")),"Off suppresses all levels and importer file");
  Thread.getDefaultUncaughtExceptionHandler().uncaughtException(Thread.currentThread(),new Exception("Off crash"));ok(crashDelegations==1,"Off still delegates Android crash handling");
  AppLog.setEnabled(true);ok(ctx.prefs.getBoolean("logs_enabled",false) && PTApplication.enabled && PTActivity.calls==0,"On persists without requiring a game");
  all();ok(Log.calls==7,"Normal passes info/warning/error and skips debug/verbose");ctx.prefs.edit().putBoolean("verbose_logs",true).commit();AppLog.setEnabled(true);Log.calls=0;all();ok(Log.calls==11,"Verbose passes all Java log levels");AppLog.append(ctx,"driver-import.log","visible");Path file=root.resolve("data/driver-import.log");ok(Files.readString(file).contains("visible"),"On writes importer diagnostics");
  Thread.getDefaultUncaughtExceptionHandler().uncaughtException(Thread.currentThread(),new Exception("On crash"));
  Path crash=root.resolve("data/pt-java-crash.txt");String report=Files.readString(crash);ok(crashDelegations==2 && report.contains("On crash"),"enabled crash falls back to the app data folder and still delegates Android handling");ok(report.contains("build source-fixture23")&&report.contains("timestamp_utc"),"Java crash includes identity and timestamp");Files.delete(crash);
  PTActivity.running=true;AppLog.setEnabled(false);ok(!PTActivity.enabled && PTActivity.calls==1 && !PTApplication.enabled,"Off reaches live native game and process logger");
  long bytes=Files.size(file);int calls=Log.calls;all();AppLog.append(ctx,"driver-import.log","after Off");ok(Files.size(file)==bytes && Log.calls==calls,"Off stops further file and logcat writes");
  ctx.prefs.fail=true;try{AppLog.setEnabled(true);throw new AssertionError("expected preference failure");}catch(IOException expected){}ok(!AppLog.enabled() && !PTApplication.enabled,"failed preference commit does not change gate");ctx.prefs.fail=false;
  AppLog.setEnabled(true);AppLog.append(ctx,"driver-import.log","reopened");ok(Files.readString(file).contains("visible") && Files.readString(file).contains("reopened"),"re-enabling preserves prior report");
  ExecutorService workers=Executors.newFixedThreadPool(4);for(int i=0;i<200;i++)workers.submit(()->AppLog.append(ctx,"driver-import.log","racing writer"));AppLog.setEnabled(false);workers.shutdown();workers.awaitTermination(10,TimeUnit.SECONDS);bytes=Files.size(file);AppLog.append(ctx,"driver-import.log","late writer");ok(Files.size(file)==bytes,"serialized writes cannot restart after Off");
  ok(!ctx.prefs.getBoolean("logs_enabled",true),"Off persists for next launch");
  Files.delete(file);Files.delete(file.getParent());Files.delete(root);System.out.println("Java logging checks passed: "+checks);
 }
}'''
}
with tempfile.TemporaryDirectory() as tmp:
    t=Path(tmp)
    for name,text in stubs.items():
        p=t/name;p.parent.mkdir(parents=True,exist_ok=True);p.write_text(text)
    shutil.copy2(repo/'android/app/src/main/java/com/ptport/app/AppLog.java',t/'com/ptport/app/AppLog.java')
    subprocess.run([str(java_home/'bin/javac'),'-d',str(t/'classes')]+[str(p)for p in t.rglob('*.java')],check=True,timeout=30)
    subprocess.run([str(java_home/'bin/java'),'-cp',str(t/'classes'),'com.ptport.app.LogTest'],check=True,timeout=15)
