#!/usr/bin/env python3
"""Run the production touch event/state methods against a deterministic event sink."""
from pathlib import Path
import os,tempfile,subprocess
repo=Path(__file__).resolve().parents[3]
source=(repo/'android/app/src/main/java/com/ptport/app/TouchControls.java').read_text()
def method(signature):
 a=source.index(signature);start=source.index('{',a);depth=1;i=start+1
 while depth:
  if source[i]=='{':depth+=1
  elif source[i]=='}':depth-=1
  i+=1
 return source[a:i]
methods='\n'.join(method(s) for s in ['public boolean onTouchEvent(', 'private void updateLook(', 'private void press(', 'private void release(int i)', 'private float[] axes(', 'private void send()', 'private boolean anyFingerDown()', 'static boolean isFingerEvent(', 'private void setShown(', 'void releaseAll()', 'boolean toolbarVisible()'])
code=r'''import java.util.*;
public class TouchTest {
 static int checks,stateCalls,settingsOpened;static float dx,dy,rx,ry,lx,ly;static int buttons;static boolean visible;
 static void ok(boolean b,String why){if(!b)throw new AssertionError(why);checks++;}
 static class RectF{float left,top,right,bottom;RectF(float a,float b,float c,float d){left=a;top=b;right=c;bottom=d;}float centerX(){return(left+right)/2;}float centerY(){return(top+bottom)/2;}float width(){return right-left;}float height(){return bottom-top;}boolean contains(float x,float y){return x>=left&&x<right&&y>=top&&y<bottom;}}
 static class Control{int action;boolean visible=true;float size=1;RectF area=new RectF(-200,-200,-100,-100);HashSet<Integer> fingers=new HashSet<>();}
 static class Stick{int pointer=-1;float x,y,originX,originY;}
 static class MotionEvent{
  static final int TOOL_TYPE_FINGER=1;
  static final int ACTION_DOWN=0,ACTION_UP=1,ACTION_MOVE=2,ACTION_CANCEL=3,ACTION_POINTER_DOWN=5,ACTION_POINTER_UP=6;
  int action,index,source=4098,tool=1;int[] ids;float[] x,y;
  MotionEvent(int a,int i,int[] ids,float[] x,float[] y){action=a;index=i;this.ids=ids;this.x=x;this.y=y;}
  int getActionMasked(){return action;}int getActionIndex(){return index;}int getPointerCount(){return ids.length;}int getPointerId(int i){return ids[i];}float getX(){return x[0];}float getY(){return y[0];}float getX(int i){return x[i];}float getY(int i){return y[i];}int getToolType(int i){return tool;}boolean isFromSource(int s){return(source&s)==s;}
 }
 static class PTActivity{static void openMobileSettings(){settingsOpened++;}static void setTouchLayoutEditing(boolean v){}static void touchOverlayShown(boolean shown){}}
 Control[] controls=new Control[7];Stick move=new Stick(),look=new Stick();RectF safe=new RectF(0,0,1200,600),settingsHit=new RectF(-100,-100,-50,-50);
 int mode=0,moveStyle=0,lookStyle=1,lastButtons=-1;float sensitivity=1,deadZone=.05f,lastLx,lastLy,lastRx,lastRy,lookLastX,lookLastY;
 boolean floating=true,shown=true,editing,editorSaw;Runnable hideIdle=()->{};long timeout=5000;long scheduled=-1;long hideAfterMs(){return timeout;}static final int[] ACTION_BITS={1,8,2,4,16};
 TouchTest(){for(int i=0;i<7;i++){controls[i]=new Control();controls[i].action=i<5?i:0;}}
 float dp(float value){return value;}float radius(int i){return 56*controls[i].size;}static float clamp(float v,float a,float b){return Math.max(a,Math.min(b,v));}
 void removeCallbacks(Runnable r){}void postDelayed(Runnable r,long time){scheduled=time;}void invalidate(){}void postInvalidate(){}boolean editEvent(MotionEvent e){editorSaw=true;return true;}
 static void nativeLookDelta(float x,float y){dx+=x;dy+=y;}static void nativeClearLook(){dx=dy=0;}static void nativeVisible(boolean value){visible=value;}
 static void nativeState(float a,float b,float c,float d,int bits){stateCalls++;lx=a;ly=b;rx=c;ry=d;buttons=bits;}
''' + methods + r'''
 static MotionEvent e(int action,int id,float x,float y){return new MotionEvent(action,0,new int[]{id},new float[]{x},new float[]{y});}
 static MotionEvent multi(int action,int index,int[] ids,float[] x,float[] y){return new MotionEvent(action,index,ids,x,y);}
 static void reset(){dx=dy=rx=ry=lx=ly=0;buttons=stateCalls=settingsOpened=0;}
 public static void main(String[] args){
  reset();TouchTest t=new TouchTest();t.floating=false;t.controls[6].visible=false;t.onTouchEvent(e(0,1,900,300));t.onTouchEvent(e(2,1,960,330));ok(Math.abs(dx-72)<.001 && Math.abs(dy-36)<.001,"invisible fixed swipe");ok(rx==0 && ry==0,"swipe sends no joystick rotation");float before=dx;t.onTouchEvent(e(2,1,960,330));ok(dx==before,"stationary finger does not turn");t.onTouchEvent(e(1,1,980,330));ok(Math.abs(dx-96)<.001 && t.look.pointer<0,"final lift included");
  reset();t=new TouchTest();t.onTouchEvent(e(0,1,100,100));t.onTouchEvent(multi(5,1,new int[]{1,2},new float[]{100,900},new float[]{100,300}));t.onTouchEvent(multi(2,0,new int[]{1,2},new float[]{150,950},new float[]{150,300}));ok(lx>0 && ly>0 && dx>0,"walking and looking together");ok(Math.sqrt(lx*lx+ly*ly)<=1.0001,"diagonal stick bounded");MotionEvent cancel=e(3,2,0,0);cancel.tool=0;cancel.source=0;t.onTouchEvent(cancel);ok(lx==0 && ly==0 && dx==0 && t.look.pointer<0 && t.move.pointer<0,"unknown tool cancellation releases controls");
  reset();t=new TouchTest();t.controls[0].area=new RectF(800,200,1000,400);t.onTouchEvent(e(0,4,900,300));ok(buttons==1 && t.look.pointer<0,"buttons take priority");t.onTouchEvent(multi(5,1,new int[]{4,5},new float[]{900,910},new float[]{300,310}));t.onTouchEvent(multi(6,0,new int[]{4,5},new float[]{900,910},new float[]{300,310}));ok(buttons==1,"one of two fingers retains button");t.onTouchEvent(e(1,5,910,310));ok(buttons==0,"last finger releases button");t.controls[0].action=4;t.onTouchEvent(e(0,6,900,300));ok(buttons==16,"button remapping");t.releaseAll();
  reset();t=new TouchTest();MotionEvent mouse=e(0,1,900,300);mouse.source=8194;mouse.tool=3;ok(!t.onTouchEvent(mouse) && dx==0,"physical mouse passes through");t.mode=1;ok(!t.onTouchEvent(e(0,1,900,300)),"Off passes through");
  reset();t=new TouchTest();t.lookStyle=2;t.floating=false;t.controls[6].visible=false;t.onTouchEvent(e(0,7,900,300));t.onTouchEvent(e(2,7,930,300));ok(rx>0 && dx==0,"hidden joystick continuous axis");t.onTouchEvent(e(1,7,930,300));ok(rx==0,"hidden joystick release");
  reset();t=new TouchTest();t.safe=new RectF(0,0,2400,1200);t.onTouchEvent(e(0,8,1800,600));t.onTouchEvent(e(2,8,1920,660));ok(Math.abs(dx-72)<.001 && Math.abs(dy-36)<.001,"sensitivity follows screen size");t.releaseAll();t.editing=true;t.onTouchEvent(e(0,9,1900,600));ok(t.editorSaw && dx==0,"editor owns input");t.editing=false;t.setShown(false);ok(!visible,"hidden overlay visibility cleared");
  reset();t=new TouchTest();t.moveStyle=1;t.floating=false;t.controls[5].visible=false;
  t.onTouchEvent(e(0,10,120,300));ok(t.move.pointer==10 && t.move.originX==120 && lx==0 && ly==0,"hidden movement starts at touch even with floating/visibility off");
  t.onTouchEvent(e(2,10,148,300));ok(lx>0 && lx<1 && ly==0,"hidden movement retains analog speed");
  int calls=stateCalls;t.onTouchEvent(e(2,10,148,300));ok(lx>0 && stateCalls==calls,"held movement persists without redundant native writes");
  t.onTouchEvent(e(1,10,148,300));ok(lx==0 && ly==0 && t.move.pointer<0,"lifting hidden movement stops immediately");
  t.onTouchEvent(e(0,11,220,300));t.onTouchEvent(e(2,11,221,300));ok(lx==0,"hidden movement respects dead zone");
  t.controls[5].size=1.75f;t.onTouchEvent(e(2,11,248,300));float largeStick=lx;t.controls[5].size=.65f;t.onTouchEvent(e(2,11,248,300));ok(lx>largeStick,"editor stick size affects hidden movement");t.releaseAll();
  reset();t=new TouchTest();t.moveStyle=1;t.floating=false;
  t.onTouchEvent(e(0,12,100,300));t.onTouchEvent(multi(5,1,new int[]{12,13},new float[]{100,900},new float[]{300,300}));
  t.onTouchEvent(multi(2,0,new int[]{13,12},new float[]{450,750},new float[]{330,250}));
  ok(t.move.pointer==12 && t.look.pointer==13 && lx>0 && ly<0 && dx<0 && dy>0,"both roles survive reordering and crossing the centre");
  t.onTouchEvent(multi(6,1,new int[]{13,12},new float[]{450,750},new float[]{330,250}));ok(lx==0 && ly==0 && t.look.pointer==13,"movement lift leaves look owner intact");
  t.onTouchEvent(e(2,13,480,330));ok(dx<0,"remaining look finger continues after movement lift");t.releaseAll();
  reset();t=new TouchTest();t.moveStyle=1;
  t.onTouchEvent(e(0,14,100,300));t.onTouchEvent(multi(5,1,new int[]{14,15},new float[]{100,300},new float[]{300,300}));
  t.onTouchEvent(multi(2,0,new int[]{14,15},new float[]{130,400},new float[]{300,300}));ok(t.move.pointer==14 && lx<1,"second left finger cannot steal origin");
  t.onTouchEvent(multi(6,0,new int[]{14,15},new float[]{130,400},new float[]{300,300}));ok(t.move.pointer<0 && lx==0,"extra held finger is not adopted when movement owner lifts");
  t.onTouchEvent(e(2,15,500,300));ok(t.move.pointer<0 && lx==0,"extra held finger cannot restart movement without a fresh press");t.releaseAll();
  reset();t=new TouchTest();t.moveStyle=1;t.controls[0].area=new RectF(100,200,200,400);
  t.onTouchEvent(e(0,16,150,300));t.onTouchEvent(e(2,16,250,300));ok(buttons==1 && t.move.pointer<0 && lx==0,"left action button wins and dragging it cannot become movement");t.onTouchEvent(e(1,16,250,300));ok(buttons==0,"dragged left button still releases");
  t.settingsHit=new RectF(0,0,90,36);t.onTouchEvent(e(0,17,250,300));t.onTouchEvent(e(2,17,280,300));
  t.onTouchEvent(multi(5,1,new int[]{17,18},new float[]{280,45},new float[]{300,18}));ok(settingsOpened==1 && t.move.pointer<0 && lx==0,"Settings wins with a second finger and releases movement");
  t.onTouchEvent(e(0,19,45,18));ok(settingsOpened==2 && t.move.pointer<0,"Settings wins on the first finger too");
  reset();t=new TouchTest();t.moveStyle=1;t.safe=new RectF(100,40,1100,560);
  for(float[] point:new float[][]{{99,300},{1101,300},{300,39},{900,561}}){t.onTouchEvent(e(0,20,point[0],point[1]));ok(t.move.pointer<0 && t.look.pointer<0,"hidden zones exclude unsafe bounds");t.onTouchEvent(e(1,20,point[0],point[1]));}
  t.onTouchEvent(e(0,21,600,300));ok(t.look.pointer==21 && t.move.pointer<0,"exact centre belongs to look");t.releaseAll();
  t.onTouchEvent(e(0,22,599,300));ok(t.move.pointer==22 && t.look.pointer<0,"left of centre belongs to movement");t.releaseAll();
  reset();t=new TouchTest();t.floating=false;t.lookStyle=0;t.controls[5].area=new RectF(100,200,212,312);t.controls[6].area=new RectF(800,200,912,312);
  t.onTouchEvent(e(0,23,50,300));ok(t.move.pointer<0,"visible fixed movement still requires its joystick");t.onTouchEvent(e(1,23,50,300));
  t.onTouchEvent(e(0,24,170,270));ok(t.move.originX==156 && t.move.originY==256,"visible fixed movement keeps saved centre");t.releaseAll();
  t.moveStyle=1;t.onTouchEvent(e(0,25,50,300));t.onTouchEvent(multi(5,1,new int[]{25,26},new float[]{50,856},new float[]{300,256}));
  t.onTouchEvent(multi(2,0,new int[]{25,26},new float[]{80,886},new float[]{300,256}));ok(lx>0 && rx>0 && dx==0,"hidden movement works with visible fixed look");t.releaseAll();
  reset();t=new TouchTest();t.moveStyle=1;t.lookStyle=2;t.floating=false;
  t.onTouchEvent(e(0,27,100,300));t.onTouchEvent(multi(5,1,new int[]{27,28},new float[]{100,900},new float[]{300,300}));
  t.onTouchEvent(multi(2,0,new int[]{27,28},new float[]{200,1000},new float[]{400,400}));ok(lx>0 && ly>0 && rx>0 && ry>0 && dx==0,"both invisible joysticks work simultaneously");
  MotionEvent hiddenCancel=e(3,0,0,0);hiddenCancel.tool=0;hiddenCancel.source=0;t.onTouchEvent(hiddenCancel);ok(lx==0 && ly==0 && rx==0 && ry==0 && !t.anyFingerDown(),"cancel releases both invisible joysticks");
  t.onTouchEvent(e(0,29,100,300));t.onTouchEvent(e(2,29,130,300));t.releaseAll();ok(lx==0 && !t.anyFingerDown(),"lifecycle release stops hidden movement");
  t.controls[5].visible=false;t.moveStyle=0;t.onTouchEvent(e(0,30,100,300));ok(t.move.pointer<0,"visible-mode disable flag is preserved after changing styles");
  reset();t=new TouchTest();t.moveStyle=1;t.controls[0].area=new RectF(800,100,950,200);
  t.onTouchEvent(e(0,31,100,300));t.onTouchEvent(multi(5,1,new int[]{31,32},new float[]{100,1000},new float[]{300,300}));
  t.onTouchEvent(multi(5,2,new int[]{31,32,33},new float[]{100,1000,875},new float[]{300,300,150}));
  t.onTouchEvent(multi(2,0,new int[]{31,32,33},new float[]{130,1030,875},new float[]{300,300,150}));ok(lx>0 && dx>0 && buttons==1,"movement look and action work with three fingers");
  t.onTouchEvent(multi(6,2,new int[]{31,32,33},new float[]{130,1030,875},new float[]{300,300,150}));ok(buttons==0 && t.move.pointer==31 && t.look.pointer==32,"action lift retains both zone owners");t.releaseAll();
  for(int i=0;i<2000;i++){t.onTouchEvent(e(0,34,120,300));t.onTouchEvent(e(2,34,150,330));t.onTouchEvent(e(1,34,150,330));}
  ok(lx==0 && ly==0 && !t.anyFingerDown(),"2000 hidden movement gestures finish without stuck input");
  reset();t=new TouchTest();t.moveStyle=1;t.controls[0].area=new RectF(100,200,200,400);t.setShown(false);
  t.onTouchEvent(e(0,27,150,300));ok(buttons==0&&t.move.pointer==27,"first wake touch cannot activate invisible action button");t.releaseAll();
  t.timeout=0;t.scheduled=-1;t.onTouchEvent(e(0,28,900,300));t.onTouchEvent(e(1,28,900,300));ok(t.scheduled==-1,"Never timeout schedules no hide");
  t.timeout=12000;t.onTouchEvent(e(0,29,900,300));t.onTouchEvent(e(1,29,900,300));ok(t.scheduled==12000,"custom timeout applied");
  System.out.println("Touch checks passed: "+checks);
 }
}'''
java_home=Path(os.environ.get('JAVA_HOME',str(repo.parent/'toolchain/host/usr/lib/jvm/java-17-openjdk-amd64')))
with tempfile.TemporaryDirectory() as tmp:
 path=Path(tmp)/'TouchTest.java';path.write_text(code)
 subprocess.run([str(java_home/'bin/javac'),str(path)],check=True)
 subprocess.run([str(java_home/'bin/java'),'-cp',tmp,'TouchTest'],check=True)
