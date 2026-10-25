"""Exercise the production troubleshooting save transaction independently of widgets."""
from pathlib import Path
from check_graphics_menu_update import stubs,execute
repo=Path(__file__).resolve().parents[3]
s=(repo/'android/app/src/main/java/com/ptport/app/SettingsPages.java').read_text()
a=s.index('    private void saveTroubleshooting()');i=s.index('{',a)+1;depth=1
while depth:
 depth+=(s[i]=='{')-(s[i]=='}');i+=1
method=s[a:i]
code=r'''package com.ptport.app;
import android.app.*;import android.content.*;import java.io.*;import java.nio.file.*;import java.nio.charset.*;import java.util.*;
class TroubleshootingTest{
 Activity activity;SharedPreferences prefs;Map<String,Object>values=new LinkedHashMap<>();
 METHOD
 public static void main(String[]args)throws Exception{
  Path root=Files.createTempDirectory("pt-troubleshooting");TroubleshootingTest test=new TroubleshootingTest();test.activity=new Activity(root.toFile(),null);test.prefs=test.activity.prefs;
  Path ini=root.resolve("mobile_performance.ini");String original="turnip_profile=3\nframe_pacing=1\noutput_scale=0.9\nverbose_logs=0\n";Files.writeString(ini,original);
  Path graphics=root.resolve("game_graphics_pending.ini");Files.writeString(graphics,"[display]\nfps_limit=90\n");
  test.values.put("logs_enabled",true);test.values.put("verbose_logs",true);test.values.put("diagnostic_fresh_shadows",true);
  DriverImportJob.failName="mobile_performance.ini";
  try{test.saveTroubleshooting();throw new AssertionError("injected write must fail");}catch(IOException expected){}
  if(!Files.readString(ini).equals(original)||AppLog.enabled||PTActivity.reloads!=0)throw new AssertionError("failure changed runtime/options");
  test.saveTroubleshooting();String saved=Files.readString(ini);
  for(String line:new String[]{"turnip_profile=3","frame_pacing=1","output_scale=0.9","verbose_logs=1","diagnostic_fresh_shadows=1"})if(!saved.contains(line))throw new AssertionError(line);
  if(!AppLog.enabled||PTActivity.reloads!=1||!Files.readString(graphics).contains("fps_limit=90"))throw new AssertionError("save changed graphics/FPS");
  test.values.put("diagnostic_fresh_shadows",false);test.values.put("verbose_logs",false);test.saveTroubleshooting();
  if(!Files.readString(ini).contains("diagnostic_fresh_shadows=0")||!Files.readString(ini).contains("verbose_logs=0"))throw new AssertionError("cannot restore normal path");
  System.out.println("Troubleshooting transactions: fresh maps on/off, verbose on/off, unchanged FPS/driver, rollback passed");
 }
}'''.replace('METHOD',method)
stubs=dict(stubs)
stubs['com/ptport/app/TroubleshootingTest.java']=code
stubs['com/ptport/app/AppLog.java']='''package com.ptport.app;class AppLog{static int errors;static boolean enabled;static void setEnabled(boolean value){enabled=value;}static void w(String t,String s,Throwable e){}}'''
stubs['com/ptport/app/PTActivity.java']='''package com.ptport.app;class PTActivity{static int reloads;static boolean gameRunning(){return true;}static void reloadPerformance(){reloads++;}}'''
execute(stubs,'com.ptport.app.TroubleshootingTest')
