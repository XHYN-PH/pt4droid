#!/usr/bin/env python3
"""Execute production exit/handoff methods and real cross-process save locks."""
from pathlib import Path
import os
import subprocess
import tempfile
import xml.etree.ElementTree as ET

repo = Path(__file__).resolve().parents[3]
production = repo / "android/app/src/main/java/com/ptport/app"
java = Path(os.environ.get("JAVA_HOME", str(repo.parent / "toolchain/jdk17")))


def method(name, signature):
    source = (production / name).read_text()
    start = source.index(signature)
    end = source.index("{", start) + 1
    depth = 1
    while depth:
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    return source[start:end]


stubs = {
    "android/content/Context.java": """package android.content;import java.io.*;
public class Context {public File root;public File getFilesDir(){return root;}}""",
    "android/content/Intent.java": """package android.content;import java.util.*;
public class Intent {public static final int FLAG_ACTIVITY_NEW_TASK=1,FLAG_ACTIVITY_CLEAR_TOP=2;
public Class<?> target;public int flags;public Map<String,Boolean> extras=new HashMap<>();
public Intent(Context c,Class<?> t){target=t;}public Intent putExtra(String k,boolean v){extras.put(k,v);return this;}
public Intent addFlags(int f){flags|=f;return this;}}
""",
    "android/os/Process.java": """package android.os;
public class Process {public static int killed;public static int myPid(){return 123;}
public static void killProcess(int pid){if(pid!=123)throw new AssertionError("wrong process");killed++;}}""",
    "android/widget/Toast.java": """package android.widget;
public class Toast {public static final int LENGTH_LONG=1,LENGTH_SHORT=0;public static int shown;
public static Toast makeText(Object c,String text,int length){return new Toast();}public void show(){shown++;}}""",
    "org/libsdl/app/SDLActivity.java": """package org.libsdl.app;import android.content.*;
public class SDLActivity extends Context {public static boolean mSDLMainFinished;public static int quits,destroyed;
public boolean finishing;public int finishes,starts;public Intent lastIntent;
public static void nativeSendQuit(){quits++;}public void runOnUiThread(Runnable r){r.run();}
public boolean isFinishing(){return finishing;}public void finish(){finishing=true;finishes++;}
public void startActivity(Intent i){starts++;lastIntent=i;}protected void onDestroy(){destroyed++;}}
""",
}

pt_methods = "\n".join(method("PTActivity.java", s) for s in [
    "void returnToMenu()", "public void finish()", "protected void onDestroy()"
])
setup_methods = "\n".join(method("SetupActivity.java", s) for s in [
    "private void startGame()", "private void launchGame()", "protected void onDestroy()"
])

