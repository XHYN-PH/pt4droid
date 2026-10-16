#!/usr/bin/env python3
"""Execute actual menu INI model and refresh requests with Android service boundaries."""
from pathlib import Path
import os, subprocess, shutil, tempfile
repo=Path(__file__).resolve().parents[3]
java=Path(os.environ.get('JAVA_HOME',str(repo.parent/'toolchain/host/usr/lib/jvm/java-17-openjdk-amd64')))
stubs={
'android/content/SharedPreferences.java':'''package android.content;import java.util.*;public class SharedPreferences{
 public Map<String,Object> data=new HashMap<>();public boolean failCommit;
 public Map<String,?> getAll(){return new HashMap<>(data);}
 public float getFloat(String k,float v){return data.containsKey(k)?(Float)data.get(k):v;}
 public int getInt(String k,int v){return data.containsKey(k)?(Integer)data.get(k):v;}
 public boolean getBoolean(String k,boolean v){return data.containsKey(k)?(Boolean)data.get(k):v;}
 public Editor edit(){return new Editor();}public class Editor{
  Map<String,Object> pending=new HashMap<>();
  public Editor putFloat(String k,float v){pending.put(k,v);return this;}public Editor putInt(String k,int v){pending.put(k,v);return this;}
  public Editor putBoolean(String k,boolean v){pending.put(k,v);return this;}public Editor putLong(String k,long v){pending.put(k,v);return this;}public Editor putString(String k,String v){pending.put(k,v);return this;}public Editor remove(String k){pending.put(k,null);return this;}
  public boolean commit(){if(failCommit){failCommit=false;return false;}for(Map.Entry<String,Object> e:pending.entrySet())if(e.getValue()==null)data.remove(e.getKey());else data.put(e.getKey(),e.getValue());return true;}
  public void apply(){commit();}
 }}''',
'android/content/Context.java':'''package android.content;import java.io.File;public class Context{public File internal,external;public SharedPreferences prefs=new SharedPreferences();public Context(File i,File e){internal=i;external=e;}public File getFilesDir(){return internal;}public File getExternalFilesDir(String ignored){return external;}public SharedPreferences getSharedPreferences(String k,int mode){return prefs;}}''',
'android/app/Activity.java':'''package android.app;import android.content.Context;import android.view.*;import java.io.File;public class Activity extends Context{public Window window=new Window();public WindowManager manager=new WindowManager();public Activity(File i,File e){super(i,e);}public Window getWindow(){return window;}public WindowManager getWindowManager(){return manager;}}''',
'android/os/SystemClock.java':'''package android.os;public class SystemClock { public static long now=5000;public static long uptimeMillis(){return now;} }''',
'android/os/Build.java':'''package android.os;public class Build{public static class VERSION{public static int SDK_INT=35;}}''',
'android/view/Display.java':'''package android.view;public class Display{public Mode current;public Mode[] modes;public Mode getMode(){return current;}public Mode[] getSupportedModes(){return modes;}public float visibleRate;public float getRefreshRate(){return visibleRate>0?visibleRate:current.rate;}public static class Mode{public int id,w,h;public float rate;public Mode(int i,int w,int h,float r){id=i;this.w=w;this.h=h;rate=r;}public int getModeId(){return id;}public int getPhysicalWidth(){return w;}public int getPhysicalHeight(){return h;}public float getRefreshRate(){return rate;}}}''',
'android/view/Window.java':'''package android.view;public class Window{public int updates;public WindowManager.LayoutParams attributes=new WindowManager.LayoutParams();public WindowManager.LayoutParams getAttributes(){return attributes;}public void setAttributes(WindowManager.LayoutParams a){updates++;attributes=a;}}''',
'android/view/WindowManager.java':'''package android.view;public class WindowManager{public Display display=new Display();public Display getDefaultDisplay(){return display;}public static class LayoutParams{public int preferredDisplayModeId;public float preferredRefreshRate;}}''',
'android/view/Surface.java':'''package android.view;public class Surface{public static final int FRAME_RATE_COMPATIBILITY_DEFAULT=0,CHANGE_FRAME_RATE_ONLY_IF_SEAMLESS=0,CHANGE_FRAME_RATE_ALWAYS=1;public boolean valid=true;public int generation=1,calls;public float rate;public int strategy=-1;public boolean isValid(){return valid;}public int getGenerationId(){return generation;}public void setFrameRate(float rate,int compatibility,int strategy){this.strategy=strategy;setFrameRate(rate,compatibility);}public void setFrameRate(float rate,int compatibility){if(compatibility!=0)throw new AssertionError("game compatibility");calls++;this.rate=rate;}}''',
'com/ptport/app/AppLog.java':'''package com.ptport.app;public class AppLog{public static int errors;public static void w(String t,String text,Throwable e){errors++;}}''',
'com/ptport/app/DriverImportJob.java':'''package com.ptport.app;import java.io.*;import java.nio.file.*;class DriverImportJob{static String failName;static void atomicWrite(File file,byte[] bytes)throws IOException{if(file.getName().equals(failName)){failName=null;throw new IOException("injected write failure");}Path temp=Paths.get(file+".tmp");Files.write(temp,bytes);Files.move(temp,file.toPath(),StandardCopyOption.REPLACE_EXISTING,StandardCopyOption.ATOMIC_MOVE);}}''',
'com/ptport/app/GraphicsMenuTest.java':r'''package com.ptport.app;
import android.app.*;import android.view.*;import android.os.*;import java.nio.file.*;import java.io.*;
public class GraphicsMenuTest {
 static int checks;static void ok(boolean value,String why){if(!value)throw new AssertionError(why);checks++;}
 public static void main(String[] args)throws Exception{
  Path root=Files.createTempDirectory("pt-menu-test"),inside=root.resolve("internal"),outside=root.resolve("external");Files.createDirectories(inside);Files.createDirectories(outside.resolve("data"));Activity a=new Activity(inside.toFile(),outside.toFile());
  Files.writeString(outside.resolve("data/pt.ini"),"[graphics]\nshadow_quality = 3\nmotion_blur = 1\nclarity = 0.65\n[progress]\narchive = keep\n[audio]\nvolume = 1.7\n");
  GameGraphicsData data=GameGraphicsData.load(a,"");ok(data.integer("graphics.shadow_quality",-1)==3 && data.enabled("graphics.motion_blur"),"launcher reads real game settings");
  ok(!data.text().contains("archive") && !data.text().contains("volume"),"graphics overlay cannot overwrite progress or audio");
  Files.writeString(inside.resolve("performance_profile_pending.txt"),"1\n");data=GameGraphicsData.load(a,"");ok(data.integer("graphics.shadow_quality",0)==1 && !data.enabled("graphics.motion_blur") && data.number("graphics.clarity",0)==.65f,"pending Maximum shows actual intended settings without resetting clarity");
  data.put("camera.third_person",1);data.put("graphics.shadow_quality",0);data.put("graphics.film_grain",.37f);data.put("display.fps_limit",120);data.save(a);
  ok(Files.readString(inside.resolve("performance_profile_pending.txt")).trim().equals("0") && a.prefs.getInt("performance_profile",-1)==0,"individual changes replace pending preset and label Custom");
  ok(!Files.readString(inside.resolve("game_graphics_pending.ini")).contains("third_person"),"graphics page cannot overwrite camera-toolbar state");
  data=GameGraphicsData.load(a,"[graphics]\nshadow_quality = 2\n[android_capabilities]\nray_tracing = 1\n");ok(data.integer("graphics.shadow_quality",-1)==0 && data.number("graphics.film_grain",0)==.37f && data.rayTracing,"pending graphics overlays live snapshot and preserves capability");
  Files.delete(inside.resolve("game_graphics_pending.ini"));data=GameGraphicsData.load(a,"[graphics]\nshadow_quality = 1\n[android_capabilities]\nray_tracing = 0\nray_reason = missing Vulkan ray queries\n");ok(data.integer("graphics.shadow_quality",-1)==1 && !data.rayTracing && data.rayReason.contains("missing"),"live game snapshot wins over old file");
  data.preset(3);ok(data.enabled("graphics.motion_blur") && data.integer("graphics.shadow_quality",0)==3 && data.number("graphics.film_grain",0)==1,"Original restored in model");
  Files.writeString(inside.resolve("game_graphics_applying.ini"),"[graphics]\nshadow_quality = 0\n");ok(GameGraphicsData.load(a,"").integer("graphics.shadow_quality",-1)==0,"interrupted pending settings visible before next start");
  data.parse("[graphics]\nfilm_grain = nan\n");ok(data.number("graphics.film_grain",.5f)==.5f,"corrupt float cannot enter slider");
  Display d=a.manager.display;Display.Mode low=new Display.Mode(1,1080,2400,60),high=new Display.Mode(2,1080,2400,120),other=new Display.Mode(3,2160,3840,144);d.current=low;d.modes=new Display.Mode[]{low,high,other};
  ok(DisplayRefresh.choose(d.modes,low,0)==high,"automatic picks highest at same resolution");ok(DisplayRefresh.choose(d.modes,low,60)==low,"explicit 60 mode");ok(DisplayRefresh.choose(d.modes,low,999)==high,"unsupported request picks nearest available mode");ok(DisplayRefresh.choose(new Display.Mode[]{},low,120)==low,"missing mode inventory retains valid current mode");
  ok(DisplayRefresh.rates(a).size()==2 && DisplayRefresh.maximum(a)==120,"UI only exposes same-resolution supported rates");
  Surface s=new Surface();DisplayRefresh.apply(a,s);ok(s.calls==1 && s.rate==120 && a.window.attributes.preferredRefreshRate==120 && a.window.attributes.preferredDisplayModeId==2,"Android 11+ requests high rate on window and game surface");
  ok(s.strategy==Surface.CHANGE_FRAME_RATE_ALWAYS,"explicit game-rate vote permits a non-seamless switch");
  ok(DisplayRefresh.actualRate()==60,"reports the observed display rate before the requested mode takes effect");
  d.current=high;DisplayRefresh.apply(a,s);ok(DisplayRefresh.actualRate()==120,"reports display changes rather than only the requested rate");
  int updates=a.window.updates;for(int i=0;i<1000;i++)DisplayRefresh.apply(a,s);ok(s.calls==1 && updates==a.window.updates,"stable request avoids repeated surface/window updates");
  a.prefs.edit().putInt("frame_target",60).apply();DisplayRefresh.apply(a,s);ok(s.calls==2 && s.rate==60,"live preference changes request");DisplayRefresh.surfaceCreated(a,s);ok(s.calls==3,"recreated surface receives request");
  Build.VERSION.SDK_INT=29;a.prefs.edit().putInt("frame_target",0).apply();DisplayRefresh.apply(a,s);ok(s.calls==3 && a.window.attributes.preferredDisplayModeId==2,"Android 10 uses supported window mode without newer Surface API");
  Build.VERSION.SDK_INT=35;a.prefs.edit().putInt("frame_target",-10).apply();DisplayRefresh.apply(a,s);ok(s.rate==120 && AppLog.errors==0,"corrupt preference safely uses automatic");
  ok(DisplayRefresh.choose(d.modes,low,30)==low,"30 FPS uses 60 Hz multiple");
  ok(DisplayRefresh.choose(d.modes,low,40)==high,"40 FPS uses 120 Hz multiple rather than incompatible 60 Hz");
  DisplayRefresh.setTarget(40);DisplayRefresh.apply(a,s);ok(s.rate==40&&a.window.attributes.preferredRefreshRate==120,"one runtime cap drives surface content rate and compatible display mode");
  DisplayRefresh.setTarget(60);DisplayRefresh.apply(a,s);ok(s.rate==60&&a.window.attributes.preferredRefreshRate==60,"live cap selects exact mode");
  DisplayRefresh.setTarget(0);DisplayRefresh.apply(a,s);ok(s.rate==120,"unlimited restores highest display rate");
  for(int fps:new int[]{0,20,24,25,30,40,45,48,50,60,72,90,120,144,165,240}){
   DisplayRefresh.setTarget(fps);DisplayRefresh.invalidate();DisplayRefresh.apply(a,s);
   ok(s.rate==(fps==0?120:fps),"every supported/unsupported target preserves selected content FPS");
  }
  DisplayRefresh.setTarget(60);d.current=high;DisplayRefresh.invalidate();
  int requests=s.calls;
  for(int i=0;i<20;i++){android.os.SystemClock.now+=2500;DisplayRefresh.apply(a,s);}
  ok(s.calls-requests==3,"ignored display requests have a bounded recovery count");
  DisplayRefresh.invalidate();DisplayRefresh.apply(a,s);ok(s.calls-requests==4,"focus/surface invalidation permits recovery again");
  Display.Mode middle=new Display.Mode(4,1080,2400,90);d.modes=new Display.Mode[]{low,middle,high};d.current=middle;
  DisplayRefresh.setTarget(120);DisplayRefresh.invalidate();requests=s.calls;
  for(int i=0;i<20;i++){android.os.SystemClock.now+=2500;DisplayRefresh.apply(a,s);}
  ok(s.calls-requests==3,"120 on a 90 Hz override retries within the bound");
  d.current=low;android.os.SystemClock.now+=2500;DisplayRefresh.apply(a,s);
  ok(s.calls-requests==4 && s.rate==120,"a new display override rearms recovery without changing cap");
  d.current=high;d.visibleRate=60;DisplayRefresh.setTarget(120);DisplayRefresh.invalidate();requests=s.calls;
  for(int i=0;i<20;i++){android.os.SystemClock.now+=2500;DisplayRefresh.apply(a,s);}
  ok(s.calls-requests==3,"app-visible 60 Hz override recovers even with a physical 120 Hz mode");
  d.visibleRate=120;DisplayRefresh.apply(a,s);requests=s.calls;DisplayRefresh.surfaceCreated(a,s);
  ok(s.calls==requests+1,"surface replacement/resize callback reissues the cached rate vote");
  System.out.println("Graphics menu/refresh checks passed: "+checks);
 }
}'''
}
def execute(stubs,main_class):
 with tempfile.TemporaryDirectory() as tmp:
     t=Path(tmp)
     for name,text in stubs.items():
         p=t/name;p.parent.mkdir(parents=True,exist_ok=True);p.write_text(text)
     for name in ['GameGraphicsData.java','DisplayRefresh.java','SettingsFiles.java']:shutil.copy2(repo/'android/app/src/main/java/com/ptport/app'/name,t/'com/ptport/app'/name)
     subprocess.run([str(java/'bin/javac'),'-d',str(t/'classes')]+[str(p)for p in t.rglob('*.java')],check=True,timeout=30)
     subprocess.run([str(java/'bin/java'),'-cp',str(t/'classes'),main_class],check=True,timeout=15)

if __name__=="__main__":execute(stubs,"com.ptport.app.GraphicsMenuTest")
