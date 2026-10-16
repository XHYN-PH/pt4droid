package com.ptport.app;

import android.app.Activity;
import android.content.*;
import android.net.Uri;
import android.os.Build;
import android.widget.Toast;
import java.io.*;
import java.nio.file.Files;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.zip.*;

final class Diagnostics {
    private static final AtomicBoolean busy=new AtomicBoolean();
    static File output(Context context){return new File(context.getCacheDir(),"pt4droid-diagnostics.zip");}
    static void share(Activity activity){
        if(!busy.compareAndSet(false,true)){Toast.makeText(activity,"Preparing diagnostics…",Toast.LENGTH_SHORT).show();return;}
        new Thread(()->{
            try{
                File temp=new File(activity.getCacheDir(),"pt4droid-diagnostics.tmp");
                try(ZipOutputStream zip=new ZipOutputStream(new FileOutputStream(temp))){
                    String version=activity.getPackageManager().getPackageInfo(activity.getPackageName(),0).versionName;
                    entry(zip,"device.txt",("pt4droid "+version+"\n"+Build.MANUFACTURER+" "+Build.MODEL+"\nAndroid "+Build.VERSION.RELEASE+" (API "+Build.VERSION.SDK_INT+")\n"+DisplayRefresh.describe(activity)+"\n").getBytes(java.nio.charset.StandardCharsets.UTF_8));
                    String snapshot=PTActivity.gameRunning()?PTActivity.nativeGetGameGraphicsSettings():"";
                    entry(zip,"settings/effective_graphics.ini",GameGraphicsData.load(activity,snapshot).text().getBytes(java.nio.charset.StandardCharsets.UTF_8));
                    for(String name:new String[]{"driver.txt","render_scale.txt","mobile_performance.ini","performance_profile.txt","game_graphics_pending.ini"})copy(zip,new File(activity.getFilesDir(),name),"settings/"+name,64*1024);
                    File ext=activity.getExternalFilesDir(null);
                    File[] dirs={new File("/storage/emulated/0/Download/PT-logs-test"),new File(activity.getFilesDir(),"data"),ext==null?null:new File(ext,"data")};
                    int i=0;for(File dir:dirs){if(dir==null)continue;for(String name:new String[]{"pt.log","pt.log.previous","pt-java-crash.txt","pt-import.log"})copy(zip,new File(dir,name),"logs/"+i+"-"+name,2*1024*1024);i++;}
                }
                Files.move(temp.toPath(),output(activity).toPath(),java.nio.file.StandardCopyOption.REPLACE_EXISTING);
                activity.runOnUiThread(()->{if(activity.isFinishing())return;Uri uri=Uri.parse("content://"+activity.getPackageName()+".diagnostics/report.zip");Intent intent=new Intent(Intent.ACTION_SEND).setType("application/zip").putExtra(Intent.EXTRA_STREAM,uri).addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);intent.setClipData(ClipData.newRawUri("Diagnostics",uri));activity.startActivity(Intent.createChooser(intent,"Share diagnostics"));});
            }catch(Exception error){activity.runOnUiThread(()->Toast.makeText(activity,"Could not export diagnostics: "+error.getMessage(),Toast.LENGTH_LONG).show());}
            finally{busy.set(false);}
        },"diagnostic-export").start();
    }
    private static void entry(ZipOutputStream zip,String name,byte[] data)throws IOException{zip.putNextEntry(new ZipEntry(name));zip.write(data);zip.closeEntry();}
    private static void copy(ZipOutputStream zip,File file,String name,int maximum)throws IOException{
        if(!file.isFile()||!file.canRead())return;try(RandomAccessFile in=new RandomAccessFile(file,"r")){int count=(int)Math.min(in.length(),maximum);byte[] data=new byte[count];in.seek(in.length()-count);in.readFully(data);entry(zip,name,data);}
    }
}
