package com.ptport.app;

import android.app.Activity;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.os.SystemClock;
import android.view.*;
import android.widget.*;

/** Local teaser; skip or decoding failure always leads to the launcher. */
public final class IntroActivity extends Activity {
    private static final long HOLD_MS=3000;
    private final Handler handler=new Handler(Looper.getMainLooper());
    private TextView skip;
    private int holdKey=-1,holdPointer=-1;
    private long holdStarted=-1;
    private final Runnable holdTick=new Runnable(){public void run(){
        if(holdStarted<0||!resumed||finished)return;
        long remaining=HOLD_MS-(SystemClock.uptimeMillis()-holdStarted);
        if(remaining<=0){leave();return;}
        skip.setText("Hold to skip · "+((remaining+999)/1000));handler.postDelayed(this,Math.min(1000,remaining));
    }};
    private VideoView video;
    private boolean finished,resumed,advanceOnResume;
    private int position;
    @Override public void onCreate(Bundle state){
        super.onCreate(state);
        if(state!=null)position=state.getInt("position",0);
        getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN,WindowManager.LayoutParams.FLAG_FULLSCREEN);
        FrameLayout root=new FrameLayout(this);root.setBackgroundColor(android.graphics.Color.BLACK);
        video=new VideoView(this);FrameLayout.LayoutParams movie=new FrameLayout.LayoutParams(-1,-1,Gravity.CENTER);root.addView(video,movie);
        skip=new TextView(this);skip.setText("Hold to skip");skip.setTextColor(0xffe7e4db);skip.setTextSize(12);
        skip.setGravity(Gravity.CENTER);skip.setBackground(null);skip.setFocusable(true);skip.setClickable(true);
        skip.setPadding(MenuStyle.dp(this,12),0,MenuStyle.dp(this,12),0);skip.setShadowLayer(3,0,1,0xff000000);
        skip.setContentDescription("Hold for three seconds to skip the teaser");
        skip.setOnClickListener(v->skip.announceForAccessibility("Hold for three seconds to skip"));
        skip.setOnTouchListener((v,e)->{
            int action=e.getActionMasked();
            if(action==MotionEvent.ACTION_DOWN){holdPointer=e.getPointerId(0);beginHold();}
            else if(action==MotionEvent.ACTION_MOVE){int i=e.findPointerIndex(holdPointer);if(i<0||e.getX(i)<0||e.getY(i)<0||e.getX(i)>v.getWidth()||e.getY(i)>v.getHeight())cancelHold();}
            else if(action==MotionEvent.ACTION_UP){cancelHold();v.performClick();}
            else if(action==MotionEvent.ACTION_CANCEL||(action==MotionEvent.ACTION_POINTER_UP&&e.getPointerId(e.getActionIndex())==holdPointer))cancelHold();
            return true;
        });
        FrameLayout.LayoutParams button=new FrameLayout.LayoutParams(-2,MenuStyle.dp(this,48),Gravity.BOTTOM|Gravity.RIGHT);int margin=MenuStyle.dp(this,12);button.setMargins(margin,margin,margin,margin);root.addView(skip,button);setContentView(root);
        video.setOnCompletionListener(v->complete());video.setOnErrorListener((v,what,extra)->{complete();return true;});
        video.setOnPreparedListener(v->{if(position>0)video.seekTo(position);if(resumed&&!finished)video.start();});
        video.setVideoURI(Uri.parse("android.resource://"+getPackageName()+"/"+R.raw.launcher_intro));
        skip.requestFocus();
    }
    private void beginHold(){if(holdStarted>=0||!resumed||finished)return;holdStarted=SystemClock.uptimeMillis();holdTick.run();}
    private void cancelHold(){handler.removeCallbacks(holdTick);holdStarted=-1;holdKey=-1;holdPointer=-1;if(skip!=null)skip.setText("Hold to skip");}
    private boolean skipKey(int key){return key==KeyEvent.KEYCODE_BUTTON_A||key==KeyEvent.KEYCODE_BUTTON_START||key==KeyEvent.KEYCODE_BUTTON_B||key==KeyEvent.KEYCODE_DPAD_CENTER||key==KeyEvent.KEYCODE_ENTER||key==KeyEvent.KEYCODE_SPACE||key==KeyEvent.KEYCODE_BACK;}
    // Handle activation before the focused TextView can consume Enter/DPAD/A.
    @Override public boolean dispatchKeyEvent(KeyEvent event){
        int key=event.getKeyCode();if(!skipKey(key))return super.dispatchKeyEvent(event);
        if(event.getAction()==KeyEvent.ACTION_DOWN)return onKeyDown(key,event);
        if(event.getAction()==KeyEvent.ACTION_UP)return onKeyUp(key,event);
        return true;
    }
    @Override public boolean onKeyDown(int key,KeyEvent event){if(skipKey(key)){if(event.getRepeatCount()==0&&holdStarted<0){holdKey=key;beginHold();}return true;}return super.onKeyDown(key,event);}
    @Override public void onWindowFocusChanged(boolean focus){super.onWindowFocusChanged(focus);if(!focus)cancelHold();}
    private void complete(){if(resumed)leave();else advanceOnResume=true;}
    private void leave(){if(finished)return;finished=true;cancelHold();if(video!=null)video.stopPlayback();if(!getIntent().getBooleanExtra("replay",false))startActivity(new Intent(this,SetupActivity.class).putExtra("intro_played",true));finish();}
    @Override public void onBackPressed(){cancelHold();}
    @Override public boolean onKeyUp(int key,KeyEvent event){if(skipKey(key)){if(key==holdKey)cancelHold();return true;}return super.onKeyUp(key,event);}
    @Override protected void onPause(){resumed=false;cancelHold();if(video!=null){position=video.getCurrentPosition();video.pause();}super.onPause();}
    @Override protected void onResume(){super.onResume();resumed=true;if(advanceOnResume){leave();return;}if(video!=null&&!finished){video.seekTo(position);video.start();}}
    @Override protected void onSaveInstanceState(Bundle state){state.putInt("position",video==null?position:video.getCurrentPosition());super.onSaveInstanceState(state);}
    @Override protected void onDestroy(){cancelHold();if(video!=null)video.stopPlayback();super.onDestroy();}
}
