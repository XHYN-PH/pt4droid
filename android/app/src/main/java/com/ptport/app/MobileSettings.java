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
        text(v,"Choose what you want to change. Your settings and FPS limit stay as you set them.");
        AlertDialog menu=new AlertDialog.Builder(activity).setTitle("Settings").setView(scroll(v)).setNegativeButton("Close",null).create();
        button(v,"Game graphics",()->{menu.dismiss();GameGraphicsSettings.open(activity);});
        button(v,"Controls",()->{menu.dismiss();showPage(false);});
        button(v,"Performance & advanced",()->{menu.dismiss();showPage(true);});
        if(!(activity instanceof PTActivity))button(v,"Graphics driver",()->{menu.dismiss();drivers();});
        if(activity instanceof PTActivity)button(v,"Return to main menu",()->new AlertDialog.Builder(activity).setTitle("Return to main menu?").setMessage("Progress follows the game's normal save points.").setPositiveButton("Return",(d,b)->{menu.dismiss();((PTActivity)activity).returnToMenu();}).setNegativeButton("Stay",null).show());
        menu.show();MenuStyle.styleDialog(menu);
    }
    private void showPage(boolean advanced){new SettingsPages(activity,touch,this::show).show(advanced);}
    private void drivers(){
        LinearLayout v=column(activity);List<String> names=new ArrayList<>(Arrays.asList("Automatic • match device","WinNative Turnip 1.19 • Balanced (Adreno)","System Vulkan driver","PanVK Kbase RC3 • experimental (Mali, Android 15+)"));List<String> selections=new ArrayList<>(Arrays.asList("auto","turnip","system","panvk"));
        File[] folders=new File(activity.getFilesDir(),"drivers").listFiles();if(folders!=null){Arrays.sort(folders,Comparator.comparing(File::getName));for(File folder:folders){
            if(!folder.isDirectory() || folder.getName().startsWith("."))continue;
            try{JSONObject info=new JSONObject(new String(Files.readAllBytes(new File(folder,"package.json").toPath()),StandardCharsets.UTF_8));String lib=info.getString("library");File file=DriverArchive.contained(folder,lib);DriverArchive.validateElf(file);names.add(info.optString("name",folder.getName())+(info.optBoolean("experimental",false)?" • experimental ("+info.optString("family","Mali")+")":""));selections.add("imported/"+folder.getName()+"/"+lib);}catch(Exception ignored){}
        }}
        int selected=0;try{selected=selections.indexOf(new String(Files.readAllBytes(new File(activity.getFilesDir(),"driver.txt").toPath()),StandardCharsets.UTF_8).trim());}catch(Exception ignored){}
        Spinner selection=choice(v,"Vulkan driver",names.toArray(new String[0]),selected);
        text(v,"Automatic uses WinNative Turnip 1.19 Balanced on supported Adreno 6xx/7xx with 4 KiB pages. The upstream 1.19 binary has 4 KiB alignment; 16 KiB phones use System instead. Other GPUs also use System. PanVK is experimental for Mali v9–v13 with mali_kbase and Android 15+. Game compatibility and performance are not verified. Older Mali GPUs and other kernel drivers may fail. Fully close and reopen after a driver change.");
        AlertDialog d=new AlertDialog.Builder(activity).setTitle("Vulkan drivers").setView(scroll(v)).setPositiveButton("Select",(a,b)->{try{String driver=selections.get(selection.getSelectedItemPosition());if(driver.equals("panvk")&&android.os.Build.VERSION.SDK_INT<35)throw new IOException("Bundled PanVK requires Android 15 or later. Use System or an Android-compatible imported driver.");write("driver.txt",driver);message("Driver selected. Fully close and reopen the game to load it.");}catch(IOException ex){message(ex.getMessage());}}).setNegativeButton("Close",null).create();
        text(v,"Imported Android/Bionic Vulkan packages can target Mali (PanVK) or supported Xclipse GPUs (RADV). Match the package to your exact GPU and Android version. Linux/glibc and OpenGL drivers do not work here.");
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
