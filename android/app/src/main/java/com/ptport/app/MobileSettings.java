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
    static LinearLayout column(Context c){LinearLayout v=new LinearLayout(c);v.setOrientation(LinearLayout.VERTICAL);int pad=(int)(18*c.getResources().getDisplayMetrics().density);v.setPadding(pad,pad,pad,pad);return v;}
    static ScrollView scroll(LinearLayout column){ScrollView s=new ScrollView(column.getContext());s.addView(column);return s;}
    static void slider(LinearLayout v,String label,int value,int max,IntConsumer changed){
        TextView text=new TextView(v.getContext());text.setText(label);v.addView(text);
        SeekBar bar=new SeekBar(v.getContext());bar.setMax(max);bar.setProgress(Math.max(0,Math.min(max,value)));v.addView(bar);
        bar.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener(){public void onStartTrackingTouch(SeekBar b){}public void onStopTrackingTouch(SeekBar b){}public void onProgressChanged(SeekBar b,int p,boolean user){if(user)changed.accept(p);}});
    }
    private void text(LinearLayout v,String t){TextView view=new TextView(activity);view.setText(t);v.addView(view);}
    private void button(LinearLayout v,String label,Runnable action){Button b=new Button(activity);b.setText(label);b.setOnClickListener(w->action.run());v.addView(b);}
    private Spinner choice(LinearLayout v,String title,String[] values,int selected){text(v,title);Spinner s=new Spinner(activity);s.setAdapter(new ArrayAdapter<>(activity,android.R.layout.simple_spinner_dropdown_item,values));s.setSelection(Math.max(0,Math.min(values.length-1,selected)));v.addView(s);return s;}
    private void write(String name,String value) throws IOException {DriverImportJob.atomicWrite(new File(activity.getFilesDir(),name),(value+"\n").getBytes(StandardCharsets.UTF_8));}
    private float scale(){
        if(PTActivity.gameRunning())try{return PTActivity.nativeGetRenderScale();}catch(UnsatisfiedLinkError ignored){}
        try{float v=Float.parseFloat(new String(Files.readAllBytes(new File(activity.getFilesDir(),"render_scale.txt").toPath()),StandardCharsets.UTF_8).trim());if(Float.isFinite(v))return Math.max(.25f,Math.min(1f,v));}catch(Exception ignored){}
        return .667f;
    }
    void show(){
        if(touch!=null)touch.releaseAll();
        LinearLayout v=column(activity);
        Spinner profile=choice(v,"Performance preset",new String[]{"Custom • keep game settings","Maximum performance","Balanced • up to 60 FPS"},prefs.getInt("performance_profile",0));
        text(v,"Maximum performance removes the frame cap and reduces shadows, texture detail and effects. Actual FPS depends on the scene and device temperature.");
        float[] scale={scale()};TextView scaleText=new TextView(activity);scaleText.setText("Render scale: "+Math.round(scale[0]*100)+"%");v.addView(scaleText);
        slider(v,"Scene resolution",Math.round(scale[0]*100)-25,75,p->{scale[0]=(p+25)/100f;scaleText.setText("Render scale: "+(p+25)+"%");});
        CheckBox bloom=new CheckBox(activity);bloom.setText("Optimized mobile bloom");bloom.setChecked(prefs.getBoolean("optimized_bloom",true));v.addView(bloom);
        Spinner look=choice(v,"Look control",new String[]{"Visible joystick","Swipe to look • no button","Hidden joystick"},prefs.getInt("look_style",1));
        text(v,"Swipe on the right half to turn by finger movement. Hidden joystick keeps turning while held away from its touch origin. Buttons take priority.");
        Spinner mode=choice(v,"Touch controls",new String[]{"Auto","Off","Always show"},prefs.getInt("overlay_mode",0));
        AlertDialog dialog=new AlertDialog.Builder(activity).setTitle("Android settings").setView(scroll(v)).setPositiveButton("Apply",null).setNegativeButton("Close",null).create();
        button(v,"Customize touch layout",()->{dialog.dismiss();if(touch!=null)touch.beginEdit();else message("Start the game, then open Android settings to edit the layout.");});
        button(v,"Vulkan drivers",()->{dialog.dismiss();drivers();});
        button(v,"Delete saves",()->{dialog.dismiss();deleteSaves();});
        dialog.setOnShowListener(d->dialog.getButton(AlertDialog.BUTTON_POSITIVE).setOnClickListener(w->{
            try{
                int selected=profile.getSelectedItemPosition();if(selected==1)scale[0]=Math.min(scale[0],.5f);
                write("render_scale.txt",Float.toString(scale[0]));write("performance_profile.txt",Integer.toString(selected));write("bloom_pass_limit.txt",bloom.isChecked()?"6":"0");
                prefs.edit().putInt("performance_profile",selected).putInt("look_style",look.getSelectedItemPosition()).putInt("overlay_mode",mode.getSelectedItemPosition()).putBoolean("optimized_bloom",bloom.isChecked()).apply();
                if(touch!=null){touch.reloadPreferences();touch.setMode(mode.getSelectedItemPosition());}
                if(PTActivity.gameRunning()){PTActivity.nativeSetRenderScale(scale[0]);PTActivity.nativeSetPerformanceProfile(selected);PTActivity.nativeSetBloomPassLimit(bloom.isChecked()?6:0);}
                dialog.dismiss();
            }catch(Throwable error){message("Could not apply settings: "+error.getMessage());}
        }));dialog.show();
    }
    private void drivers(){
        LinearLayout v=column(activity);List<String> names=new ArrayList<>(Arrays.asList("Mesa Turnip • bundled","System Vulkan driver"));List<String> selections=new ArrayList<>(Arrays.asList("turnip","system"));
        File[] folders=new File(activity.getFilesDir(),"drivers").listFiles();if(folders!=null){Arrays.sort(folders,Comparator.comparing(File::getName));for(File folder:folders){
            if(!folder.isDirectory() || folder.getName().startsWith("."))continue;
            try{JSONObject info=new JSONObject(new String(Files.readAllBytes(new File(folder,"package.json").toPath()),StandardCharsets.UTF_8));String lib=info.getString("library");File file=DriverArchive.contained(folder,lib);DriverArchive.validateElf(file);names.add(info.optString("name",folder.getName()));selections.add("imported/"+folder.getName()+"/"+lib);}catch(Exception ignored){}
        }}
        int selected=0;try{selected=selections.indexOf(new String(Files.readAllBytes(new File(activity.getFilesDir(),"driver.txt").toPath()),StandardCharsets.UTF_8).trim());}catch(Exception ignored){}
        Spinner selection=choice(v,"Vulkan driver",names.toArray(new String[0]),selected);
        text(v,"Use an ARM64 AdrenoTools driver ZIP. The selected driver loads after fully closing and reopening the game.");
        AlertDialog d=new AlertDialog.Builder(activity).setTitle("Vulkan drivers").setView(scroll(v)).setPositiveButton("Select",(a,b)->{try{write("driver.txt",selections.get(selection.getSelectedItemPosition()));message("Driver selected. Fully close and reopen the game to load it.");}catch(IOException ex){message(ex.getMessage());}}).setNegativeButton("Close",null).create();
        button(v,"Import driver ZIP",()->{d.dismiss();pickDriver();});d.show();
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
        if(PTActivity.gameRunning()){message("Close the game before deleting saves.");return;}
        new AlertDialog.Builder(activity).setTitle("Delete saves?").setMessage("Delete the two save slots and begin a new game. Game files, settings, touch layout, drivers, caches and Extras unlocks stay.").setNegativeButton("Cancel",null).setPositiveButton("Delete saves",(d,w)->{
            if(PTActivity.gameRunning()){message("Close the game first.");return;}
            try{int count=SaveFiles.delete(new File(activity.getFilesDir(),"data"));File external=activity.getExternalFilesDir(null);if(external!=null)count+=SaveFiles.delete(new File(external,"data"));message(count>0?"Saves deleted. Start P.T. for a new game.":"No save slots found.");}catch(IOException ex){message("Could not delete saves: "+ex.getMessage());}
        }).show();
    }
    private void message(String text){if(!detached && !activity.isFinishing())new AlertDialog.Builder(activity).setTitle("P.T. Test").setMessage(text).setPositiveButton("OK",null).show();}
}
