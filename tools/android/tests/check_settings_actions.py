#!/usr/bin/env python3
"""Execute the production page Apply body and immediate-failure rollback."""
from pathlib import Path
from check_graphics_menu_update import stubs, execute, repo
source=repo/'android/app/src/main/java/com/ptport/app'
settings=(source/'SettingsPages.java').read_text()
start=settings.index('    private void apply(')
method=settings[start:settings.rindex('\n}')]
test=r'''package com.ptport.app;
import android.app.Activity;import android.content.SharedPreferences;import android.view.Display;
import java.io.*;import java.nio.file.*;import java.nio.charset.StandardCharsets;import java.util.*;
public class SettingsActionsTest {
 final Activity activity;final SharedPreferences prefs;TouchControls touch;final Map<String,Object> values=new LinkedHashMap<>();
 SettingsActionsTest(Path root)throws Exception{activity=new Activity(root.toFile(),root.resolve("external").toFile());prefs=activity.prefs;Display.Mode mode=new Display.Mode(1,1080,2400,120);activity.manager.display.current=mode;activity.manager.display.modes=new Display.Mode[]{mode};}
 METHOD
 static void ok(boolean v,String message){if(!v)throw new AssertionError(message);}
 void controls(){values.clear();values.put("invert_y",true);values.put("mouse_sensitivity",1.25f);values.put("controller_sensitivity",1.5f);values.put("overlay_mode",0);values.put("buttons_timeout_ms",8000);}
 void advanced(int profile){values.clear();values.put("performance_profile",profile);values.put("optimized_bloom",true);values.put("mali_optimizations",true);values.put("mali_safe_lighting",true);values.put("compat_texture_index",2);values.put("frame_pacing",false);values.put("sgsr",false);values.put("sharpness",1f);values.put("ao_quality",0);values.put("reflection_quality",0);values.put("sustained",false);values.put("logs_enabled",false);}
 public static void main(String[] args)throws Exception{
  Path root=Files.createTempDirectory("pt-pages");SettingsActionsTest t=new SettingsActionsTest(root);
  for(int preset=0;preset<4;preset++)for(boolean advanced:new boolean[]{false,true})for(int cap:new int[]{0,30,40,45,60,90,120,144,165,240}){
   Files.writeString(root.resolve("game_graphics_pending.ini"),"keep-fsr-and-cap="+cap);Files.writeString(root.resolve("game_graphics_applying.ini"),"applying");Files.deleteIfExists(root.resolve("performance_profile_pending.txt"));
   Files.writeString(root.resolve("mobile_performance.ini"),"unknown_future_option=17\ninvert_y=0\nmali_optimizations=0\n");Files.writeString(root.resolve("render_scale.txt"),"0.8");
   PTActivity.cap=cap;PTActivity.presetCalls=0;if(advanced)t.advanced(preset);else t.controls();t.apply(advanced,preset,.8f);
   ok(PTActivity.cap==cap&&PTActivity.presetCalls==0,"unchanged preset preserves every cap");
   ok(Files.readString(root.resolve("game_graphics_pending.ini")).equals("keep-fsr-and-cap="+cap),"unrelated apply preserves pending FSR and cap");
   ok(Files.readString(root.resolve("game_graphics_applying.ini")).equals("applying"),"unrelated apply preserves applying overlay");
   ok(!Files.exists(root.resolve("performance_profile_pending.txt")),"unrelated apply does not queue preset");
   String ini=Files.readString(root.resolve("mobile_performance.ini"));ok(ini.contains("unknown_future_option=17"),"preserve future INI keys");
   ok(ini.contains(advanced?"invert_y=0":"mali_optimizations=0"),"preserve other page's INI keys");
   if(!advanced)ok(Files.readString(root.resolve("render_scale.txt")).equals("0.8"),"Controls never writes render scale");
  }
  for(int output=0;output<3;++output)for(int halo=0;halo<3;++halo){t.advanced(0);t.values.put("output_scale_index",output);t.values.put("flare_halo_index",halo);int cap=PTActivity.cap;t.apply(true,0,.8f);String ini=Files.readString(root.resolve("mobile_performance.ini"));ok(ini.contains("output_scale="+new String[]{"1.0","0.9","0.8"}[output])&&ini.contains("flare_halo="+new String[]{"0.0","0.2","1.0"}[halo]),"output and halo choices reach native configuration");ok(PTActivity.cap==cap,"output/halo choice preserves frame cap");}
  for(boolean enabled:new boolean[]{false,true}){t.advanced(0);t.values.put("lens_flares",enabled);int previousCap=PTActivity.cap;t.apply(true,0,.8f);ok(PTActivity.cap==previousCap,"glare control preserves FPS cap");ok(Files.readString(root.resolve("mobile_performance.ini")).contains("lens_flares="+(enabled?"1":"0")),"main flare control persists through actual Apply");ok(t.prefs.getBoolean("lens_flares",!enabled)==enabled,"main flare preference matches native config");}
  t.advanced(0);t.values.put("dynamic_resolution",true);t.values.put("temporal_upscaling",false);String unchanged=Files.readString(root.resolve("mobile_performance.ini"));
  try{t.apply(true,0,.8f);throw new AssertionError("inconsistent temporal state must fail");}catch(IOException expected){}
  ok(Files.readString(root.resolve("mobile_performance.ini")).equals(unchanged),"invalid dependency never silently re-enables SGSR2 or writes settings");
  t.values.put("temporal_upscaling",true);t.apply(true,0,.8f);ok(Files.readString(root.resolve("mobile_performance.ini")).contains("temporal_upscaling=1"),"consistent dynamic dependency persists");
  t.advanced(1);t.apply(true,2,.8f);ok(PTActivity.cap==0&&PTActivity.presetCalls==1,"explicit preset applies once");
  ok(!Files.exists(root.resolve("game_graphics_pending.ini"))&&Files.readString(root.resolve("performance_profile_pending.txt")).trim().equals("1"),"explicit preset replaces old overlay");
  // Failure on the second write restores the already written first file and preferences.
  Path first=root.resolve("first"),second=root.resolve("second");Files.writeString(first,"before-a");Files.writeString(second,"before-b");
  t.prefs.edit().putInt("existing",7).commit();Map<File,byte[]> changes=new LinkedHashMap<>();changes.put(first.toFile(),SettingsFiles.text("after-a"));changes.put(second.toFile(),SettingsFiles.text("after-b"));Map<String,Object> prefs=new LinkedHashMap<>();prefs.put("existing",8);prefs.put("new-key",true);
  DriverImportJob.failName="second";try{SettingsFiles.commit(changes,t.prefs,prefs);throw new AssertionError("expected failed write");}catch(IOException expected){}
  ok(Files.readString(first).equals("before-a")&&Files.readString(second).equals("before-b")&&t.prefs.getInt("existing",0)==7&&!t.prefs.getAll().containsKey("new-key"),"write failure rollback");
  t.prefs.failCommit=true;try{SettingsFiles.commit(changes,t.prefs,prefs);throw new AssertionError("expected failed preferences");}catch(IOException expected){}
  ok(Files.readString(first).equals("before-a")&&Files.readString(second).equals("before-b")&&t.prefs.getInt("existing",0)==7,"preference failure restores files");
  changes.clear();changes.put(first.toFile(),null);changes.put(second.toFile(),SettingsFiles.text("after"));DriverImportJob.failName="second";try{SettingsFiles.commit(changes,t.prefs,prefs);throw new AssertionError("expected failed delete group");}catch(IOException expected){}
  ok(Files.readString(first).equals("before-a"),"deleted file restored after later failure");
  System.out.println("Settings actions passed: 80 page/preset/FPS combinations, explicit preset, file/preferences/deletion rollback");
 }
}'''.replace('METHOD',method)
stubs=dict(stubs)
stubs['com/ptport/app/SettingsActionsTest.java']=test
stubs['com/ptport/app/TouchControls.java']='''package com.ptport.app;class TouchControls{void reloadPreferences(){}void setMode(int mode){}}'''
stubs['com/ptport/app/PTActivity.java']='''package com.ptport.app;class PTActivity{static int cap,presetCalls;static boolean gameRunning(){return true;}static void nativeSetRenderScale(float v){}static void nativeSetBloomPassLimit(int v){}static void nativeSetPerformanceProfile(int v){presetCalls++;if(v==1)cap=0;if(v==2)cap=60;}static void reloadPerformance(){}static void refreshDisplay(){}}'''
stubs['com/ptport/app/AppLog.java']='''package com.ptport.app;class AppLog{static int errors;static void w(String t,String s,Throwable e){errors++;}static void setEnabled(boolean on){}}'''
execute(stubs,'com.ptport.app.SettingsActionsTest')
