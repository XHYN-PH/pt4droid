package com.ptport.app;
import android.app.*;
import android.content.*;
import android.os.Handler;
import android.os.Looper;
import android.view.*;
import android.widget.*;
import org.json.JSONObject;
import java.io.*;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.util.*;
import java.util.function.IntConsumer;

final class MobileSettings {
    private static final int PICK_DRIVER=730;
    private final Activity activity;
    private final TouchControls touch;
    private final android.content.SharedPreferences prefs;
    private final Handler handler=new Handler(Looper.getMainLooper());
    private ProgressDialog progress;
    private DriverImportJob observed;
    private boolean detached;
    private final Runnable poll=this::pollImport;
    MobileSettings(Activity owner,TouchControls controls){activity=owner;touch=controls;prefs=owner.getSharedPreferences("touch_layout_v1",0);}
    static LinearLayout column(Context c){LinearLayout v=new LinearLayout(c);v.setOrientation(LinearLayout.VERTICAL);int pad=MenuStyle.dp(c,20);v.setPadding(pad,pad,pad,pad);v.setBackgroundColor(MenuStyle.BACKGROUND);return v;}
    static ScrollView scroll(LinearLayout column){ScrollView s=new ScrollView(column.getContext());s.addView(column);return s;}
    static void slider(LinearLayout v,String label,int value,int max,IntConsumer changed){
        TextView text=new TextView(v.getContext());text.setText(label);text.setTextColor(MenuStyle.TEXT);v.addView(text);
        SeekBar bar=new SeekBar(v.getContext());bar.setMax(max);bar.setProgress(Math.max(0,Math.min(max,value)));bar.setProgressTintList(MenuStyle.accent());bar.setThumbTintList(MenuStyle.accent());v.addView(bar);
        bar.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener(){public void onStartTrackingTouch(SeekBar b){}public void onStopTrackingTouch(SeekBar b){}public void onProgressChanged(SeekBar b,int p,boolean user){if(user)changed.accept(p);}});
    }
    private TextView text(LinearLayout v,String t){TextView view=new TextView(activity);view.setText(t);view.setTextColor(MenuStyle.MUTED);view.setTextSize(14);view.setPadding(0,MenuStyle.dp(activity,5),0,MenuStyle.dp(activity,8));v.addView(view);return view;}
    private void button(LinearLayout v,String label,Runnable action){Button b=new Button(activity);b.setText(label);MenuStyle.button(b);b.setOnClickListener(w->action.run());LinearLayout.LayoutParams lp=new LinearLayout.LayoutParams(-1,MenuStyle.dp(activity,50));lp.setMargins(0,MenuStyle.dp(activity,8),0,0);v.addView(b,lp);}
    private Spinner choice(LinearLayout v,String title,String[] values,int selected){text(v,title);Spinner s=new Spinner(activity);s.setAdapter(new ArrayAdapter<>(activity,android.R.layout.simple_spinner_dropdown_item,values));s.setSelection(Math.max(0,Math.min(values.length-1,selected)));v.addView(s);return s;}
    private CheckBox check(LinearLayout v,String label,String key,boolean fallback){CheckBox b=new CheckBox(activity);b.setText(label);b.setTextColor(MenuStyle.TEXT);b.setButtonTintList(MenuStyle.accent());b.setChecked(prefs.getBoolean(key,fallback));v.addView(b);return b;}
    private float[] gain(LinearLayout v,String label,String key,float fallback,float maximum){
        float value=prefs.getFloat(key,fallback);if(!Float.isFinite(value))value=fallback;
        float[] result={Math.max(.25f,Math.min(maximum,value))};TextView caption=text(v,String.format(Locale.ROOT,"%s: %.2f×",label,result[0]));
        slider(v,label,Math.round((result[0]-.25f)*100),Math.round((maximum-.25f)*100),p->{result[0]=.25f+p/100f;caption.setText(String.format(Locale.ROOT,"%s: %.2f×",label,result[0]));});return result;
    }
    private void write(String name,String value) throws IOException {DriverImportJob.atomicWrite(new File(activity.getFilesDir(),name),(value+"\n").getBytes(StandardCharsets.UTF_8));}
    private float scale(){
        if(PTActivity.gameRunning())try{return PTActivity.nativeGetRenderScale();}catch(UnsatisfiedLinkError ignored){}
        try{float v=Float.parseFloat(new String(Files.readAllBytes(new File(activity.getFilesDir(),"render_scale.txt").toPath()),StandardCharsets.UTF_8).trim());if(Float.isFinite(v))return Math.max(.25f,Math.min(1f,v));}catch(Exception ignored){}
        return .667f;
    }
    void show(){
        if(touch!=null)touch.releaseAll();
        LinearLayout v=column(activity);
        MenuStyle.section(v,"Graphics");
        Spinner profile=choice(v,"Graphics preset",new String[]{"Custom • keep game settings","Maximum performance","Balanced • up to 60 FPS","Original (PS4)"},prefs.getInt("performance_profile",0));
        text(v,"Maximum performance removes the frame cap and reduces shadows, texture detail and effects. Actual FPS depends on the scene and device temperature.");
        float[] scale={scale()};TextView scaleText=new TextView(activity);scaleText.setText("Render scale: "+Math.round(scale[0]*100)+"%");v.addView(scaleText);
        slider(v,"Scene resolution",Math.round(scale[0]*100)-25,75,p->{scale[0]=(p+25)/100f;scaleText.setText("Render scale: "+(p+25)+"%");});
        CheckBox bloom=new CheckBox(activity);bloom.setText("Optimized mobile bloom");bloom.setChecked(prefs.getBoolean("optimized_bloom",true));v.addView(bloom);
        CheckBox pacing=check(v,"Android frame pacing", "frame_pacing",false);
        text(v,"Optional smoother presentation with a frame limit. Uncapped gameplay keeps the fastest existing presentation path.");
        CheckBox sgsr=check(v,"SGSR1 sharper upscaling", "sgsr",false);
        text(v,"Optional image quality at reduced render scale. It adds GPU work; keep it off for maximum performance. Resolution stays fixed.");
        float[] sharpness=gain(v,"Upscaling sharpness","sharpness",1f,2f);
        Spinner ao=choice(v,"Ambient occlusion",new String[]{"Original quality","Performance • fewer samples"},prefs.getInt("ao_quality",0));
        Spinner reflections=choice(v,"Screen reflections",new String[]{"Original quality","Performance • fewer steps"},prefs.getInt("reflection_quality",0));
        CheckBox sustained=check(v,"Sustained performance", "sustained",false);
        android.os.PowerManager power=(android.os.PowerManager)activity.getSystemService(Context.POWER_SERVICE);
        sustained.setEnabled(power!=null&&power.isSustainedPerformanceModeSupported());
        if(!sustained.isEnabled())sustained.setChecked(false);
        text(v,sustained.isEnabled()?"Asks Android for steadier performance during long sessions. Keeps your FPS limit and graphics settings. Android still controls temperature and clock speeds.":"This device does not support Android sustained performance mode.");
        text(v,String.format(Locale.ROOT,"Current display: %.2f Hz. FPS and display refresh are controlled together in Game graphics.",DisplayRefresh.display(activity).getRefreshRate()));
        MenuStyle.section(v,"Controls");
        Spinner movement=choice(v,"Movement control",new String[]{"Visible joystick","Invisible left-half movement"},prefs.getInt("move_style",0));
        text(v,"Invisible movement: touch and drag on the left half, then hold to keep moving. Pair it with swipe look on the right. Lift to stop. Stick size and dead zone still apply; buttons and Settings take priority.");
        Spinner look=choice(v,"Look control",new String[]{"Visible joystick","Swipe to look • no button","Hidden joystick"},prefs.getInt("look_style",1));
        text(v,"Swipe on the right half to turn by finger movement. Hidden joystick keeps turning while held away from its touch origin. Buttons take priority.");
        float stored=prefs.getFloat("sensitivity",1f);float[] sensitivity={Float.isFinite(stored)?Math.max(.25f,Math.min(3f,stored)):1f};
        TextView sensitivityText=text(v,String.format(Locale.ROOT,"Look sensitivity: %.2f×",sensitivity[0]));
        slider(v,"Slower ← swipe / joystick → faster",Math.round((sensitivity[0]-.25f)*100),275,p->{sensitivity[0]=.25f+p/100f;sensitivityText.setText(String.format(Locale.ROOT,"Look sensitivity: %.2f×",sensitivity[0]));});
        Spinner mode=choice(v,"Touch controls",new String[]{"Auto","Off","Always show"},prefs.getInt("overlay_mode",0));
        CheckBox invert=check(v,"Invert vertical look", "invert_y",false);
        float[] mouse=gain(v,"Mouse sensitivity","mouse_sensitivity",1f,3f);
        float[] controller=gain(v,"Controller sensitivity","controller_sensitivity",1f,3f);
        CheckBox gyro=check(v,"Gyroscope look", "gyro",false);gyro.setEnabled(MobileRuntime.hasGyro(activity));
        if(!gyro.isEnabled()){gyro.setChecked(false);text(v,"No gyroscope is available on this device.");}
        float[] gyroGain=gain(v,"Gyroscope sensitivity","gyro_sensitivity",1f,3f);
        MenuStyle.section(v,"Diagnostics");
        CheckBox logs=new CheckBox(activity);logs.setText("Enable logs");logs.setChecked(AppLog.enabled());logs.setButtonTintList(MenuStyle.accent());v.addView(logs);
        text(v,"Off stops app diagnostic files, console output and GPU timing collection. Turn on before reproducing a problem you want to report.");
        AlertDialog dialog=new AlertDialog.Builder(activity).setTitle("pt4droid settings").setView(scroll(v)).setPositiveButton("Apply",null).setNegativeButton("Close",null).create();
        button(v,"Customize touch layout",()->{dialog.dismiss();if(touch!=null)touch.beginEdit();else message("Start the game, then open Android settings to edit the layout.");});
        button(v,"Vulkan drivers",()->{dialog.dismiss();drivers();});
        button(v,"Game graphics",()->{dialog.dismiss();GameGraphicsSettings.open(activity);});
        if(activity instanceof PTActivity)button(v,"Return to menu",()->{
            new AlertDialog.Builder(activity).setTitle("Return to menu?")
                    .setMessage("Leave the game and return to the launcher. Progress follows the game's normal save points.")
                    .setPositiveButton("Return",(d,b)->{dialog.dismiss();((PTActivity)activity).returnToMenu();})
                    .setNegativeButton("Stay",null).show();
        });
        button(v,"Delete saves",()->{dialog.dismiss();deleteSaves();});
        dialog.setOnShowListener(d->dialog.getButton(AlertDialog.BUTTON_POSITIVE).setOnClickListener(w->{
            try{
                int selected=profile.getSelectedItemPosition();if(selected==1)scale[0]=Math.min(scale[0],.5f);
                if(selected>0){Files.deleteIfExists(new File(activity.getFilesDir(),"game_graphics_pending.ini").toPath());Files.deleteIfExists(new File(activity.getFilesDir(),"game_graphics_applying.ini").toPath());}
                write("render_scale.txt",Float.toString(scale[0]));write("performance_profile.txt",Integer.toString(selected));write("bloom_pass_limit.txt",bloom.isChecked()?"6":"0");
                write("performance_profile_pending.txt",Integer.toString(selected));
                write("mobile_performance.ini","frame_pacing="+(pacing.isChecked()?1:0)+"\nsgsr="+(sgsr.isChecked()?1:0)+"\nsharpness="+sharpness[0]+"\nao_quality="+ao.getSelectedItemPosition()+"\nreflection_quality="+reflections.getSelectedItemPosition()+"\nsustained="+(sustained.isChecked()?1:0)+"\ninvert_y="+(invert.isChecked()?1:0)+"\nmouse_sensitivity="+mouse[0]+"\ncontroller_sensitivity="+controller[0]);
                prefs.edit().putBoolean("frame_pacing",pacing.isChecked()).putBoolean("sgsr",sgsr.isChecked()).putFloat("sharpness",sharpness[0]).putInt("ao_quality",ao.getSelectedItemPosition()).putInt("reflection_quality",reflections.getSelectedItemPosition()).putBoolean("sustained",sustained.isChecked()).putBoolean("invert_y",invert.isChecked()).putFloat("mouse_sensitivity",mouse[0]).putFloat("controller_sensitivity",controller[0]).putBoolean("gyro",gyro.isChecked()).putFloat("gyro_sensitivity",gyroGain[0]).apply();
                prefs.edit().putInt("performance_profile",selected).putFloat("sensitivity",sensitivity[0]).putInt("move_style",movement.getSelectedItemPosition()).putInt("look_style",look.getSelectedItemPosition()).putInt("overlay_mode",mode.getSelectedItemPosition()).putBoolean("optimized_bloom",bloom.isChecked()).apply();
                DisplayRefresh.apply(activity,null);PTActivity.refreshDisplay();
                AppLog.setEnabled(logs.isChecked());
                if(touch!=null){touch.reloadPreferences();touch.setMode(mode.getSelectedItemPosition());}
                if(PTActivity.gameRunning()){PTActivity.nativeSetRenderScale(scale[0]);PTActivity.nativeSetPerformanceProfile(selected);PTActivity.nativeSetBloomPassLimit(bloom.isChecked()?6:0);PTActivity.reloadPerformance();}
                dialog.dismiss();
            }catch(Throwable error){message("Could not apply settings: "+error.getMessage());}
        }));dialog.show();MenuStyle.styleDialog(dialog);
    }
    private void drivers(){
        LinearLayout v=column(activity);List<String> names=new ArrayList<>(Arrays.asList("Automatic • match device","Mesa Turnip • bundled (Adreno)","System Vulkan driver","PanVK Kbase RC3 • experimental (Mali, Android 15+)"));List<String> selections=new ArrayList<>(Arrays.asList("auto","turnip","system","panvk"));
        File[] folders=new File(activity.getFilesDir(),"drivers").listFiles();if(folders!=null){Arrays.sort(folders,Comparator.comparing(File::getName));for(File folder:folders){
            if(!folder.isDirectory() || folder.getName().startsWith("."))continue;
            try{JSONObject info=new JSONObject(new String(Files.readAllBytes(new File(folder,"package.json").toPath()),StandardCharsets.UTF_8));String lib=info.getString("library");File file=DriverArchive.contained(folder,lib);DriverArchive.validateElf(file);names.add(info.optString("name",folder.getName())+(info.optBoolean("experimental",false)?" • experimental (Mali)":""));selections.add("imported/"+folder.getName()+"/"+lib);}catch(Exception ignored){}
        }}
        int selected=0;try{selected=selections.indexOf(new String(Files.readAllBytes(new File(activity.getFilesDir(),"driver.txt").toPath()),StandardCharsets.UTF_8).trim());}catch(Exception ignored){}
        Spinner selection=choice(v,"Vulkan driver",names.toArray(new String[0]),selected);
        text(v,"Automatic uses bundled Turnip on supported Adreno 6xx/7xx and System on other GPUs. PanVK is experimental for Mali v9–v13 with mali_kbase and Android 15+. Game compatibility and performance are not verified. Older Mali GPUs and other kernel drivers may fail. Fully close and reopen after a driver change.");
        AlertDialog d=new AlertDialog.Builder(activity).setTitle("Vulkan drivers").setView(scroll(v)).setPositiveButton("Select",(a,b)->{try{String driver=selections.get(selection.getSelectedItemPosition());if(driver.equals("panvk")&&android.os.Build.VERSION.SDK_INT<35)throw new IOException("Bundled PanVK requires Android 15 or later. Use System or an Android-compatible imported driver.");write("driver.txt",driver);message("Driver selected. Fully close and reopen the game to load it.");}catch(IOException ex){message(ex.getMessage());}}).setNegativeButton("Close",null).create();
        button(v,"Import driver ZIP",()->{d.dismiss();pickDriver();});d.show();MenuStyle.styleDialog(d);
    }
    private void pickDriver(){try{Intent intent=new Intent(Intent.ACTION_OPEN_DOCUMENT);intent.addCategory(Intent.CATEGORY_OPENABLE);intent.setType("*/*");intent.putExtra(Intent.EXTRA_MIME_TYPES,new String[]{"application/zip","application/octet-stream","application/x-zip-compressed"});activity.startActivityForResult(intent,PICK_DRIVER);}catch(Exception ex){message("Could not open file picker: "+ex.getMessage());}}
    boolean activityResult(int request,int result,Intent data){
        if(request!=PICK_DRIVER)return false;
        if(result==Activity.RESULT_OK && data!=null && data.getData()!=null){DriverImportJob.start(activity,data.getData());resumeImport();}return true;
    }
    void resumeImport(){detached=false;DriverImportJob job=DriverImportJob.get();if(job!=null && job!=observed){observed=job;pollImport();}else if(job!=null && !job.done)pollImport();}
    private void pollImport(){
        handler.removeCallbacks(poll);if(detached || activity.isFinishing())return;DriverImportJob job=DriverImportJob.get();if(job==null)return;
        if(job.done){if(progress!=null){progress.dismiss();progress=null;}if(!job.acknowledged){job.acknowledged=true;message(job.result);}return;}
        if(progress==null){progress=new ProgressDialog(activity);progress.setTitle("Importing driver");progress.setProgressStyle(ProgressDialog.STYLE_HORIZONTAL);progress.setMax(100);progress.setCancelable(true);progress.setOnCancelListener(d->job.cancel("Import cancelled"));progress.setButton(DialogInterface.BUTTON_NEGATIVE,"Cancel",(d,w)->job.cancel("Import cancelled"));progress.show();}
        progress.setMessage(job.phase+" • "+(job.bytes/1024)+" KB");progress.setIndeterminate(job.total<=0);if(job.total>0)progress.setProgress((int)Math.min(100,job.bytes*100/job.total));handler.postDelayed(poll,150);
    }
    void detach(){detached=true;handler.removeCallbacks(poll);if(progress!=null){progress.dismiss();progress=null;}}
    void deleteSaves(){
        if(PTActivity.gameRunning() || GameSession.isActive(activity)){message("Close the game before deleting saves.");return;}
        new AlertDialog.Builder(activity).setTitle("Delete saves?").setMessage("Delete the two save slots and begin a new game. Game files, settings, touch layout, drivers, caches and Extras unlocks stay.").setNegativeButton("Cancel",null).setPositiveButton("Delete saves",(d,w)->{
            if(PTActivity.gameRunning() || GameSession.isActive(activity)){message("Close the game first.");return;}
            try{int count=SaveFiles.delete(new File(activity.getFilesDir(),"data"));File external=activity.getExternalFilesDir(null);if(external!=null)count+=SaveFiles.delete(new File(external,"data"));message(count>0?"Saves deleted. Start P.T. for a new game.":"No save slots found.");}catch(IOException ex){message("Could not delete saves: "+ex.getMessage());}
        }).show();
    }
    private void message(String text){if(!detached && !activity.isFinishing()){AlertDialog d=new AlertDialog.Builder(activity).setTitle("pt4droid").setMessage(text).setPositiveButton("OK",null).show();MenuStyle.styleDialog(d);}}
}
