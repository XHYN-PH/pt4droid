package com.ptport.app;

import android.content.Context;
import android.hardware.Sensor;
import android.hardware.SensorEvent;
import android.hardware.SensorEventListener;
import android.hardware.SensorManager;
import android.hardware.display.DisplayManager;
import android.os.PowerManager;
import android.view.Surface;

/** Sensors run only during focused gameplay. Sustained mode preserves the FPS target. */
final class MobileRuntime implements SensorEventListener {
    private final PTActivity activity;
    private final SensorManager sensors;
    private final Sensor gyro;
    private final PowerManager power;
    private final DisplayManager displays;
    private final DisplayManager.DisplayListener displayListener=new DisplayManager.DisplayListener(){
        @Override public void onDisplayAdded(int id){PTActivity.refreshDisplay();}
        @Override public void onDisplayRemoved(int id){PTActivity.refreshDisplay();}
        @Override public void onDisplayChanged(int id){PTActivity.refreshDisplay();}
    };
    private final PowerManager.OnThermalStatusChangedListener thermal=PTActivity::nativeThermalStatus;
    private boolean resumed,focused=true,allowed,editing,registered,thermalRegistered,displayRegistered;
    private long previous;
    MobileRuntime(PTActivity owner){
        activity=owner;sensors=(SensorManager)owner.getSystemService(Context.SENSOR_SERVICE);
        gyro=sensors==null?null:sensors.getDefaultSensor(Sensor.TYPE_GYROSCOPE);
        power=(PowerManager)owner.getSystemService(Context.POWER_SERVICE);
        displays=(DisplayManager)owner.getSystemService(Context.DISPLAY_SERVICE);
    }
    static boolean hasGyro(Context c){SensorManager s=(SensorManager)c.getSystemService(Context.SENSOR_SERVICE);return s!=null&&s.getDefaultSensor(Sensor.TYPE_GYROSCOPE)!=null;}
    void resume(){resumed=true;if(displays!=null&&!displayRegistered){displays.registerDisplayListener(displayListener,null);displayRegistered=true;}PTActivity.refreshDisplay();reload();}
    void pause(){resumed=false;if(displays!=null&&displayRegistered){displays.unregisterDisplayListener(displayListener);displayRegistered=false;}activity.getWindow().setSustainedPerformanceMode(false);updateSensor();if(power!=null&&thermalRegistered){power.removeThermalStatusListener(thermal);thermalRegistered=false;}}
    void focus(boolean value){focused=value;updateSensor();}
    void allow(boolean value){allowed=value;updateSensor();}
    void edit(boolean value){editing=value;updateSensor();}
    void reload(){
        boolean sustained=activity.getSharedPreferences("touch_layout_v1",0).getBoolean("sustained",false);
        activity.getWindow().setSustainedPerformanceMode(resumed&&sustained&&power!=null&&power.isSustainedPerformanceModeSupported());
        if(power!=null&&resumed&&!thermalRegistered){power.addThermalStatusListener(activity.getMainExecutor(),thermal);thermalRegistered=true;}
        else if(power!=null&&!resumed&&thermalRegistered){power.removeThermalStatusListener(thermal);thermalRegistered=false;}
        PTActivity.nativeThermalStatus(power!=null?power.getCurrentThermalStatus():0);
        updateSensor();
    }
    private void updateSensor(){
        boolean want=resumed&&focused&&allowed&&!editing&&gyro!=null&&activity.getSharedPreferences("touch_layout_v1",0).getBoolean("gyro",false);
        if(want&&!registered)registered=sensors.registerListener(this,gyro,SensorManager.SENSOR_DELAY_GAME);
        else if(!want&&registered){sensors.unregisterListener(this);registered=false;}
        previous=0;
    }
    @Override public void onSensorChanged(SensorEvent e){
        if(!registered||!resumed||!focused||!allowed||editing){previous=0;return;}
        long last=previous;previous=e.timestamp;if(last==0)return;
        float dt=(e.timestamp-last)*1e-9f;if(dt<=0||dt>.05f)return;
        float x=e.values[0],y=e.values[1],sx=x,sy=y;
        switch(activity.getWindowManager().getDefaultDisplay().getRotation()){
            case Surface.ROTATION_90:sx=y;sy=-x;break;
            case Surface.ROTATION_180:sx=-x;sy=-y;break;
            case Surface.ROTATION_270:sx=-y;sy=x;break;
            default:break;
        }
        float gain=activity.getSharedPreferences("touch_layout_v1",0).getFloat("gyro_sensitivity",1f);
        if(!Float.isFinite(gain))gain=1f;
        gain=Math.max(.25f,Math.min(3f,gain));
        // Android's calibrated gyro includes the sensor's bias compensation. Ignore tiny resting noise.
        PTActivity.nativeGyroDelta(Math.abs(sy)<.01f?0:-sy*dt*gain,Math.abs(sx)<.01f?0:-sx*dt*gain);
    }
    @Override public void onAccuracyChanged(Sensor s,int accuracy){}
}
