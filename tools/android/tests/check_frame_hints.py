#!/usr/bin/env python3
# Exercise the real ADPF boundary on old Android, lifecycle, TID changes and failures.
from pathlib import Path
import os,subprocess,tempfile,shutil
repo=Path(__file__).resolve().parents[3]
stubs={
'android/annotation/TargetApi.java':'package android.annotation;public @interface TargetApi{int value();}',
'android/os/Build.java':'package android.os;public class Build{public static class VERSION{public static int SDK_INT=29;}}',
'android/os/Process.java':'package android.os;public class Process{public static int tid=7;public static int myTid(){return tid;}}',
'android/content/Context.java':'''package android.content;public class Context{public android.os.PerformanceHintManager manager=new android.os.PerformanceHintManager();public <T>T getSystemService(Class<T> cls){return cls.cast(manager);}}''',
'android/os/PerformanceHintManager.java':'''package android.os;public class PerformanceHintManager{public static int creates=0,closes=0,reports=0,updates=0;public static boolean fail=false;public static long duration=0,target=0;public Session createHintSession(int[] threads,long t){if(fail)throw new IllegalStateException();creates++;target=t;return new Session();}public static class Session{public void close(){closes++;}public void updateTargetWorkDuration(long t){updates++;target=t;}public void reportActualWorkDuration(long w){reports++;duration=w;}}}''',
'com/ptport/app/HintTest.java':'''package com.ptport.app;import android.os.Build;import android.os.PerformanceHintManager;import android.content.Context;public class HintTest{static void ok(boolean b){if(!b)throw new AssertionError();}public static void main(String[] args){Context c=new Context();FrameHints.resume();FrameHints.report(c,1000,16_666_667,true);ok(PerformanceHintManager.creates==0);Build.VERSION.SDK_INT=31;FrameHints.report(c,1000,16_666_667,true);ok(PerformanceHintManager.creates==1&&PerformanceHintManager.reports==1);FrameHints.report(c,2000,33_333_333,true);ok(PerformanceHintManager.updates==1&&PerformanceHintManager.duration==2000);FrameHints.pause();FrameHints.report(c,1000,16_666_667,true);ok(PerformanceHintManager.closes==1&&PerformanceHintManager.reports==2);FrameHints.resume();FrameHints.report(c,1000,16_666_667,true);ok(PerformanceHintManager.creates==2);android.os.Process.tid=8;FrameHints.report(c,1000,16_666_667,true);ok(PerformanceHintManager.creates==3&&PerformanceHintManager.closes==2);FrameHints.report(c,1000,16_666_667,false);ok(PerformanceHintManager.closes==3);PerformanceHintManager.fail=true;FrameHints.report(c,1000,16_666_667,true);PerformanceHintManager.fail=false;FrameHints.report(c,1000,16_666_667,true);ok(PerformanceHintManager.creates==3);FrameHints.pause();FrameHints.resume();FrameHints.report(c,1000,16_666_667,true);ok(PerformanceHintManager.creates==4);FrameHints.pause();System.out.println("ADPF boundary passed: old API, pause/resume, target updates, render TID changes, disabled mode and recoverable service failure");}}'''
}
with tempfile.TemporaryDirectory()as folder:
 root=Path(folder)
 for name,text in stubs.items():p=root/name;p.parent.mkdir(parents=True,exist_ok=True);p.write_text(text)
 shutil.copyfile(repo/'android/app/src/main/java/com/ptport/app/FrameHints.java',root/'com/ptport/app/FrameHints.java')
 java=Path(os.environ['JAVA_HOME'])/'bin'
 subprocess.run([str(java/'javac'),'-d',str(root/'classes')]+[str(p)for p in root.rglob('*.java')],check=True,timeout=30)
 subprocess.run([str(java/'java'),'-cp',str(root/'classes'),'com.ptport.app.HintTest'],check=True,timeout=20)
