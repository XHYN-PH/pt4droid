package com.ptport.app;

import android.content.Context;
import android.os.*;

/** Receives only the existing game's virtual-controller rumble, not puzzle events. */
final class PhoneHaptics {
    private final Context context;
    private final Vibrator vibrator;
    PhoneHaptics(Context context){
        this.context=context;
        if(Build.VERSION.SDK_INT>=31){VibratorManager manager=(VibratorManager)context.getSystemService(Context.VIBRATOR_MANAGER_SERVICE);vibrator=manager==null?null:manager.getDefaultVibrator();}
        else vibrator=(Vibrator)context.getSystemService(Context.VIBRATOR_SERVICE);
    }
    boolean amplitudeControl(){return available()&&vibrator.hasAmplitudeControl();}
    static int amplitude(int low,int high,int strength){int input=Math.max(0,Math.min(65535,Math.max(low,high))),gain=Math.max(0,Math.min(100,strength));return input==0||gain==0?0:Math.max(1,Math.round(input*(255f/65535f)*gain/100f));}
    boolean available(){return vibrator!=null&&vibrator.hasVibrator();}
    void rumble(int low,int high){
        if(!available())return;
        if(!context.getSharedPreferences("touch_layout_v1",0).getBoolean("phone_vibration",true)||Math.max(low,high)<=0){cancel();return;}
        int amplitude=amplitude(low,high,context.getSharedPreferences("touch_layout_v1",0).getInt("phone_vibration_strength",100));
        if(amplitude<=0){cancel();return;}
        try{vibrator.vibrate(VibrationEffect.createOneShot(1000,vibrator.hasAmplitudeControl()?amplitude:VibrationEffect.DEFAULT_AMPLITUDE));}catch(SecurityException ignored){}
    }
    void cancel(){if(vibrator!=null)try{vibrator.cancel();}catch(SecurityException ignored){}}
}
