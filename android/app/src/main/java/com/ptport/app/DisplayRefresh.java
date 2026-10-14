package com.ptport.app;

import android.app.Activity;
import android.os.Build;
import android.view.Display;
import android.view.Surface;
import android.view.WindowManager;
import java.util.ArrayList;
import java.util.List;
import java.util.WeakHashMap;
import java.util.Locale;

/** Request an app-window rate without modifying the device's global display settings. */
public final class DisplayRefresh {
    private DisplayRefresh(){}
    private static final WeakHashMap<Surface,Float> surfaceRates=new WeakHashMap<>();
    private static volatile int runtimeTarget=-1;
    private static volatile float actualRate;
    public static float actualRate(){return actualRate;}
    public static void setTarget(int fps){runtimeTarget=Math.max(0,fps);}
    static String label(Activity activity,int fps){Display d=display(activity);float hz=choose(d.getSupportedModes(),d.getMode(),fps).getRefreshRate();return String.format(Locale.ROOT,"%s / %.0f Hz",fps==0?"Unlimited":fps+" FPS",hz);}
    public static void surfaceCreated(Activity activity,Surface surface){surfaceRates.remove(surface);apply(activity,surface);}
    static Display display(Activity activity){return activity.getWindowManager().getDefaultDisplay();}
    static Display.Mode choose(Display.Mode[] modes,Display.Mode current,float requested){
        Display.Mode selected=null;float difference=Float.MAX_VALUE;
        for(Display.Mode mode:modes){
            if(mode.getPhysicalWidth()!=current.getPhysicalWidth() || mode.getPhysicalHeight()!=current.getPhysicalHeight())continue;
            float rate=mode.getRefreshRate();if(!Float.isFinite(rate) || rate<=0)continue;
            if(requested<=0){if(selected==null || rate>selected.getRefreshRate())selected=mode;}
            else {
                // Prefer exact rate, then the lowest integer multiple. A
                // lower, incompatible mode must not win merely by proximity.
                float multiple=Math.round(rate/requested);
                float score=multiple>=1&&Math.abs(rate-multiple*requested)<.5f?multiple-1:
                    (rate>=requested-.5f?1000+rate-requested:2000+requested-rate);
                if(score<difference){selected=mode;difference=score;}
            }
        }
        return selected==null?current:selected;
    }
    static List<Float> rates(Activity activity){
        Display display=display(activity);Display.Mode current=display.getMode();List<Float> rates=new ArrayList<>();
        for(Display.Mode mode:display.getSupportedModes())if(mode.getPhysicalWidth()==current.getPhysicalWidth() && mode.getPhysicalHeight()==current.getPhysicalHeight()){
            float rate=mode.getRefreshRate();if(Float.isFinite(rate) && rate>0 && !rates.contains(rate))rates.add(rate);
        }
        if(rates.isEmpty())rates.add(current.getRefreshRate());rates.sort(Float::compare);return rates;
    }
    static float maximum(Activity activity){List<Float> rates=rates(activity);return rates.get(rates.size()-1);}
    public static void apply(Activity activity,Surface surface){
        try{
            Display display=display(activity);float requested=runtimeTarget>=0?runtimeTarget:activity.getSharedPreferences("touch_layout_v1",0).getInt("frame_target",0);
            if(!Float.isFinite(requested) || requested<0)requested=0;
            actualRate=display.getRefreshRate();
            Display.Mode selected=choose(display.getSupportedModes(),display.getMode(),requested);float rate=selected.getRefreshRate();
            WindowManager.LayoutParams attributes=activity.getWindow().getAttributes();int modeId=selected.getModeId();
            if(attributes.preferredDisplayModeId!=modeId || attributes.preferredRefreshRate!=rate){attributes.preferredDisplayModeId=modeId;attributes.preferredRefreshRate=rate;activity.getWindow().setAttributes(attributes);}
            if(Build.VERSION.SDK_INT>=30 && surface!=null && surface.isValid()){
                float contentRate=requested>0?requested:rate;
                if(!Float.valueOf(contentRate).equals(surfaceRates.get(surface))){surface.setFrameRate(contentRate,Surface.FRAME_RATE_COMPATIBILITY_DEFAULT);surfaceRates.put(surface,contentRate);}
            }
        }catch(Exception error){AppLog.w("pt","Refresh rate request failed",error);}
    }
}
