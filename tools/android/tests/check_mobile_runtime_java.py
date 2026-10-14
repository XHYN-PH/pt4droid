#!/usr/bin/env python3
"""Execute production sensor lifecycle and first-run permission actions against Android boundary stubs."""
from pathlib import Path
import os, shutil, subprocess, tempfile

repo=Path(__file__).resolve().parents[3]
java=Path(os.environ.get('JAVA_HOME','/usr/lib/jvm/java-17-openjdk-amd64'))
def method(source,signature):
    start=source.index(signature);at=source.index('{',start)+1;depth=1
    while depth:
        depth+=(source[at]=='{')-(source[at]=='}');at+=1
    return source[start:at]

stubs={
'android/content/Context.java':'''package android.content;import java.io.File;public class Context {public static final String SENSOR_SERVICE="sensor",POWER_SERVICE="power",DISPLAY_SERVICE="display";public File internal,external;public Object getSystemService(String s){return null;}public File getFilesDir(){return internal;}public File getExternalFilesDir(String s){return external;}}''',
'android/content/SharedPreferences.java':'''package android.content;import java.util.*;public class SharedPreferences{public Map<String,Object> values=new HashMap<>();public boolean getBoolean(String s,boolean f){return (Boolean)values.getOrDefault(s,f);}public float getFloat(String s,float f){return (Float)values.getOrDefault(s,f);}}''',
'android/hardware/Sensor.java':'''package android.hardware;public class Sensor{public static final int TYPE_GYROSCOPE=4;}''',
'android/hardware/SensorEvent.java':'''package android.hardware;public class SensorEvent{public long timestamp;public float[] values;public SensorEvent(long t,float x,float y){timestamp=t;values=new float[]{x,y,0};}}''',
'android/hardware/SensorEventListener.java':'''package android.hardware;public interface SensorEventListener{void onSensorChanged(SensorEvent e);void onAccuracyChanged(Sensor s,int a);}''',
'android/hardware/SensorManager.java':'''package android.hardware;public class SensorManager{public static final int SENSOR_DELAY_GAME=1;public boolean available=true;public int added,removed;public Sensor getDefaultSensor(int t){return available?new Sensor():null;}public boolean registerListener(SensorEventListener l,Sensor s,int rate){added++;return true;}public void unregisterListener(SensorEventListener l){removed++;}}''',
'android/hardware/display/DisplayManager.java':'''package android.hardware.display;public class DisplayManager{public interface DisplayListener{void onDisplayAdded(int id);void onDisplayRemoved(int id);void onDisplayChanged(int id);}public int added,removed;public DisplayListener listener;public void registerDisplayListener(DisplayListener l,Object h){added++;listener=l;}public void unregisterDisplayListener(DisplayListener l){removed++;listener=null;}}''',
'android/os/PowerManager.java':'''package android.os;import java.util.concurrent.Executor;public class PowerManager{public interface OnThermalStatusChangedListener{void onThermalStatusChanged(int status);}public boolean supported=true;public int status,added,removed;public OnThermalStatusChangedListener listener;public boolean isSustainedPerformanceModeSupported(){return supported;}public int getCurrentThermalStatus(){return status;}public void addThermalStatusListener(Executor e,OnThermalStatusChangedListener l){added++;listener=l;}public void removeThermalStatusListener(OnThermalStatusChangedListener l){removed++;listener=null;}}''',
'android/view/Surface.java':'''package android.view;public class Surface{public static final int ROTATION_0=0,ROTATION_90=1,ROTATION_180=2,ROTATION_270=3;}''',
'android/view/Window.java':'''package android.view;public class Window{public boolean sustained;public void setSustainedPerformanceMode(boolean v){sustained=v;}}''',
'android/view/Display.java':'''package android.view;public class Display{public int rotation;public int getRotation(){return rotation;}}''',
'android/view/WindowManager.java':'''package android.view;public class WindowManager{public Display display=new Display();public Display getDefaultDisplay(){return display;}}''',
'android/widget/Toast.java':'''package android.widget;public class Toast{public static final int LENGTH_LONG=1;public static int shown;public static Toast makeText(Object a,String s,int l){return new Toast();}public void show(){shown++;}}''',
'com/ptport/app/PTActivity.java':'''package com.ptport.app;import android.content.*;import android.hardware.*;import android.os.*;import android.view.*;import java.util.concurrent.Executor;public class PTActivity extends Context{public SensorManager sensors=new SensorManager();public PowerManager power=new PowerManager();public android.hardware.display.DisplayManager displays=new android.hardware.display.DisplayManager();static int refreshes;public static void refreshDisplay(){refreshes++;}public SharedPreferences prefs=new SharedPreferences();public Window window=new Window();public WindowManager manager=new WindowManager();static int moves,thermal;static float dx,dy;public Object getSystemService(String s){return s.equals(SENSOR_SERVICE)?sensors:s.equals(DISPLAY_SERVICE)?displays:power;}public SharedPreferences getSharedPreferences(String s,int m){return prefs;}public Window getWindow(){return window;}public WindowManager getWindowManager(){return manager;}public Executor getMainExecutor(){return Runnable::run;}static void nativeGyroDelta(float x,float y){moves++;dx+=x;dy+=y;}static void nativeThermalStatus(int v){thermal=v;}}''',
'com/ptport/app/RuntimeTest.java':r'''package com.ptport.app;import android.hardware.*;import android.view.Surface;public class RuntimeTest{
 static int checks;static void ok(boolean v,String why){if(!v)throw new AssertionError(why);checks++;}
 public static void main(String[] a)throws Exception{PTActivity owner=new PTActivity();MobileRuntime r=new MobileRuntime(owner);
  r.allow(true);r.resume();ok(owner.sensors.added==0&&owner.power.added==1&&!owner.window.sustained,"defaults keep sensors off while observing thermal status");
  int refreshes=PTActivity.refreshes;owner.displays.listener.onDisplayChanged(0);ok(PTActivity.refreshes==refreshes+1,"display changes refresh the observed rate");
  r.resume();ok(owner.displays.added==1,"repeated resume registers one display listener");
  owner.prefs.values.put("gyro",true);r.reload();ok(owner.sensors.added==1,"enabling registers once");
  r.onSensorChanged(new SensorEvent(1000000000L,1,2));ok(PTActivity.moves==0,"first sample discarded");
  owner.manager.display.rotation=Surface.ROTATION_90;r.onSensorChanged(new SensorEvent(1010000000L,1,2));ok(Math.abs(PTActivity.dx-.01f)<1e-6&&Math.abs(PTActivity.dy+.02f)<1e-6,"landscape gyro axes and seconds convert to radians");
  int moves=PTActivity.moves;r.onSensorChanged(new SensorEvent(1200000000L,1,2));ok(PTActivity.moves==moves,"late samples do not jump");
  r.focus(false);ok(owner.sensors.removed==1,"focus loss unregisters");r.onSensorChanged(new SensorEvent(1210000000L,1,2));ok(PTActivity.moves==moves,"unfocused input ignored");
  r.focus(true);r.onSensorChanged(new SensorEvent(1220000000L,1,2));ok(PTActivity.moves==moves,"focus return discards first sample");
  r.allow(false);ok(owner.sensors.removed==2,"menu disables gyro");r.allow(true);
  owner.prefs.values.put("sustained",true);owner.power.status=3;r.reload();ok(owner.window.sustained&&owner.power.added==1&&PTActivity.thermal==3,"sustained mode and initial thermal state applied");
  owner.power.listener.onThermalStatusChanged(4);ok(PTActivity.thermal==4,"thermal changes reach native policy");
  r.pause();ok(!owner.window.sustained&&owner.power.removed==1&&owner.sensors.removed==3,"pause releases all resources");r.pause();ok(owner.power.removed==1&&owner.sensors.removed==3,"repeated pause is safe");
  owner.power.supported=false;r.resume();ok(!owner.window.sustained,"unsupported sustained hint is not requested");
  owner.prefs.values.put("sustained",false);owner.prefs.values.put("gyro",false);r.reload();ok(owner.power.removed==1&&owner.sensors.removed==4,"disabling stops gyro while retaining thermal observation");
  PTActivity absent=new PTActivity();absent.sensors.available=false;absent.prefs.values.put("gyro",true);MobileRuntime missing=new MobileRuntime(absent);missing.allow(true);missing.resume();ok(!MobileRuntime.hasGyro(absent)&&absent.sensors.added==0,"missing sensor is safe");
  java.nio.file.Path temporary=java.nio.file.Files.createTempDirectory("pt-storage-test");owner.internal=temporary.resolve("internal").toFile();owner.internal.mkdir();
  ok(GameStorage.dataRoot(owner).equals(owner.internal),"null external folder falls back internally");
  owner.external=temporary.resolve("external").toFile();ok(GameStorage.dataRoot(owner).equals(owner.external)&&owner.external.list().length==0,"writable app storage selected and probe removed");
  java.io.File blocked=temporary.resolve("blocked").toFile();blocked.createNewFile();owner.external=blocked;ok(GameStorage.dataRoot(owner).equals(owner.internal),"invalid external storage falls back internally");
  owner.prefs.values.put("gyro",true);r.reload();int registered=owner.sensors.added,removed=owner.sensors.removed;
  r.edit(true);ok(owner.sensors.removed==removed+1,"layout editor unregisters gyro");r.allow(true);ok(owner.sensors.added==registered,"gameplay callback cannot enable gyro inside editor");
  r.edit(false);ok(owner.sensors.added==registered+1,"leaving editor restores gyro when enabled");r.pause();
  blocked.delete();temporary.resolve("external").toFile().delete();owner.internal.delete();temporary.toFile().delete();
  ok(owner.displays.added==owner.displays.removed,"display listeners are removed on pause");
  System.out.println("Sensor/thermal lifecycle checks passed: "+checks);
 }
}'''
}

