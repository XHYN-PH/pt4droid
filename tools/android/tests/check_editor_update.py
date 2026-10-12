#!/usr/bin/env python3
"""Exercise production editor events, persistence, layout and drawing with UI boundary stubs."""
from pathlib import Path
import os, subprocess, tempfile

repo = Path(__file__).resolve().parents[3]
path = 'android/app/src/main/java/com/ptport/app/TouchControls.java'
source = (repo/path).read_text()

def method(text, signature):
    a = text.index(signature)
    start = text.index('{', a)
    depth, i = 1, start + 1
    while depth:
        if text[i] == '{': depth += 1
        elif text[i] == '}': depth -= 1
        i += 1
    return text[a:i]

signatures = ['private float dp(', 'private static float clamp(', 'private float radius(',
    'private void defaults()', 'private void load()', 'private void save()', 'private void layout()',
    'void setMode(', 'private void setShown(', 'void releaseAll()', 'private boolean anyFingerDown()',
    'static boolean isFingerEvent(', 'public boolean onTouchEvent(', 'private void updateLook(',
    'private void press(', 'private void release(int i)', 'private float[] axes(', 'private void send()',
    'void beginEdit()', 'boolean cancelEditor(', 'private void finishEdit(', 'private boolean editEvent(', 'void reloadPreferences()',
    'protected void onDraw(', 'private void text(', 'private void drawStick(']

