#!/usr/bin/env python3
"""Execute hold-to-skip timing, haptic scaling and real gallery-save failure paths."""
from pathlib import Path
import os, subprocess, tempfile, shutil
repo=Path(__file__).resolve().parents[3]
app=repo/'android/app/src/main/java/com/ptport/app'
java=Path(os.environ.get('JAVA_HOME','/usr/lib/jvm/java-17-openjdk-amd64'))/'bin'
def method(source,signature):
 a=source.index(signature);i=source.index('{',a)+1;depth=1
 while depth:
  depth+=(source[i]=='{')-(source[i]=='}');i+=1
 return source[a:i]
intro=(app/'IntroActivity.java').read_text()
tick=intro[intro.index('    private final Runnable holdTick='):intro.index('    private VideoView video;')]
core=r'''
class Clock {static long now;} class SystemClock {static long uptimeMillis(){return Clock.now;}}
class KeyEvent {static final int ACTION_DOWN=0,ACTION_UP=1,KEYCODE_BUTTON_A=96,KEYCODE_BUTTON_START=108,KEYCODE_BUTTON_B=97,KEYCODE_DPAD_CENTER=23,KEYCODE_ENTER=66,KEYCODE_SPACE=62,KEYCODE_BACK=4;int repeat,key,action;KeyEvent(int n){repeat=n;}KeyEvent(int key,int action){this.key=key;this.action=action;}int getRepeatCount(){return repeat;}int getKeyCode(){return key;}int getAction(){return action;}}
class BaseActivity {int childDispatches;public boolean dispatchKeyEvent(KeyEvent e){childDispatches++;return false;}public boolean onKeyDown(int k,KeyEvent e){return false;}public boolean onKeyUp(int k,KeyEvent e){return false;}}
public class IntroCoreTest extends BaseActivity {
 static final long HOLD_MS=3000;boolean resumed=true,finished;int holdKey=-1,holdPointer=-1,leaves;long holdStarted=-1;
 static class Label {String text;void setText(String s){text=s;}} Label skip=new Label();
 static class Handler {Runnable pending;long due;void postDelayed(Runnable r,long delay){pending=r;due=Clock.now+delay;}void removeCallbacks(Runnable r){if(pending==r)pending=null;}} Handler handler=new Handler();
 TICK
 METHODS
 void leave(){finished=true;leaves++;cancelHold();}
 void advance(long ms){long end=Clock.now+ms;while(handler.pending!=null&&handler.due<=end){Clock.now=handler.due;Runnable r=handler.pending;handler.pending=null;r.run();}Clock.now=end;}
 static void ok(boolean b,String why){if(!b)throw new AssertionError(why);}
 public static void main(String[] args){
  IntroCoreTest a=new IntroCoreTest();a.beginHold();a.advance(2999);ok(a.leaves==0,"cannot skip early");a.advance(1);ok(a.leaves==1&&a.handler.pending==null,"three second hold skips once and cancels timer");
  a=new IntroCoreTest();for(int i=0;i<10;i++){a.beginHold();a.advance(2900);a.cancelHold();a.advance(500);}ok(a.leaves==0,"short holds never accumulate");
  a.onKeyDown(KeyEvent.KEYCODE_BUTTON_B,new KeyEvent(0));a.advance(1500);a.onKeyDown(KeyEvent.KEYCODE_BUTTON_B,new KeyEvent(1));a.advance(1499);ok(a.leaves==0,"key repeat neither skips nor resets hold");a.advance(1);ok(a.leaves==1,"controller hold follows same duration");
  a=new IntroCoreTest();a.onKeyDown(KeyEvent.KEYCODE_ENTER,new KeyEvent(0));a.advance(1000);a.onKeyUp(KeyEvent.KEYCODE_ENTER,new KeyEvent(0));a.advance(4000);ok(a.leaves==0,"key release cancels skip");
  a.beginHold();a.advance(1000);a.resumed=false;a.cancelHold();a.advance(4000);ok(a.leaves==0,"background/focus cancellation cannot skip");a.beginHold();ok(a.holdStarted<0,"background cannot start a new hold");
  a=new IntroCoreTest();a.dispatchKeyEvent(new KeyEvent(KeyEvent.KEYCODE_ENTER,KeyEvent.ACTION_DOWN));a.advance(3000);ok(a.leaves==1&&a.childDispatches==0,"Enter hold bypasses focused text control consumption");
  a=new IntroCoreTest();a.dispatchKeyEvent(new KeyEvent(KeyEvent.KEYCODE_BUTTON_A,KeyEvent.ACTION_DOWN));a.advance(2000);a.dispatchKeyEvent(new KeyEvent(KeyEvent.KEYCODE_BUTTON_A,KeyEvent.ACTION_UP));a.advance(3000);ok(a.leaves==0&&a.childDispatches==0,"controller activation release cancels before child click handling");
  System.out.println("Intro hold checks passed: duration, cancellation, repeated presses, controller repeat, background");
 }
}
'''.replace('TICK',tick).replace('METHODS','\n'.join(method(intro,s) for s in ['private void beginHold()','private void cancelHold()','private boolean skipKey(','public boolean dispatchKeyEvent(','public boolean onKeyDown(','public boolean onKeyUp(']))
haptic='''public class HapticTest {METHOD public static void main(String[] args){
 for(int gain=0;gain<=100;gain++)for(int input:new int[]{0,1,128,32768,65535,100000,Integer.MAX_VALUE}){
 int a=amplitude(input,0,gain);if(a<0||a>255||(gain==0&&a!=0)||(input==0&&a!=0))throw new AssertionError("bounds");
 if(input>0&&gain>0&&a==0)throw new AssertionError("positive rumble lost");}
 if(amplitude(65535,0,100)!=255||amplitude(0,65535,50)!=128||amplitude(-1,-1,100)!=0||amplitude(65535,0,-1)!=0||amplitude(65535,0,999)!=255)throw new AssertionError("strength");
 System.out.println("Phone vibration scaling checks passed: 712 amplitude/strength cases");}}'''.replace('METHOD',method((app/'PhoneHaptics.java').read_text(),'static int amplitude('))