with tempfile.TemporaryDirectory() as temporary:
    root=Path(temporary)
    for name,content in stubs.items():
        file=root/name;file.parent.mkdir(parents=True,exist_ok=True);file.write_text(content)
    shutil.copy2(repo/'android/app/src/main/java/com/ptport/app/MobileRuntime.java',root/'com/ptport/app/MobileRuntime.java')
    shutil.copy2(repo/'android/app/src/main/java/com/ptport/app/GameStorage.java',root/'com/ptport/app/GameStorage.java')
    source=(repo/'android/app/src/main/java/com/ptport/app/SetupActivity.java').read_text()
    actions='\n'.join(method(source,s) for s in ['private boolean storageGranted()', 'private void promptFirstRunStorage()', 'private void requestAllFilesAccess()', 'private void finishStorageRequest()', 'private void pickPackage()'])
    permission_prefix=r'''import java.util.function.BiConsumer;
public class StorageTest{
 static int checks,dialogs,starts,ready,welcome,errors,toasts;static void ok(boolean v,String why){if(!v)throw new AssertionError(why);checks++;}
 static final int ASK_STORAGE=3,PICK_PKG=1,MODE_PRIVATE=0;boolean waitingForAccess,firstRunAccess,ownGame,sharedGame,finishing;int permission=-1,requests,lastCode,failSettings;Intent lastIntent;SharedPreferences prefs=new SharedPreferences();
 static class Build{static class VERSION{static int SDK_INT=35;}static class VERSION_CODES{static final int R=30;}}
 static class Environment{static boolean granted;static boolean isExternalStorageManager(){return granted;}}
 static class PackageManager{static final int PERMISSION_GRANTED=0;}
 static class Manifest{static class permission{static final String READ_EXTERNAL_STORAGE="read";}}
 static class Settings{static final String ACTION_MANAGE_APP_ALL_FILES_ACCESS_PERMISSION="app",ACTION_MANAGE_ALL_FILES_ACCESS_PERMISSION="all";}
 static class Uri{String value;static Uri parse(String s){Uri u=new Uri();u.value=s;return u;}}
 static class Intent{static final String ACTION_OPEN_DOCUMENT="pick",CATEGORY_OPENABLE="open";String action;Uri uri;Intent(String s){action=s;}void setData(Uri u){uri=u;}void addCategory(String s){}void setType(String s){}}
 static class SharedPreferences{boolean asked;boolean getBoolean(String s,boolean f){return asked;}Editor edit(){return new Editor();}class Editor{Editor putBoolean(String s,boolean v){asked=v;return this;}void apply(){}}}
 static class AlertDialog{static BiConsumer<Object,Integer> allow;static class Builder{Builder(Object a){}Builder setTitle(String s){return this;}Builder setMessage(String s){return this;}Builder setPositiveButton(String s,BiConsumer<Object,Integer> v){allow=v;return this;}Builder setNegativeButton(String s,BiConsumer<Object,Integer> v){return this;}AlertDialog create(){return new AlertDialog();}}void show(){dialogs++;}}
 static class MenuStyle{static void styleDialog(Object d){}}
 boolean isFinishing(){return finishing;}boolean isDestroyed(){return false;}SharedPreferences getSharedPreferences(String s,int m){return prefs;}int checkSelfPermission(String s){return permission;}String getPackageName(){return "com.ptport.app.test";}
 boolean gameInstalled(){return ownGame||(sharedGame&&storageGranted());}void showReady(){ready++;}void showWelcome(){welcome++;}void showError(String s){errors++;}void startGame(){starts++;}
 void requestPermissions(String[] p,int code){requests++;lastCode=code;ok(p.length==1&&p[0].equals("read"),"Android 10 runtime permission");}
 void startActivityForResult(Intent i,int code){if(!i.action.equals("pick")&&failSettings-->0)throw new RuntimeException("unavailable");requests++;lastCode=code;lastIntent=i;}
'''
    permission_suffix=r'''
 public static void main(String[] a){
  StorageTest s=new StorageTest();s.promptFirstRunStorage();ok(dialogs==1&&s.prefs.asked,"first launch prompts and persists once");s.promptFirstRunStorage();ok(dialogs==1,"no prompt loop");AlertDialog.allow.accept(null,0);ok(s.waitingForAccess&&s.firstRunAccess&&s.lastIntent.action.equals("app")&&s.lastCode==ASK_STORAGE,"first-run opens the per-app system grant");
  s.finishStorageRequest();ok(!s.waitingForAccess&&!s.firstRunAccess&&welcome==1&&starts==0,"denial returns to import screen without launching");s.pickPackage();ok(s.lastCode==PICK_PKG&&s.lastIntent.action.equals("pick"),"file-picker import remains available without access");
  s.sharedGame=true;s.firstRunAccess=true;s.requestAllFilesAccess();Environment.granted=true;s.finishStorageRequest();ok(ready==1&&starts==0,"first-run grant refreshes launcher without auto-start");
  s.requestAllFilesAccess();ok(ready==2&&starts==0,"manual extracted-files action enters menu without auto-start");
  s.firstRunAccess=true;s.ownGame=false;s.sharedGame=false;s.requestAllFilesAccess();ok(!s.firstRunAccess&&welcome==2,"grant race consumes the first-run flag safely");
  Environment.granted=false;StorageTest blocked=new StorageTest();blocked.firstRunAccess=true;blocked.failSettings=2;blocked.requestAllFilesAccess();ok(!blocked.waitingForAccess&&!blocked.firstRunAccess&&android.widget.Toast.shown==1,"missing settings screen does not leave pending state");
  StorageTest fallback=new StorageTest();fallback.firstRunAccess=true;fallback.failSettings=1;fallback.requestAllFilesAccess();ok(fallback.lastIntent.action.equals("all")&&fallback.waitingForAccess,"global settings fallback works");fallback.finishStorageRequest();ok(!fallback.waitingForAccess,"cancelled settings returns cleanly");
  Build.VERSION.SDK_INT=29;StorageTest legacy=new StorageTest();legacy.firstRunAccess=true;legacy.requestAllFilesAccess();ok(legacy.requests==1&&legacy.lastCode==ASK_STORAGE,"Android 10 requests runtime storage access");legacy.permission=0;legacy.sharedGame=true;legacy.finishStorageRequest();ok(!legacy.waitingForAccess&&ready==3,"Android 10 grant refreshes ready screen");
  StorageTest own=new StorageTest();own.ownGame=true;own.firstRunAccess=true;own.requestAllFilesAccess();own.finishStorageRequest();ok(ready==4,"imported internal game works after denied shared access");
  System.out.println("First-run storage and file-picker checks passed: "+checks);
 }
}'''
    (root/'StorageTest.java').write_text(permission_prefix+actions+permission_suffix)
    subprocess.run([str(java/'bin/javac'),'-d',str(root/'classes')]+[str(p) for p in root.rglob('*.java')],check=True,timeout=30)
    for name in ['com.ptport.app.RuntimeTest','StorageTest']:
        subprocess.run([str(java/'bin/java'),'-cp',str(root/'classes'),name],check=True,timeout=15)