test = r'''package com.ptport.app;
import android.content.*;import org.libsdl.app.SDLActivity;import java.io.*;import java.nio.file.*;
class TouchControls {int released;void releaseAll(){released++;}}
class MobileRuntime {int paused;void pause(){paused++;}}
class MobileSettings {int detached;void detach(){detached++;}}
class DriverImportJob {static DriverImportJob current;boolean done,cancelled;static DriverImportJob get(){return current;}void cancel(String reason){done=true;cancelled=true;}}
class AppLog {static void w(String t,String s,Throwable error){throw new AssertionError(error);}}
class PTActivity extends SDLActivity {
 TouchControls touchControls=new TouchControls();MobileRuntime mobileRuntime=new MobileRuntime();
 MobileSettings mobileSettings=new MobileSettings();static PTActivity current;
 GameSession gameSession;boolean returningToMenu,menuOpened;
 PT_METHODS
}
class SetupActivity extends SDLActivity {
 boolean handoffToGame,cancelled,microphone;MobileSettings mobileSettings=new MobileSettings();
 boolean shouldAskMicrophone(){return microphone;}void showMicrophone(){}
 void start(){startGame();}void launch(){launchGame();}
 SETUP_METHODS
}
public class GameExitTest {
 static int checks;static void ok(boolean b,String why){if(!b)throw new AssertionError(why);checks++;}
 static Context context(Path root){Context c=new Context();c.root=root.toFile();return c;}
 static java.lang.Process child(Path root)throws Exception{
  return new ProcessBuilder(System.getProperty("java.home")+"/bin/java","-cp",System.getProperty("java.class.path"),
   GameExitTest.class.getName(),"child",root.toString()).start();
 }
 public static void main(String[] args)throws Exception{
  if(args.length>0){try(GameSession session=GameSession.start(context(Path.of(args[1])))){
   System.out.println("locked");System.out.flush();System.in.read();}return;}
  Path root=Files.createTempDirectory("pt-exit-test");Context c=context(root);
  ok(!GameSession.isActive(c),"idle save guard permits deletion");
  GameSession session=GameSession.start(c);ok(GameSession.isActive(c),"same-process overlap blocks deletion");
  session.close();session.close();ok(!GameSession.isActive(c),"close is idempotent and releases saves");
  java.lang.Process other=child(root);ok(new BufferedReader(new InputStreamReader(other.getInputStream())).readLine().equals("locked"),"child acquired actual OS lock");
  ok(GameSession.isActive(c),"game in another process blocks deletion");other.getOutputStream().write(1);other.getOutputStream().flush();
  ok(other.waitFor()==0 && !GameSession.isActive(c),"normal child shutdown releases guard");
  other=child(root);new BufferedReader(new InputStreamReader(other.getInputStream())).readLine();other.destroyForcibly();other.waitFor();
  ok(!GameSession.isActive(c),"process death leaves no stale save lock");
  Path blocked=Files.createDirectory(root.resolve("blocked"));Files.createDirectory(blocked.resolve("game-session.lock"));
  ok(GameSession.isActive(context(blocked)),"unreadable guard fails closed");
  PTActivity game=new PTActivity();game.root=root.toFile();game.gameSession=GameSession.start(c);PTActivity.current=game;
  DriverImportJob.current=new DriverImportJob();
  game.returnToMenu();ok(SDLActivity.quits==1 && game.touchControls.released==1 && game.mobileRuntime.paused==1,"exit clears touch and gyro, then wakes SDL");
  ok(DriverImportJob.current.cancelled,"return cancels unfinished import before process exit");DriverImportJob.current=null;
  game.returnToMenu();ok(SDLActivity.quits==1,"repeated return does not request duplicate quits");
  ok(!game.finishing && game.starts==0 && GameSession.isActive(c) && android.os.Process.killed==0,"native cleanup completes before activity exit or process stop");
  SDLActivity.mSDLMainFinished=true;game.finish();
  ok(game.finishing && game.starts==1 && game.lastIntent.target==SetupActivity.class,"native completion opens launcher and finishes game");
  ok(game.lastIntent.extras.get("returned_from_game") && game.lastIntent.flags==(Intent.FLAG_ACTIVITY_NEW_TASK|Intent.FLAG_ACTIVITY_CLEAR_TOP),"launcher handoff carries return flag and correct task routing");
  game.finish();ok(game.starts==1,"multiple native finish callbacks open one launcher");
  SetupActivity launcher=new SetupActivity();launcher.root=root.toFile();launcher.start();
  ok(launcher.starts==0 && android.widget.Toast.shown==1,"new game is blocked until old native process closes");
  game.onDestroy();ok(SDLActivity.destroyed==1 && game.mobileSettings.detached==1 && PTActivity.current==null,"SDL cleanup and UI detach happen on destruction");
  ok(android.os.Process.killed==1 && !GameSession.isActive(c),"game shutdown releases save guard and stops only its own process");
  DriverImportJob.current=new DriverImportJob();launcher.start();ok(launcher.starts==0,"launcher cannot retire process during an active import");DriverImportJob.current=null;
  launcher.start();ok(launcher.starts==1 && launcher.finishing && launcher.handoffToGame,"next game launches after cleanup");
  launcher.onDestroy();ok(android.os.Process.killed==2 && launcher.cancelled && launcher.mobileSettings.detached==1,"launcher handoff retires old preference cache after lifecycle cleanup");
  launcher=new SetupActivity();launcher.root=root.toFile();launcher.onDestroy();ok(android.os.Process.killed==2,"ordinary launcher destruction does not stop a process");
  SDLActivity.mSDLMainFinished=false;game=new PTActivity();game.root=root.toFile();game.finish();
  ok(game.starts==0,"startup failure does not create return-to-menu loop");
  game.returnToMenu();ok(SDLActivity.quits==1,"finishing game cannot issue another quit");
  for(int i=0;i<100;i++){try(GameSession ignored=GameSession.start(c)){ok(GameSession.isActive(c),"repeated game has save guard");}}
  ok(!GameSession.isActive(c),"100 game sessions leave no stuck lock");
  System.out.println("Menu exit/save guard checks passed: "+checks);
 }
}'''.replace("PT_METHODS", pt_methods).replace("SETUP_METHODS", setup_methods)

with tempfile.TemporaryDirectory() as tmp:
    root = Path(tmp)
    for name, contents in stubs.items():
        path = root / name
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(contents)
    package = root / "com/ptport/app"
    package.mkdir(parents=True)
    (package / "GameSession.java").write_text((production / "GameSession.java").read_text())
    (package / "GameExitTest.java").write_text(test)
    subprocess.run([str(java / "bin/javac"), "-d", str(root / "classes")] +
                   [str(p) for p in root.rglob("*.java")], check=True, timeout=30)
    subprocess.run([str(java / "bin/java"), "-cp", str(root / "classes"),
                    "com.ptport.app.GameExitTest"], check=True, timeout=20)

namespace = "{http://schemas.android.com/apk/res/android}"
manifest = ET.parse(repo / "android/app/src/main/AndroidManifest.xml")
activities = {a.get(namespace + "name"): a for a in manifest.findall("application/activity")}
assert activities[".PTActivity"].get(namespace + "process") == ":game"
assert activities[".SecondScreenActivity"].get(namespace + "process") == ":game"
assert activities[".SetupActivity"].get(namespace + "process") is None
setup = (production / "SetupActivity.java").read_text()
settings = (production / "MobileSettings.java").read_text()
assert 'button("Game graphics"' not in setup and 'button(v,"Game graphics"' in settings
assert 'button(v,"Delete saves"' not in settings
assert 'button("Delete saves"' in setup
assert settings.count("GameSession.isActive(activity)") == 2
assert '"returned_from_game",false' in setup
print("Launcher/menu/process wiring checks passed")