stubs={
'android/net/Uri.java':'package android.net;public class Uri {}',
'android/content/ContentValues.java':'package android.content;public class ContentValues extends java.util.HashMap<String,Object>{}',
'android/content/ContentResolver.java':r'''package android.content;import android.net.Uri;import java.io.*;public class ContentResolver {
 public String fail="";public int deletes,publishes;public boolean pending;public ByteArrayOutputStream bytes=new ByteArrayOutputStream();
 public Uri insert(Uri u,ContentValues v){if(fail.equals("insert"))throw new IllegalStateException();pending=v.get("pending").equals(1);if(fail.equals("null-insert"))return null;return new Uri();}
 public OutputStream openOutputStream(Uri u,String mode)throws IOException{if(fail.equals("null-output"))return null;if(fail.equals("open"))throw new IOException();return new FilterOutputStream(bytes){public void write(byte[] b,int o,int n)throws IOException{if(fail.equals("write"))throw new IOException();out.write(b,o,n);}};}
 public int update(Uri u,ContentValues v,String s,String[] a){if(fail.equals("update"))throw new IllegalStateException();if(fail.equals("zero-update"))return 0;if(!v.get("pending").equals(0))throw new AssertionError();publishes++;pending=false;return 1;}
 public int delete(Uri u,String s,String[] a){deletes++;return 1;}
}''',
'android/content/Context.java':'''package android.content;import java.io.File;public class Context {public Context getApplicationContext(){return this;}public File getFilesDir(){return new File("/tmp/pt-photo-test");}public File getExternalFilesDir(String s){return null;}public ContentResolver getContentResolver(){return null;}}''',
'android/app/Activity.java':'package android.app;public class Activity extends android.content.Context{public void runOnUiThread(Runnable r){r.run();}public boolean isFinishing(){return false;}}',
'android/provider/MediaStore.java':'''package android.provider;import android.net.Uri;public class MediaStore {public static class Images {public static class Media {public static final Uri EXTERNAL_CONTENT_URI=new Uri();public static final String DISPLAY_NAME="name",MIME_TYPE="mime",RELATIVE_PATH="path",IS_PENDING="pending";}}}''',
'android/widget/Toast.java':'package android.widget;public class Toast {public static final int LENGTH_LONG=1;public static Toast makeText(Object a,String s,int n){return new Toast();}public void show(){}}',
'com/ptport/app/AppLog.java':'package com.ptport.app;class AppLog {static void w(String a,String b,Throwable t){}}',
'com/ptport/app/PhotoTest.java':r'''package com.ptport.app;import android.content.*;import java.nio.file.*;import java.util.*;public class PhotoTest {
 public static void main(String[] args)throws Exception{Path photo=Files.createTempFile("pt-photo-test-",".png");try{
 byte[] input=new byte[150001];new Random(17).nextBytes(input);Files.write(photo,input);ContentResolver r=new ContentResolver();PhotoStore.save(r,photo.toFile());
 if(!Arrays.equals(input,r.bytes.toByteArray())||r.publishes!=1||r.pending||r.deletes!=0)throw new AssertionError("transaction/bytes");
 for(String failure:new String[]{"insert","null-insert","null-output","open","write","update","zero-update"}){r=new ContentResolver();r.fail=failure;boolean thrown=false;try{PhotoStore.save(r,photo.toFile());}catch(java.io.IOException expected){thrown=true;}
 if(!thrown||r.publishes!=0||r.deletes!=(failure.equals("insert")||failure.equals("null-insert")?0:1)||!Arrays.equals(input,Files.readAllBytes(photo)))throw new AssertionError(failure);}
 System.out.println("Gallery save checks passed: byte-exact multi-chunk PNG copy, publish, 7 failure/rollback paths; source retained");
 }finally{Files.delete(photo);}}
}''',
'IntroCoreTest.java':core,'HapticTest.java':haptic}
with tempfile.TemporaryDirectory() as temp:
 root=Path(temp)
 for name,code in stubs.items():
  p=root/name;p.parent.mkdir(parents=True,exist_ok=True);p.write_text(code)
 shutil.copy2(app/'PhotoStore.java',root/'com/ptport/app/PhotoStore.java')
 subprocess.run([str(java/'javac'),'-d',str(root/'classes')]+[str(p) for p in root.rglob('*.java')],check=True,timeout=30)
 for name in ['IntroCoreTest','HapticTest','com.ptport.app.PhotoTest']:subprocess.run([str(java/'java'),'-cp',str(root/'classes'),name],check=True,timeout=15)

# The exact bundled binary requires a page-size guard; GPU names alone are insufficient.
with tempfile.TemporaryDirectory() as tmp:
 root=Path(tmp);cpp=root/'driver.cpp';exe=root/'driver'
 cpp.write_text('#include "engine/platform/mobile_driver_policy.h"\n#include <cassert>\nint main(){using namespace pt::android;assert(BundledTurnipSupportsPageSize(4096));assert(!BundledTurnipSupportsPageSize(16384));assert(!BundledTurnipSupportsPageSize(0));assert(PreferBundledTurnip("Adreno (TM) 750"));assert(!PreferBundledTurnip("Mali G57"));assert(!PreferBundledTurnip("Immortalis G715"));assert(!PreferBundledTurnip("Xclipse 920"));}')
 subprocess.run(['g++','-std=c++20','-I'+str(repo/'src'),str(cpp),'-o',str(exe)],check=True,timeout=30)
 subprocess.run([str(exe)],check=True,timeout=10)
 print('Bundled Turnip GPU/page-size guard checks passed')
