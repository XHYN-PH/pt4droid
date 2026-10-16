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
    private CameraControls cameraControls;
    private PhoneHaptics phoneHaptics;
    private boolean hapticsActive;
    static native void nativeCameraAction(int action);
    static native void nativeCameraRise(float amount);
    public static void phoneRumble(int low,int high){final PTActivity a=current;if(a!=null)a.runOnUiThread(()->{if(a.phoneHaptics!=null&&a.hapticsActive)a.phoneHaptics.rumble(low,high);});}
    public static void cameraState(int state){final PTActivity a=current;if(a!=null)a.runOnUiThread(()->{if(a.cameraControls!=null)a.cameraControls.state(state);});}
    static void touchOverlayShown(boolean shown){final PTActivity a=current;if(a!=null&&a.cameraControls!=null)a.cameraControls.allowed(shown);}
    static void cameraToolbarBounds(int left,int top){final PTActivity a=current;if(a==null||a.cameraControls==null)return;android.widget.FrameLayout.LayoutParams lp=(android.widget.FrameLayout.LayoutParams)a.cameraControls.getLayoutParams();if(lp.leftMargin!=left||lp.topMargin!=top){lp.leftMargin=left;lp.topMargin=top;a.cameraControls.setLayoutParams(lp);}}
    static void cameraToolbarStyle(float opacity){final PTActivity a=current;if(a!=null&&a.cameraControls!=null)a.cameraControls.appearance(opacity);}
    public static void photoSaved(String path){final PTActivity a=current;if(a!=null)PhotoStore.publish(a,path);}
    private TouchControls touchControls;
    private static volatile PTActivity current;
    private MobileSettings mobileSettings;
    private MobileRuntime mobileRuntime;
    private GameSession gameSession;
    private boolean returningToMenu;
    private boolean menuOpened;
    private android.widget.LinearLayout loadingPanel;
    private android.widget.TextView loadingText;
    private android.widget.ProgressBar loadingBar;
    public static void loadingProgress(String label,int completed,int total){
        final PTActivity a=current;if(a==null)return;
        a.runOnUiThread(()->{
            if(a.isFinishing())return;
            if(label.isEmpty()){if(a.loadingPanel!=null)a.loadingPanel.setVisibility(android.view.View.GONE);return;}
            if(a.loadingPanel==null){
                a.loadingPanel=MobileSettings.column(a);a.loadingPanel.setPadding(MenuStyle.dp(a,12),MenuStyle.dp(a,6),MenuStyle.dp(a,12),MenuStyle.dp(a,6));
                a.loadingText=new android.widget.TextView(a);a.loadingText.setTextColor(MenuStyle.TEXT);a.loadingPanel.addView(a.loadingText);
                a.loadingBar=new android.widget.ProgressBar(a,null,android.R.attr.progressBarStyleHorizontal);a.loadingBar.setMax(100);a.loadingPanel.addView(a.loadingBar);
                android.widget.FrameLayout.LayoutParams lp=new android.widget.FrameLayout.LayoutParams(MenuStyle.dp(a,280),-2,android.view.Gravity.BOTTOM|android.view.Gravity.CENTER_HORIZONTAL);lp.bottomMargin=MenuStyle.dp(a,12);
                a.addContentView(a.loadingPanel,lp);
            }
            a.loadingPanel.setVisibility(android.view.View.VISIBLE);
            a.loadingText.setText(total>0?label+"  "+Math.min(100,Math.max(0,completed)*100/total)+"%":label+"…");
            a.loadingBar.setIndeterminate(total<=0);if(total>0)a.loadingBar.setProgress(Math.min(100,Math.max(0,completed)*100/total));
        });
    }
    public static void graphicsStartupFailed(String message){
        final PTActivity a=current;if(a==null)return;
        try{DriverImportJob.atomicWrite(new File(a.getFilesDir(),"driver_startup_failed.txt"),message.getBytes(java.nio.charset.StandardCharsets.UTF_8));}
        catch(java.io.IOException error){AppLog.w("pt","Could not save driver recovery",error);}
    }
    static native void nativeReloadPerformance();
    static native void nativeThermalStatus(int status);
    static native void nativeGyroDelta(float dx,float dy);
    static void reloadPerformance(){final PTActivity a=current;if(a!=null){nativeReloadPerformance();if(a.mobileRuntime!=null)a.mobileRuntime.reload();}}
    public static void setGameplayLookEnabled(boolean enabled){final PTActivity a=current;if(a!=null)a.runOnUiThread(()->{if(a.mobileRuntime!=null)a.mobileRuntime.allow(enabled);});}
    static void setTouchLayoutEditing(boolean editing){final PTActivity a=current;if(a!=null&&a.mobileRuntime!=null)a.mobileRuntime.edit(editing);}
    static native float nativeGetRenderScale();
    static native void nativeSetRenderScale(float value);
    static native void nativeSetPerformanceProfile(int value);
    static native void nativeSetBloomPassLimit(int value);
    static native void nativeSetLoggingEnabled(boolean enabled);
    static native String nativeGetGameGraphicsSettings();
    static native void nativeApplyGameGraphicsSettings();
    public static void refreshDisplay(){final PTActivity a=current;if(a!=null)a.runOnUiThread(()->DisplayRefresh.apply(a,mSurface==null?null:mSurface.getHolder().getSurface()));}
    public static void setFrameTarget(int fps){DisplayRefresh.setTarget(fps);refreshDisplay();}
    public static float getDisplayRefreshRate(){return DisplayRefresh.actualRate();}
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
    @Override protected void onResume(){super.onResume();hapticsActive=true;DisplayRefresh.invalidate();refreshDisplay();if(touchControls!=null)touchControls.focusReturned();if(mobileSettings!=null)mobileSettings.resumeImport();if(mobileRuntime!=null)mobileRuntime.resume();}
    @Override protected void onPause(){hapticsActive=false;if(phoneHaptics!=null)phoneHaptics.cancel();if(cameraControls!=null)cameraControls.clear();if(mobileRuntime!=null)mobileRuntime.pause();if(touchControls!=null)touchControls.releaseAll();super.onPause();}
    @Override public void onWindowFocusChanged(boolean focus){super.onWindowFocusChanged(focus);hapticsActive=focus;if(!focus){if(phoneHaptics!=null)phoneHaptics.cancel();if(cameraControls!=null)cameraControls.clear();}if(mobileRuntime!=null)mobileRuntime.focus(focus);if(focus){DisplayRefresh.invalidate();refreshDisplay();}if(touchControls!=null){if(focus)touchControls.focusReturned();else touchControls.releaseAll();}}
    void returnToMenu() {
        if (returningToMenu || isFinishing()) return;
        returningToMenu = true;
        if (touchControls != null) touchControls.releaseAll();
        if (mobileRuntime != null) mobileRuntime.pause();
        DriverImportJob job=DriverImportJob.get();
        if(job!=null && !job.done)job.cancel("Import cancelled before returning to menu");
        // Wake SDL even if a dialog has paused it. SDLMain finishes us after native cleanup.
        SDLActivity.nativeSendQuit();
    }

    @Override public void finish() {
        runOnUiThread(() -> {
            if (mSDLMainFinished && !menuOpened) {
                menuOpened = true;
                startActivity(new Intent(this, SetupActivity.class)
                        .putExtra("returned_from_game", true)
                        .addFlags(Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_CLEAR_TOP));
            }
            PTActivity.super.finish();
        });
    }

    @Override protected void onDestroy(){
        if(mobileRuntime!=null)mobileRuntime.pause();
        if(touchControls!=null)touchControls.releaseAll();
        if(mobileSettings!=null)mobileSettings.detach();
        if(current==this)current=null;
        super.onDestroy();
        if(gameSession!=null)try{gameSession.close();}catch(java.io.IOException error){AppLog.w("pt","Could not release game save guard",error);}
        // SDL and renderer globals cannot safely be reused for another SDL_main run.
        if(isFinishing())android.os.Process.killProcess(android.os.Process.myPid());
    }


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
        phoneHaptics = new PhoneHaptics(this);
        try { gameSession = GameSession.start(this); }
        catch (java.io.IOException error) {
            android.widget.Toast.makeText(this,error.getMessage(),android.widget.Toast.LENGTH_LONG).show();
            finish();
            return;
        }
        DriverImportJob.cleanupAbandoned(this);
        secondScreen = new SecondScreen(this);
        // the on-screen controls over the game (TouchControls.java)
        touchControls = new TouchControls(this);
        touchControls.setMode(getTouchControlsMode());
        mobileSettings = new MobileSettings(this,touchControls);
        mobileRuntime = new MobileRuntime(this);
        addContentView(touchControls, new ViewGroup.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT));
        cameraControls=new CameraControls(this);
        android.widget.FrameLayout.LayoutParams cameraLayout=new android.widget.FrameLayout.LayoutParams(-2,MenuStyle.dp(this,36),android.view.Gravity.TOP|android.view.Gravity.LEFT);cameraLayout.leftMargin=MenuStyle.dp(this,100);cameraLayout.rightMargin=MenuStyle.dp(this,12);
        addContentView(cameraControls,cameraLayout);cameraControls.allowed(touchControls.toolbarVisible());
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
