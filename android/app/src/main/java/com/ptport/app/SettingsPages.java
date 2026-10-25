package com.ptport.app;

import android.Manifest;
import android.app.*;
import android.content.*;
import android.content.pm.PackageManager;
import android.os.PowerManager;
import android.widget.*;
import java.io.*;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.util.*;

/** Each page reads and writes only the settings it displays. */
final class SettingsPages {
    private final Activity activity;
    private final TouchControls touch;
    private final Runnable back;
    private final SharedPreferences prefs;
    private LinearLayout column;
    private Runnable resolutionChanged;
    private final Map<String,Object> values=new LinkedHashMap<>();
    SettingsPages(Activity activity,TouchControls touch,Runnable back){this.activity=activity;this.touch=touch;this.back=back;prefs=activity.getSharedPreferences("touch_layout_v1",0);}
    private void note(String label){TextView t=new TextView(activity);t.setText(label);t.setTextColor(MenuStyle.MUTED);column.addView(t);}
    private void button(String title,Runnable action){Button b=new Button(activity);b.setText(title);MenuStyle.button(b);b.setOnClickListener(v->action.run());column.addView(b);}
    private void choice(String title,String key,String[] labels,int fallback){
        note(title);Spinner s=new Spinner(activity);s.setAdapter(new ArrayAdapter<>(activity,android.R.layout.simple_spinner_dropdown_item,labels));s.setSelection(Math.max(0,Math.min(labels.length-1,prefs.getInt(key,fallback))));column.addView(s);
        values.put(key,s.getSelectedItemPosition());s.setOnItemSelectedListener(new android.widget.AdapterView.OnItemSelectedListener(){public void onNothingSelected(AdapterView<?> p){}public void onItemSelected(AdapterView<?> p,android.view.View v,int position,long id){values.put(key,position);if(resolutionChanged!=null)resolutionChanged.run();}});
    }
    private CheckBox check(String title,String key,boolean fallback){CheckBox c=new CheckBox(activity);c.setText(title);c.setTextColor(MenuStyle.TEXT);c.setButtonTintList(MenuStyle.accent());c.setChecked(prefs.getBoolean(key,fallback));values.put(key,c.isChecked());c.setOnCheckedChangeListener((v,on)->values.put(key,on));column.addView(c);return c;}
    private void gain(String title,String key,float fallback,float maximum){
        float n=prefs.getFloat(key,fallback);if(!Float.isFinite(n))n=fallback;n=Math.max(.25f,Math.min(maximum,n));values.put(key,n);
        TextView label=new TextView(activity);label.setTextColor(MenuStyle.TEXT);column.addView(label);label.setText(String.format(Locale.ROOT,"%s: %.2f×",title,n));
        MobileSettings.slider(column,title,Math.round((n-.25f)*100),Math.round((maximum-.25f)*100),p->{float value=.25f+p/100f;values.put(key,value);label.setText(String.format(Locale.ROOT,"%s: %.2f×",title,value));});
    }
    void show(boolean advanced){
        if(touch!=null)touch.releaseAll();resolutionChanged=null;column=MobileSettings.column(activity);
        final int originalProfile=prefs.getInt("performance_profile",0);
        final float[] scale={readScale()};
        if(advanced){
            choice("Graphics preset","performance_profile",new String[]{"Custom • keep game settings","Maximum performance","Balanced • up to 60 FPS","Original (PS4)"},0);
            note("Changing preset can change graphics and the FPS limit. Other settings preserve your selected limit.");
            TextView caption=new TextView(activity);caption.setTextColor(MenuStyle.TEXT);column.addView(caption);caption.setText("Render scale: "+Math.round(scale[0]*100)+"%");
            MobileSettings.slider(column,"Scene resolution",Math.round(scale[0]*100)-25,75,p->{scale[0]=(p+25)/100f;caption.setText(resolutionCaption(scale[0]));});
            choice("Scene output resolution","output_scale_index",new String[]{"100% • full detail","90% • sharper performance","80% • maximum performance"},0);
            resolutionChanged=()->caption.setText(resolutionCaption(scale[0]));resolutionChanged.run();
            note("Output resolution reduces scene and post-processing detail while keeping menus and touch controls at display resolution. Multiplies Scene resolution; for example, 50% scene × 90% output gives about 45% of display resolution.");
            check("Optimized mobile bloom","optimized_bloom",true);
            check("Android frame pacing","frame_pacing",false);
            note("May smooth frame delivery. Compare On and Off at the same FPS limit; leave it Off if motion feels less responsive.");
            check("Android performance hints","adpf",true);
            check("Lens flares (lamp glare)","lens_flares",true);
            choice("Lamp halo strength","flare_halo_index",new String[]{"Subtle","Balanced","Original"},1);
            note("Controls lamp glare only. Room lighting and shadows stay active; Lens flare ghosts is a separate Game graphics option.");
            check("Full-screen pass optimizations (experimental)","post_optimizations",false);
            check("Parallel geometry preparation (experimental)","parallel_preparation",false);
            CheckBox temporal=check("SGSR2 temporal upscaling (experimental)","temporal_upscaling",false);
            CheckBox dynamic=check("Dynamic resolution (experimental)","dynamic_resolution",false);
            temporal.setOnCheckedChangeListener((v,on)->{values.put("temporal_upscaling",on);if(!on)dynamic.setChecked(false);});
            dynamic.setOnCheckedChangeListener((v,on)->{values.put("dynamic_resolution",on);if(on)temporal.setChecked(true);});
            if(dynamic.isChecked()&&!temporal.isChecked())temporal.setChecked(true);
            note("Turning SGSR2 off also turns Dynamic resolution off. Enabling Dynamic resolution enables SGSR2.");
            choice("Dynamic resolution floor","dynamic_floor_index",new String[]{"50% • balanced floor","60% • sharper floor","67% • sharper floor"},0);
            note("SGSR2 and dynamic resolution are independent of the graphics preset. Dynamic resolution uses SGSR2 and the selected minimum scene scale; a lower manually chosen scale is preserved.");
            check("SGSR1 sharper upscaling","sgsr",false);
            note("SGSR1 adds GPU work at reduced scene resolution. The game's FSR options are available from the launcher.");
            choice("Turnip rendering profile • restart required","turnip_profile",new String[]{"Driver default","Prefer system memory","Prefer tile memory","Tune small passes","Tile memory + small passes"},0);
            note("Turnip profiles are independent of the graphics preset. Compare the same scene after fully closing and reopening the game; the fastest profile depends on the GPU and driver.");
            gain("Upscaling sharpness","sharpness",1f,2f);
            choice("Ambient occlusion","ao_quality",new String[]{"Original quality","Performance • fewer samples"},0);
            choice("Screen reflections","reflection_quality",new String[]{"Original quality","Performance • fewer steps"},0);
            int saved=prefs.getInt("compat_texture_size",0);
            // The choice stores an index temporarily; publish the pixel size only when applying.
            String sizeKey="compat_texture_index";
            note("Compatibility texture size");Spinner sizes=new Spinner(activity);sizes.setAdapter(new ArrayAdapter<>(activity,android.R.layout.simple_spinner_dropdown_item,new String[]{"Original detail","2048 px","1024 px","512 px"}));sizes.setSelection(saved==2048?1:saved==1024?2:saved==512?3:0);column.addView(sizes);
            values.put(sizeKey,sizes.getSelectedItemPosition());sizes.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener(){public void onNothingSelected(AdapterView<?> p){}public void onItemSelected(AdapterView<?> p,android.view.View v,int i,long id){values.put(sizeKey,i);}});
            note("Only affects formats the driver cannot sample. Changing texture compatibility or Mali lighting requires restarting the game.");
            CheckBox sustained=check("Sustained performance","sustained",false);PowerManager power=(PowerManager)activity.getSystemService(Context.POWER_SERVICE);sustained.setEnabled(power!=null&&power.isSustainedPerformanceModeSupported());if(!sustained.isEnabled())sustained.setChecked(false);
            check("Faster Mali lighting • restart required","mali_optimizations",true);
            check("Mali safe lighting • restart required","mali_safe_lighting",true);
            note("Mali safe lighting uses an explicit filtered probe lookup and guards invalid lighting values. Other GPU families retain their existing path.");
            MenuStyle.section(column,"Diagnostics");check("Enable logs","logs_enabled",AppLog.enabled());
            check("Verbose logs • for troubleshooting","verbose_logs",false);
            note("Normal logs keep performance summaries, warnings and errors. Enable verbose logs only when more detail is needed.");button("Export diagnostics",()->Diagnostics.share(activity));
        }else{
            choice("Movement control","move_style",new String[]{"Visible joystick","Invisible left-half movement"},0);
            choice("Look control","look_style",new String[]{"Visible joystick","Swipe to look • no button","Hidden joystick"},1);
            note("Drag on the left to move and on the right to look. Action buttons take priority.");
            gain("Look sensitivity","sensitivity",1f,3f);
            choice("Touch controls","overlay_mode",new String[]{"Auto","Off","Always show"},0);
            int timeout=Math.max(0,Math.min(60000,prefs.getInt("buttons_timeout_ms",5000)));
            TextView caption=new TextView(activity);caption.setTextColor(MenuStyle.TEXT);column.addView(caption);caption.setText(timeout==0?"Button timeout: Never":"Button timeout: "+timeout/1000+" s");values.put("buttons_timeout_ms",timeout);
            MobileSettings.slider(column,"Auto-hide delay • 0 = Never",timeout/1000,60,p->{values.put("buttons_timeout_ms",p*1000);caption.setText(p==0?"Button timeout: Never":"Button timeout: "+p+" s");});
            note("The timeout applies in Auto mode. Touching the screen shows the controls again.");
            check("Invert vertical look","invert_y",false);gain("Mouse sensitivity","mouse_sensitivity",1f,3f);gain("Controller sensitivity","controller_sensitivity",1f,3f);
            CheckBox gyro=check("Gyroscope look","gyro",false);gyro.setEnabled(MobileRuntime.hasGyro(activity));if(!gyro.isEnabled())gyro.setChecked(false);gain("Gyroscope sensitivity","gyro_sensitivity",1f,3f);
            CheckBox vibration=check("Phone vibration for game rumble","phone_vibration",true);PhoneHaptics haptics=new PhoneHaptics(activity);vibration.setEnabled(haptics.available());
            int strength=Math.max(0,Math.min(100,prefs.getInt("phone_vibration_strength",100)));values.put("phone_vibration_strength",strength);
            TextView strengthLabel=new TextView(activity);strengthLabel.setTextColor(MenuStyle.TEXT);strengthLabel.setText("Phone vibration strength: "+strength+"%");column.addView(strengthLabel);
            MobileSettings.slider(column,"Vibration strength • 0 = Off",strength,100,p->{values.put("phone_vibration_strength",p);strengthLabel.setText("Phone vibration strength: "+p+"%");});
            note(haptics.amplitudeControl()?"The game's controller rumble must also be enabled.":"The game's controller rumble must also be enabled. This phone has no amplitude control: strength 0 turns vibration off; nonzero levels use the hardware default.");
            note(activity.checkSelfPermission(Manifest.permission.RECORD_AUDIO)==PackageManager.PERMISSION_GRANTED?"Microphone permission is enabled. The Name touch button is also available for the ending puzzle.":"Microphone permission is off. Use the Name touch button for the ending puzzle, or enable microphone access in Android app settings.");
        }
        AlertDialog dialog=new AlertDialog.Builder(activity).setTitle(advanced?"Performance & advanced":"Controls").setView(MobileSettings.scroll(column)).setPositiveButton("Apply",null).setNegativeButton("Close",null).create();
        if(!advanced)button("Customize touch layout",()->{dialog.dismiss();if(touch!=null)touch.beginEdit();else Toast.makeText(activity,"Start the game to customize the touch layout.",Toast.LENGTH_LONG).show();});
        button("Back to settings",()->{dialog.dismiss();back.run();});
        dialog.setOnShowListener(d->dialog.getButton(AlertDialog.BUTTON_POSITIVE).setOnClickListener(v->{try{apply(advanced,originalProfile,scale[0]);dialog.dismiss();}catch(Exception error){Toast.makeText(activity,"Could not apply settings: "+error.getMessage(),Toast.LENGTH_LONG).show();}}));dialog.show();MenuStyle.styleDialog(dialog);
    }
    private String resolutionCaption(float scale){
        int index=(Integer)values.getOrDefault("output_scale_index",0);
        float output=new float[]{1f,.9f,.8f}[Math.max(0,Math.min(2,index))];
        return String.format(Locale.ROOT,"Render scale: %.0f%% · Output: %.0f%%\nCombined scene scale: about %.0f%% of display resolution",scale*100,output*100,scale*output*100);
    }
    private float readScale(){
        if(PTActivity.gameRunning())try{return PTActivity.nativeGetRenderScale();}catch(UnsatisfiedLinkError ignored){}
        try{float f=Float.parseFloat(new String(Files.readAllBytes(new File(activity.getFilesDir(),"render_scale.txt").toPath()),StandardCharsets.UTF_8).trim());if(Float.isFinite(f))return Math.max(.25f,Math.min(1f,f));}catch(Exception ignored){}return .667f;
    }
    private void apply(boolean advanced,int originalProfile,float scale)throws IOException{
        Map<File,byte[]> files=new LinkedHashMap<>();Map<String,Object> saved=new LinkedHashMap<>(values);
        Map<String,String> performance=new LinkedHashMap<>();File ini=new File(activity.getFilesDir(),"mobile_performance.ini");
        if(ini.exists())for(String line:Files.readAllLines(ini.toPath(),StandardCharsets.UTF_8)){int eq=line.indexOf('=');if(eq>0)performance.put(line.substring(0,eq).trim(),line.substring(eq+1).trim());}
        String[] keys=advanced?new String[]{"mali_optimizations","mali_safe_lighting","compat_texture_size","frame_pacing","sgsr","sharpness","ao_quality","reflection_quality","sustained","lens_flares","post_optimizations","adpf","parallel_preparation","temporal_upscaling","dynamic_resolution","turnip_profile","dynamic_floor","output_scale","flare_halo","verbose_logs"}:new String[]{"invert_y","mouse_sensitivity","controller_sensitivity"};
        if(advanced){saved.put("compat_texture_size",new int[]{0,2048,1024,512}[(Integer)saved.remove("compat_texture_index")]);}
        if(advanced){saved.putIfAbsent("verbose_logs",false);saved.putIfAbsent("lens_flares",true);saved.putIfAbsent("adpf",true);for(String k:new String[]{"post_optimizations","parallel_preparation","temporal_upscaling","dynamic_resolution"})saved.putIfAbsent(k,false);if(Boolean.TRUE.equals(saved.get("dynamic_resolution"))&&!Boolean.TRUE.equals(saved.get("temporal_upscaling")))throw new IOException("Dynamic resolution requires SGSR2. Enable SGSR2 or turn Dynamic resolution off.");}
        if(advanced){saved.putIfAbsent("turnip_profile",0);int index=(Integer)saved.getOrDefault("dynamic_floor_index",0);saved.put("dynamic_floor",new float[]{.5f,.6f,.667f}[Math.max(0,Math.min(2,index))]);}
        if(advanced){int output=(Integer)saved.getOrDefault("output_scale_index",0),halo=(Integer)saved.getOrDefault("flare_halo_index",1);saved.put("output_scale",new float[]{1f,.9f,.8f}[Math.max(0,Math.min(2,output))]);saved.put("flare_halo",new float[]{0f,.2f,1f}[Math.max(0,Math.min(2,halo))]);}
        for(String key:keys){Object value=saved.get(key);performance.put(key,value instanceof Boolean?((Boolean)value?"1":"0"):String.valueOf(value));}
        StringBuilder text=new StringBuilder();for(Map.Entry<String,String> e:performance.entrySet())text.append(e.getKey()).append('=').append(e.getValue()).append('\n');files.put(ini,SettingsFiles.text(text.toString()));
        int selected=advanced?(Integer)saved.get("performance_profile"):originalProfile;boolean changed=advanced&&selected!=originalProfile;
        if(advanced){if(changed&&selected==1)scale=Math.min(scale,.5f);files.put(new File(activity.getFilesDir(),"render_scale.txt"),SettingsFiles.text(Float.toString(scale)));files.put(new File(activity.getFilesDir(),"performance_profile.txt"),SettingsFiles.text(Integer.toString(selected)));files.put(new File(activity.getFilesDir(),"bloom_pass_limit.txt"),SettingsFiles.text((Boolean)saved.get("optimized_bloom")?"6":"0"));
            if(changed){if(selected>0){files.put(new File(activity.getFilesDir(),"game_graphics_pending.ini"),null);files.put(new File(activity.getFilesDir(),"game_graphics_applying.ini"),null);}files.put(new File(activity.getFilesDir(),"performance_profile_pending.txt"),SettingsFiles.text(Integer.toString(selected)));}}
        SettingsFiles.commit(files,prefs,saved);
        if(advanced)AppLog.setEnabled((Boolean)saved.get("logs_enabled"));
        if(touch!=null){touch.reloadPreferences();if(!advanced)touch.setMode((Integer)saved.get("overlay_mode"));}
        if(PTActivity.gameRunning()){if(advanced){PTActivity.nativeSetRenderScale(scale);if(changed)PTActivity.nativeSetPerformanceProfile(selected);PTActivity.nativeSetBloomPassLimit((Boolean)saved.get("optimized_bloom")?6:0);}PTActivity.reloadPerformance();}
        DisplayRefresh.apply(activity,null);PTActivity.refreshDisplay();
    }
}
