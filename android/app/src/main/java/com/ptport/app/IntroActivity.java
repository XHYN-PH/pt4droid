package com.ptport.app;

import android.app.Activity;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.view.*;
import android.widget.*;

/** Local teaser; skip or decoding failure always leads to the launcher. */
public final class IntroActivity extends Activity {
    private VideoView video;
    private boolean finished,resumed,advanceOnResume;
    private int position;
    @Override public void onCreate(Bundle state){
        super.onCreate(state);
        if(state!=null)position=state.getInt("position",0);
        getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN,WindowManager.LayoutParams.FLAG_FULLSCREEN);
        FrameLayout root=new FrameLayout(this);root.setBackgroundColor(android.graphics.Color.BLACK);
        video=new VideoView(this);FrameLayout.LayoutParams movie=new FrameLayout.LayoutParams(-1,-1,Gravity.CENTER);root.addView(video,movie);
        Button skip=new Button(this);skip.setText("Skip");MenuStyle.button(skip);skip.setOnClickListener(v->leave());
        FrameLayout.LayoutParams button=new FrameLayout.LayoutParams(-2,MenuStyle.dp(this,48),Gravity.BOTTOM|Gravity.RIGHT);button.setMargins(16,16,16,16);root.addView(skip,button);setContentView(root);
        video.setOnCompletionListener(v->complete());video.setOnErrorListener((v,what,extra)->{complete();return true;});
        video.setOnPreparedListener(v->{if(position>0)video.seekTo(position);if(resumed&&!finished)video.start();});
        video.setVideoURI(Uri.parse("android.resource://"+getPackageName()+"/"+R.raw.launcher_intro));
        skip.requestFocus();
    }
    private void complete(){if(resumed)leave();else advanceOnResume=true;}
    private void leave(){if(finished)return;finished=true;if(video!=null)video.stopPlayback();if(!getIntent().getBooleanExtra("replay",false))startActivity(new Intent(this,SetupActivity.class));finish();}
    @Override public void onBackPressed(){leave();}
    @Override public boolean onKeyUp(int key,KeyEvent event){if(key==KeyEvent.KEYCODE_BUTTON_START||key==KeyEvent.KEYCODE_BUTTON_B){leave();return true;}return super.onKeyUp(key,event);}
    @Override protected void onPause(){resumed=false;if(video!=null){position=video.getCurrentPosition();video.pause();}super.onPause();}
    @Override protected void onResume(){super.onResume();resumed=true;if(advanceOnResume){leave();return;}if(video!=null&&!finished){video.seekTo(position);video.start();}}
    @Override protected void onSaveInstanceState(Bundle state){state.putInt("position",video==null?position:video.getCurrentPosition());super.onSaveInstanceState(state);}
    @Override protected void onDestroy(){if(video!=null)video.stopPlayback();super.onDestroy();}
}
