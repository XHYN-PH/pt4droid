package com.ptport.app;

import android.app.Activity;
import android.os.Build;
import android.view.Display;
import android.view.Surface;
import android.view.WindowManager;
import java.util.ArrayList;
import java.util.List;
import java.util.WeakHashMap;

/** Request an app-window rate without modifying the device's global display settings. */
public final class DisplayRefresh {
    private DisplayRefresh(){}
    private static final WeakHashMap<Surface,Float> surfaceRates=new WeakHashMap<>();
    public static void surfaceCreated(Activity activity,Surface surface){surfaceRates.remove(surface);apply(activity,surface);}
    static Display display(Activity activity){return activity.getWindowManager().getDefaultDisplay();}
    static Display.Mode choose(Display.Mode[] modes,Display.Mode current,float requested){
        Display.Mode selected=null;float difference=Float.MAX_VALUE;
        for(Display.Mode mode:modes){
            if(mode.getPhysicalWidth()!=current.getPhysicalWidth() || mode.getPhysicalHeight()!=current.getPhysicalHeight())continue;
            float rate=mode.getRefreshRate();if(!Float.isFinite(rate) || rate<=0)continue;
            if(requested<=0){if(selected==null || rate>selected.getRefreshRate())selected=mode;}
            else {float d=Math.abs(rate-requested);if(d<difference || (d==difference && selected!=null && rate<selected.getRefreshRate())){selected=mode;difference=d;}}
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
            Display display=display(activity);float requested=activity.getSharedPreferences("touch_layout_v1",0).getFloat("display_refresh",0);
            if(!Float.isFinite(requested) || requested<0)requested=0;
            Display.Mode selected=choose(display.getSupportedModes(),display.getMode(),requested);float rate=selected.getRefreshRate();
            WindowManager.LayoutParams attributes=activity.getWindow().getAttributes();int modeId=Build.VERSION.SDK_INT<30?selected.getModeId():0;
            if(attributes.preferredDisplayModeId!=modeId || attributes.preferredRefreshRate!=rate){attributes.preferredDisplayModeId=modeId;attributes.preferredRefreshRate=rate;activity.getWindow().setAttributes(attributes);}
            if(Build.VERSION.SDK_INT>=30 && surface!=null && surface.isValid()){
                if(!Float.valueOf(rate).equals(surfaceRates.get(surface))){surface.setFrameRate(rate,Surface.FRAME_RATE_COMPATIBILITY_DEFAULT);surfaceRates.put(surface,rate);}
            }
        }catch(Exception error){AppLog.w("pt","Refresh rate request failed",error);}
    }
}
