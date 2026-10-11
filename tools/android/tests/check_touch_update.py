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
methods='\n'.join(method(s) for s in ['public boolean onTouchEvent(', 'private void updateLook(', 'private void press(', 'private void release(int i)', 'private float[] axes(', 'private void send()', 'private boolean anyFingerDown()', 'static boolean isFingerEvent(', 'private void setShown(', 'void releaseAll()'])
code=r'''import java.util.*;
public class TouchTest {
 static int checks;static float dx,dy,rx,ry,lx,ly;static int buttons;static boolean visible;
 static void ok(boolean b,String why){if(!b)throw new AssertionError(why);checks++;}
 static class RectF{float left,top,right,bottom;RectF(float a,float b,float c,float d){left=a;top=b;right=c;bottom=d;}float centerX(){return(left+right)/2;}float centerY(){return(top+bottom)/2;}float width(){return right-left;}float height(){return bottom-top;}boolean contains(float x,float y){return x>=left&&x<right&&y>=top&&y<bottom;}}
 static class Control{int action;boolean visible=true;float size=1;RectF area=new RectF(-200,-200,-100,-100);HashSet<Integer> fingers=new HashSet<>();}
 static class Stick{int pointer=-1;float x,y,originX,originY;}
 static class MotionEvent{
  static final int ACTION_DOWN=0,ACTION_UP=1,ACTION_MOVE=2,ACTION_CANCEL=3,ACTION_POINTER_DOWN=5,ACTION_POINTER_UP=6;
  int action,index,source=4098,tool=1;int[] ids;float[] x,y;
  MotionEvent(int a,int i,int[] ids,float[] x,float[] y){action=a;index=i;this.ids=ids;this.x=x;this.y=y;}
  int getActionMasked(){return action;}int getActionIndex(){return index;}int getPointerCount(){return ids.length;}int getPointerId(int i){return ids[i];}float getX(){return x[0];}float getY(){return y[0];}float getX(int i){return x[i];}float getY(int i){return y[i];}int getToolType(int i){return tool;}boolean isFromSource(int s){return(source&s)==s;}
 }
 static class PTActivity{static void openMobileSettings(){}}
 Control[] controls=new Control[7];Stick move=new Stick(),look=new Stick();RectF safe=new RectF(0,0,1200,600),settingsHit=new RectF(-100,-100,-50,-50);
 int mode=0,lookStyle=1,lastButtons=-1;float sensitivity=1,deadZone=.05f,lastLx,lastLy,lastRx,lastRy,lookLastX,lookLastY;
 boolean floating=true,shown=true,editing,editorSaw;Runnable hideIdle=()->{};static final long HIDE_AFTER_MS=5000;static final int[] ACTION_BITS={1,8,2,4,16};
 TouchTest(){for(int i=0;i<7;i++){controls[i]=new Control();controls[i].action=i<5?i:0;}}
 float dp(float value){return value;}float radius(int i){return 56*controls[i].size;}static float clamp(float v,float a,float b){return Math.max(a,Math.min(b,v));}
 void removeCallbacks(Runnable r){}void postDelayed(Runnable r,long time){}void invalidate(){}void postInvalidate(){}boolean editEvent(MotionEvent e){editorSaw=true;return true;}
 static void nativeLookDelta(float x,float y){dx+=x;dy+=y;}static void nativeClearLook(){dx=dy=0;}static void nativeVisible(boolean value){visible=value;}
 static void nativeState(float a,float b,float c,float d,int bits){lx=a;ly=b;rx=c;ry=d;buttons=bits;}
''' + methods + r'''
 static MotionEvent e(int action,int id,float x,float y){return new MotionEvent(action,0,new int[]{id},new float[]{x},new float[]{y});}
 static MotionEvent multi(int action,int index,int[] ids,float[] x,float[] y){return new MotionEvent(action,index,ids,x,y);}
 static void reset(){dx=dy=rx=ry=lx=ly=0;buttons=0;}
 public static void main(String[] args){
  reset();TouchTest t=new TouchTest();t.floating=false;t.controls[6].visible=false;t.onTouchEvent(e(0,1,900,300));t.onTouchEvent(e(2,1,960,330));ok(Math.abs(dx-72)<.001 && Math.abs(dy-36)<.001,"invisible fixed swipe");ok(rx==0 && ry==0,"swipe sends no joystick rotation");float before=dx;t.onTouchEvent(e(2,1,960,330));ok(dx==before,"stationary finger does not turn");t.onTouchEvent(e(1,1,980,330));ok(Math.abs(dx-96)<.001 && t.look.pointer<0,"final lift included");
  reset();t=new TouchTest();t.onTouchEvent(e(0,1,100,100));t.onTouchEvent(multi(5,1,new int[]{1,2},new float[]{100,900},new float[]{100,300}));t.onTouchEvent(multi(2,0,new int[]{1,2},new float[]{150,950},new float[]{150,300}));ok(lx>0 && ly>0 && dx>0,"walking and looking together");ok(Math.sqrt(lx*lx+ly*ly)<=1.0001,"diagonal stick bounded");MotionEvent cancel=e(3,2,0,0);cancel.tool=0;cancel.source=0;t.onTouchEvent(cancel);ok(lx==0 && ly==0 && dx==0 && t.look.pointer<0 && t.move.pointer<0,"unknown tool cancellation releases controls");
  reset();t=new TouchTest();t.controls[0].area=new RectF(800,200,1000,400);t.onTouchEvent(e(0,4,900,300));ok(buttons==1 && t.look.pointer<0,"buttons take priority");t.onTouchEvent(multi(5,1,new int[]{4,5},new float[]{900,910},new float[]{300,310}));t.onTouchEvent(multi(6,0,new int[]{4,5},new float[]{900,910},new float[]{300,310}));ok(buttons==1,"one of two fingers retains button");t.onTouchEvent(e(1,5,910,310));ok(buttons==0,"last finger releases button");t.controls[0].action=4;t.onTouchEvent(e(0,6,900,300));ok(buttons==16,"button remapping");t.releaseAll();
  reset();t=new TouchTest();MotionEvent mouse=e(0,1,900,300);mouse.source=8194;mouse.tool=3;ok(!t.onTouchEvent(mouse) && dx==0,"physical mouse passes through");t.mode=1;ok(!t.onTouchEvent(e(0,1,900,300)),"Off passes through");
  reset();t=new TouchTest();t.lookStyle=2;t.floating=false;t.controls[6].visible=false;t.onTouchEvent(e(0,7,900,300));t.onTouchEvent(e(2,7,930,300));ok(rx>0 && dx==0,"hidden joystick continuous axis");t.onTouchEvent(e(1,7,930,300));ok(rx==0,"hidden joystick release");
  reset();t=new TouchTest();t.safe=new RectF(0,0,2400,1200);t.onTouchEvent(e(0,8,1800,600));t.onTouchEvent(e(2,8,1920,660));ok(Math.abs(dx-72)<.001 && Math.abs(dy-36)<.001,"sensitivity follows screen size");t.releaseAll();t.editing=true;t.onTouchEvent(e(0,9,1900,600));ok(t.editorSaw && dx==0,"editor owns input");t.editing=false;t.setShown(false);ok(!visible,"hidden overlay visibility cleared");
  System.out.println("Touch checks passed: "+checks);
 }
}'''
java_home=Path(os.environ.get('JAVA_HOME',str(repo.parent/'toolchain/host/usr/lib/jvm/java-17-openjdk-amd64')))
with tempfile.TemporaryDirectory() as tmp:
 path=Path(tmp)/'TouchTest.java';path.write_text(code)
 subprocess.run([str(java_home/'bin/javac'),str(path)],check=True)
 subprocess.run([str(java_home/'bin/java'),'-cp',tmp,'TouchTest'],check=True)