prefix = r'''import java.util.*;
public class EditorTest {
 static int checks,options,visibility;static boolean nativeShown;static float dx,dy;
 static void ok(boolean value,String why){if(!value)throw new AssertionError(why);checks++;}
 static class RectF {
  float left,top,right,bottom;void set(float a,float b,float c,float d){left=a;top=b;right=c;bottom=d;}
  float centerX(){return(left+right)/2;}float centerY(){return(top+bottom)/2;}
  float width(){return right-left;}float height(){return bottom-top;}
  boolean contains(float x,float y){return x>=left && x<right && y>=top && y<bottom;}
 }
 static class Control {int action;float nx,ny,size=1;boolean visible=true;RectF area=new RectF();HashSet<Integer> fingers=new HashSet<>();}
 static class Stick {int pointer=-1;float x,y,originX,originY;}
 static class MotionEvent {
  static final int TOOL_TYPE_FINGER=1;
  static final int ACTION_DOWN=0,ACTION_UP=1,ACTION_MOVE=2,ACTION_CANCEL=3,ACTION_POINTER_DOWN=5,ACTION_POINTER_UP=6;
  int action,index,source=4098,tool=1;int[] ids;float[] x,y;
  MotionEvent(int a,int i,int[] p,float[] xs,float[] ys){action=a;index=i;ids=p;x=xs;y=ys;}
  int getActionMasked(){return action;}int getActionIndex(){return index;}int getPointerCount(){return ids.length;}
  int getPointerId(int i){return ids[i];}float getX(){return x[0];}float getY(){return y[0];}
  float getX(int i){return x[i];}float getY(int i){return y[i];}int getToolType(int i){return tool;}
  boolean isFromSource(int s){return(source&s)==s;}int findPointerIndex(int id){for(int i=0;i<ids.length;i++)if(ids[i]==id)return i;return -1;}
 }
 static class KeyEvent {int action;KeyEvent(int a){action=a;}int getAction(){return action;}}
 static class View {static final int VISIBLE=0,GONE=8;}
 static class Resources {static class Metrics{float density=1;}Metrics getDisplayMetrics(){return new Metrics();}}
 static class Paint {void setColor(int c){}void setStrokeWidth(float w){}void setAlpha(int a){}void setTextSize(float s){}float descent(){return 2;}float ascent(){return -8;}}
 static class Color {static int argb(int a,int r,int g,int b){return(a<<24)|(r<<16)|(g<<8)|b;}}
 static class Canvas {
  int draws,circles;Set<String> texts=new HashSet<>();void draw(){if(++draws>200)throw new AssertionError("unbounded drawing");}
  void drawRoundRect(RectF r,float x,float y,Paint p){draw();}
  void drawText(String s,float x,float y,Paint p){if(!Float.isFinite(x+y))throw new AssertionError("invalid text position");texts.add(s);draw();}
  void drawCircle(float x,float y,float radius,Paint p){circles++;draw();}
 }
 static class SharedPreferences {
  Map<String,Object> data=new HashMap<>();
  float getFloat(String k,float v){return data.containsKey(k)?(Float)data.get(k):v;}
  int getInt(String k,int v){return data.containsKey(k)?(Integer)data.get(k):v;}
  boolean getBoolean(String k,boolean v){return data.containsKey(k)?(Boolean)data.get(k):v;}
  class Editor {Map<String,Object> pending=new HashMap<>();Editor putFloat(String k,float v){pending.put(k,v);return this;}
   Editor putInt(String k,int v){pending.put(k,v);return this;}Editor putBoolean(String k,boolean v){pending.put(k,v);return this;}
   void apply(){data.putAll(pending);}}
  Editor edit(){return new Editor();}
 }
 static class PTActivity {static void openMobileSettings(){}static void setTouchLayoutEditing(boolean v){}}
 static final int[] ACTION_BITS={1,8,2,4,16};static final String[] ACTION_NAMES={"Use","Zoom","Back","Pause","Name"};
 static final long HIDE_AFTER_MS=5000;
 Control[] controls=new Control[7];Stick move=new Stick(),look=new Stick();
 RectF safe=new RectF(),settingsHit=new RectF();RectF[] toolbar={new RectF(),new RectF(),new RectF(),new RectF()};
 Paint ring=new Paint(),fill=new Paint(),label=new Paint();SharedPreferences prefs=new SharedPreferences();
 int insetLeft,insetRight,insetTop,insetBottom,mode,moveStyle=0,lookStyle=1,selected=-1,editPointer=-1,toolbarPressed=-1,lastButtons=-1;
 float deadZone=.05f,dragDx,dragDy,opacity=.7f,sensitivity=1,lastLx,lastLy,lastRx,lastRy,lookLastX,lookLastY;
 boolean floating=true,editing,shown=true;Runnable hideIdle=()->{};int width=1200,height=600;
 EditorTest(){for(int i=0;i<7;i++)controls[i]=new Control();load();}
 int getWidth(){return width;}int getHeight(){return height;}Resources getResources(){return new Resources();}
 void invalidate(){}void postInvalidate(){}void removeCallbacks(Runnable r){}void postDelayed(Runnable r,long delay){}
 void setVisibility(int v){visibility=v;}void showEditorOptions(){options++;}
 static void nativeVisible(boolean v){nativeShown=v;}static void nativeState(float a,float b,float c,float d,int flags){}
 static void nativeClearLook(){dx=dy=0;}static void nativeLookDelta(float x,float y){dx+=x;dy+=y;}
'''
suffix = r'''
 static MotionEvent e(int a,int id,float x,float y){return new MotionEvent(a,0,new int[]{id},new float[]{x},new float[]{y});}
 static void tap(EditorTest t,int toolbarIndex){RectF r=t.toolbar[toolbarIndex];float x=r.centerX(),y=r.centerY();t.onTouchEvent(e(0,10,x,y));t.onTouchEvent(e(1,10,x,y));}
 static void drag(EditorTest t,int control,float x,float y){RectF r=t.controls[control].area;t.onTouchEvent(e(0,1,r.centerX(),r.centerY()));if(t.selected!=control)throw new AssertionError("test must select intended control");t.onTouchEvent(e(2,1,x,y));t.onTouchEvent(e(1,1,x,y));}
 public static void main(String[] args){
  EditorTest t=new EditorTest();t.beginEdit();
  if(args.length>0){t.onTouchEvent(e(0,1,600,300));return;}
  t.onTouchEvent(e(0,1,600,300));ok(t.selected==-1 && t.editPointer==1,"background touch returns");t.onTouchEvent(e(1,1,600,300));
  Canvas canvas=new Canvas();t.onDraw(canvas);ok(canvas.draws>=20 && canvas.draws<40,"all editor controls and toolbar draw");
  drag(t,0,600,500);ok(t.selected==0 && Math.abs(t.controls[0].nx-(600-t.safe.left)/t.safe.width())<.001,"drag changes normalized position");
  float saved=t.controls[0].nx;tap(t,0);ok(!t.editing && t.prefs.getFloat("control.0.x",-1)==saved,"Save persists and exits");
  t.beginEdit();drag(t,0,850,480);tap(t,1);ok(t.controls[0].nx==saved && !t.editing,"Cancel restores saved layout");
  t.beginEdit();drag(t,0,-100,10000);ok(t.controls[0].nx==0 && t.controls[0].ny==1,"drag outside screen bounded");tap(t,3);ok(t.editing && t.controls[0].nx==.92f && t.selected==-1,"Reset restores defaults and remains editable");tap(t,1);ok(t.controls[0].nx==saved,"Cancel also discards Reset");
  t.beginEdit();tap(t,2);ok(options==1 && t.editPointer==-1 && t.editing,"Options opens once after release");
  RectF r=t.toolbar[0];t.onTouchEvent(e(0,2,r.centerX(),r.centerY()));t.onTouchEvent(e(1,2,800,300));ok(t.editing,"releasing outside toolbar does not Save");
  r=t.controls[0].area;t.onTouchEvent(e(0,3,r.centerX(),r.centerY()));float old=t.controls[0].nx;
  t.onTouchEvent(new MotionEvent(2,0,new int[]{99},new float[]{300},new float[]{300}));ok(t.controls[0].nx==old,"missing owning pointer cannot drag");
  t.onTouchEvent(new MotionEvent(6,0,new int[]{99,3},new float[]{300,800},new float[]{300,400}));ok(t.editPointer==3,"other pointer lift does not steal ownership");
  MotionEvent cancel=e(3,3,0,0);cancel.tool=0;cancel.source=0;t.onTouchEvent(cancel);ok(t.editPointer==-1 && t.toolbarPressed==-1,"unknown tool cancellation releases editor pointer");
  t.controls[6].visible=false;t.onDraw(new Canvas());ok(t.editing,"hidden control remains editable");
  t.controls[0].action=4;t.controls[0].size=1.6f;t.sensitivity=3;t.opacity=.4f;tap(t,0);t.load();
  ok(t.controls[0].action==4 && t.controls[0].size==1.6f && t.sensitivity==3 && t.opacity==.4f,"remapping size opacity and sensitivity persist");
  t.prefs.edit().putFloat("control.0.x",Float.NaN).putFloat("sensitivity",Float.POSITIVE_INFINITY).apply();t.load();ok(Float.isFinite(t.controls[0].nx) && t.sensitivity==.25f,"corrupt numeric preferences bounded");
  t.width=0;t.height=0;t.layout();t.width=2400;t.height=1080;t.insetLeft=90;t.insetBottom=40;t.layout();
  ok(t.safe.left==100 && t.safe.bottom==1030 && t.controls[0].area.left>=t.safe.left,"resizing respects safe area");
  t.setMode(1);t.beginEdit();ok(visibility==0 && t.shown && !nativeShown,"Off mode still allows editor");t.cancelEditor(new KeyEvent(1));ok(!t.editing && visibility==8 && !t.shown,"Back exits and restores Off mode");
  t.setMode(2);t.beginEdit();tap(t,0);ok(visibility==0 && t.shown && nativeShown,"Always mode restored after Save");
  ok(t.moveStyle==0,"existing layouts default to visible movement");t.prefs.edit().putInt("move_style",1).putInt("look_style",1).apply();t.reloadPreferences();
  canvas=new Canvas();t.onDraw(canvas);ok(canvas.circles==0 && !canvas.texts.contains("MOVE") && !canvas.texts.contains("LOOK"),"both hidden sticks draw nothing during play");
  t.move.pointer=7;t.move.originX=100;t.move.originY=300;t.move.x=130;t.move.y=300;canvas=new Canvas();t.onDraw(canvas);ok(canvas.circles==0,"active hidden movement draws no thumb or ring");
  t.beginEdit();canvas=new Canvas();t.onDraw(canvas);ok(canvas.circles==2 && canvas.texts.contains("MOVE") && canvas.texts.contains("LOOK"),"editor shows both hidden guides");
  drag(t,5,400,400);t.controls[5].size=1.5f;tap(t,0);t.load();ok(t.moveStyle==1 && t.controls[5].size==1.5f,"layout Save preserves hidden mode and movement size");
  t.move.pointer=8;t.prefs.edit().putInt("move_style",0).apply();t.reloadPreferences();ok(t.move.pointer<0 && t.moveStyle==0,"mode preference reload releases existing movement");
  t.prefs.edit().putInt("move_style",99).apply();t.load();ok(t.moveStyle==1,"out-of-range hidden movement preference bounded");t.prefs.edit().putInt("move_style",-1).apply();t.load();ok(t.moveStyle==0,"negative movement preference bounded");
  for(int i=0;i<5000;i++){t.beginEdit();t.onTouchEvent(e(0,1,600,300));t.onDraw(new Canvas());t.finishEdit(false);}
  ok(!t.editing,"repeated editor open touch draw close completes");
  System.out.println("Editor checks passed: "+checks+"; 5000 edit cycles");
 }
}'''
java_home=Path(os.environ.get('JAVA_HOME',str(repo.parent/'toolchain/host/usr/lib/jvm/java-17-openjdk-amd64')))
with tempfile.TemporaryDirectory() as tmp:
    t=Path(tmp)
    def run(text, folder, baseline=False):
        folder.mkdir(); java=folder/'EditorTest.java'
        java.write_text(prefix+'\n'.join(method(text,s) for s in signatures)+suffix)
        subprocess.run([str(java_home/'bin/javac'),str(java)],check=True,timeout=30)
        subprocess.run([str(java_home/'bin/java'),'-cp',str(folder),'EditorTest']+(['hang'] if baseline else []),check=True,timeout=2 if baseline else 15)
    # The first public fork has different commit IDs from the earlier private test build.
    # 6e78c43 is the public revision before the editor loop was fixed.
    baseline=subprocess.check_output(['git','show','6e78c43:'+path],cwd=repo,text=True)
    try: run(baseline,t/'baseline',True)
    except subprocess.TimeoutExpired: print('Baseline editor freeze reproduced')
    else: raise AssertionError('Expected the reported editor freeze in the previous revision')
    run(source,t/'fixed')
