package com.ptport.app;

import android.os.Bundle;
import android.content.Intent;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.ViewGroup;

import java.io.File;
import java.io.FileWriter;
import java.io.PrintWriter;

import org.libsdl.app.SDLActivity;

/* SDL's activity starts SDL_main in libmain.so (the game, src/main.cpp). The hook libraries and the Turnip driver are not
   loaded here: libadrenotools opens them itself when the game starts Vulkan (src/engine/platform/android.cpp). */
public class PTActivity extends SDLActivity {
    private SecondScreen secondScreen;
    private TouchControls touchControls;
    private static volatile PTActivity current;
    private MobileSettings mobileSettings;
    static native float nativeGetRenderScale();
    static native void nativeSetRenderScale(float value);
    static native void nativeSetPerformanceProfile(int value);
    static native void nativeSetBloomPassLimit(int value);
    static native void nativeSetLoggingEnabled(boolean enabled);
    static native String nativeGetGameGraphicsSettings();
    static native void nativeApplyGameGraphicsSettings();
    public static void refreshDisplay(){final PTActivity a=current;if(a!=null)a.runOnUiThread(()->DisplayRefresh.apply(a,mSurface==null?null:mSurface.getHolder().getSurface()));}
    public static void gameGraphicsSettingsFailed(String message){final PTActivity a=current;if(a!=null)a.runOnUiThread(()->android.widget.Toast.makeText(a,message,android.widget.Toast.LENGTH_LONG).show());}
    static boolean gameRunning() {return current!=null;}
    public static int getTouchControlsMode() {return current==null?0:current.getSharedPreferences("touch_layout_v1",0).getInt("overlay_mode",0);}
    public static void setTouchControlsMode(int mode) {final PTActivity a=current;if(a!=null)a.runOnUiThread(()->{a.getSharedPreferences("touch_layout_v1",0).edit().putInt("overlay_mode",mode).apply();if(a.touchControls!=null)a.touchControls.setMode(mode);});}
    public static void openMobileSettings() {final PTActivity a=current;if(a!=null)a.runOnUiThread(()->{if(a.mobileSettings!=null)a.mobileSettings.show();});}
    public static void gameGraphicsPresetSelected(int preset) {
        final PTActivity a=current;if(a==null)return;
        a.runOnUiThread(()->{
            int profile=preset==1?3:0;
            try {
                DriverImportJob.atomicWrite(new File(a.getFilesDir(),"performance_profile.txt"),(profile+"\n").getBytes(java.nio.charset.StandardCharsets.UTF_8));
                a.getSharedPreferences("touch_layout_v1",0).edit().putInt("performance_profile",profile).apply();
            } catch(java.io.IOException error){android.widget.Toast.makeText(a,"Could not keep preset after restart: "+error.getMessage(),android.widget.Toast.LENGTH_LONG).show();}
        });
    }
    @Override protected void onActivityResult(int request,int result,Intent data){if(mobileSettings!=null && mobileSettings.activityResult(request,result,data))return;super.onActivityResult(request,result,data);}
    @Override protected void onResume(){super.onResume();refreshDisplay();if(touchControls!=null)touchControls.focusReturned();if(mobileSettings!=null)mobileSettings.resumeImport();}
    @Override protected void onPause(){if(touchControls!=null)touchControls.releaseAll();super.onPause();}
    @Override public void onWindowFocusChanged(boolean focus){super.onWindowFocusChanged(focus);if(touchControls!=null){if(focus)touchControls.focusReturned();else touchControls.releaseAll();}}
    @Override protected void onDestroy(){if(touchControls!=null)touchControls.releaseAll();if(mobileSettings!=null)mobileSettings.detach();if(current==this)current=null;super.onDestroy();}


    // from the game (src/engine/platform/android.cpp SetTouchControlsEnabled): the setting, any thread
    @SuppressWarnings("unused")
    public static void setTouchControlsEnabled(boolean enabled) {
        final PTActivity activity = current;
        if (activity == null) return;
        activity.runOnUiThread(() -> {
            if (activity.touchControls != null) activity.touchControls.setAllowed(enabled);
        });
    }

    // from the game (src/engine/platform/android.cpp NoteControllerEvent): a real controller was used, on the game thread
    @SuppressWarnings("unused")
    public static void controllerUsed() {
        final PTActivity activity = current;
        if (activity == null) return;
        activity.runOnUiThread(() -> {
            if (activity.touchControls != null) activity.touchControls.controllerUsed();
        });
    }

    // from the game (src/engine/platform/android.cpp UpdateSecondScreen): the loop and the subtitle line, UTF-8
    @SuppressWarnings("unused")
    public static void updateSecondScreen(int loop, byte[] line) {
        SecondScreen.update(loop, line);
    }

    // from the game: the Second screen setting (0 clock and subtitles, 1 clock, 2 dim clock, 3 off)
    @SuppressWarnings("unused")
    public static void setSecondScreenMode(int mode) {
        SecondScreen.setMode(mode);
    }

    // from the game, building its settings page: whether to offer the Second screen setting
    @SuppressWarnings("unused")
    public static boolean hasSecondScreen() {
        try {
            return SecondScreen.available();
        } catch (Throwable t) {
            return false;
        }
    }

    @Override
    protected String[] getLibraries() {
        return new String[] { "SDL3", "main" };
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        current = this;
        secondScreen = new SecondScreen(this);
        // the on-screen controls over the game (TouchControls.java)
        touchControls = new TouchControls(this);
        touchControls.setMode(getTouchControlsMode());
        mobileSettings = new MobileSettings(this,touchControls);
        addContentView(touchControls, new ViewGroup.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT));
    }

    // a real controller in use puts the touch controls away (a touch brings them back); the event goes on to the game
    @Override
    public boolean dispatchKeyEvent(KeyEvent event) {
        if(touchControls!=null && (event.getKeyCode()==KeyEvent.KEYCODE_BACK || event.getKeyCode()==KeyEvent.KEYCODE_ESCAPE) && touchControls.cancelEditor(event)) return true;
        if (touchControls != null && TouchControls.isController(event.getSource())) touchControls.controllerUsed();
        return super.dispatchKeyEvent(event);
    }

    @Override
    public boolean dispatchGenericMotionEvent(MotionEvent event) {
        // a stick or trigger pushed past half, not the small drift a resting stick reports
        if (touchControls != null && TouchControls.isController(event.getSource()) && event.getAction() == MotionEvent.ACTION_MOVE) {
            for (int axis : new int[] {MotionEvent.AXIS_X, MotionEvent.AXIS_Y, MotionEvent.AXIS_Z, MotionEvent.AXIS_RZ,
                     MotionEvent.AXIS_HAT_X, MotionEvent.AXIS_HAT_Y, MotionEvent.AXIS_LTRIGGER, MotionEvent.AXIS_RTRIGGER}) {
                if (Math.abs(event.getAxisValue(axis)) > 0.5f) {
                    touchControls.controllerUsed();
                    break;
                }
            }
        }
        return super.dispatchGenericMotionEvent(event);
    }

    // shown while the game is visible (start/stop, not resume/pause: a second-display activity can pause this one without
    // hiding it, which would take the clock down and up again)
    @Override
    protected void onStart() {
        super.onStart();
        if (secondScreen != null) {
            try {
                secondScreen.resume();
            } catch (Throwable t) {
                AppLog.w("pt", "second screen: " + t);
            }
        }
    }

    @Override
    protected void onStop() {
        if (secondScreen != null) {
            try {
                secondScreen.pause();
            } catch (Throwable t) {
                AppLog.w("pt", "second screen: " + t);
            }
        }
        super.onStop();
    }

}
