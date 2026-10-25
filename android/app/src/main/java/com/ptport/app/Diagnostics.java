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
                    entry(zip,"build.txt",CrashMetadata.identity(activity).getBytes(java.nio.charset.StandardCharsets.UTF_8));
                    String snapshot=PTActivity.gameRunning()?PTActivity.nativeGetGameGraphicsSettings():"";
                    entry(zip,"settings/effective_graphics.ini",GameGraphicsData.load(activity,snapshot).text().getBytes(java.nio.charset.StandardCharsets.UTF_8));
                    writeSettings(zip,activity);
                    writeLogs(zip,activity);
                }
                Files.move(temp.toPath(),output(activity).toPath(),java.nio.file.StandardCopyOption.REPLACE_EXISTING);
                activity.runOnUiThread(()->{if(activity.isFinishing())return;Uri uri=Uri.parse("content://"+activity.getPackageName()+".diagnostics/report.zip");Intent intent=new Intent(Intent.ACTION_SEND).setType("application/zip").putExtra(Intent.EXTRA_STREAM,uri).addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);intent.setClipData(ClipData.newRawUri("Diagnostics",uri));activity.startActivity(Intent.createChooser(intent,"Share diagnostics"));});
            }catch(Exception error){activity.runOnUiThread(()->Toast.makeText(activity,"Could not export diagnostics: "+error.getMessage(),Toast.LENGTH_LONG).show());}
            finally{busy.set(false);}
        },"diagnostic-export").start();
    }
    private static void writeSettings(ZipOutputStream zip,Context context)throws IOException{
        for(String name:new String[]{"driver.txt","driver_runtime.txt","driver_startup_failed.txt","render_scale.txt","mobile_performance.ini","performance_profile.txt","performance_profile_pending.txt","game_graphics_pending.ini","game_graphics_applying.ini"})
            copy(zip,new File(context.getFilesDir(),name),"settings/"+name,64*1024);
    }
    private static void writeLogs(ZipOutputStream zip,Context context)throws IOException{
        File ext=context.getExternalFilesDir(null);
        File shared=new File("/storage/emulated/0/Download/"+(context.getPackageName().endsWith(".test")?"PT-logs-test":"PT-logs"));
        // Older Java crash handlers also wrote to the folder root when shared access was unavailable.
        File[] dirs={shared,new File(context.getFilesDir(),"data"),ext==null?null:new File(ext,"data"),context.getFilesDir(),ext};
        int i=0;for(File dir:dirs){if(dir==null)continue;
            for(String name:new String[]{"pt.log","pt.log.previous","pt-import.log","driver-import.log","driver-import.log.previous"})
                copy(zip,new File(dir,name),"logs/"+i+"-"+name,2*1024*1024);
            for(String name:new String[]{"pt-crash.txt","pt-crash.txt.previous","pt-java-crash.txt"})
                copy(zip,new File(dir,name),"logs/"+i+"-"+name,128*1024);
            i++;
        }
    }
    private static void entry(ZipOutputStream zip,String name,byte[] data)throws IOException{zip.putNextEntry(new ZipEntry(name));zip.write(data);zip.closeEntry();}
    private static void copy(ZipOutputStream zip,File file,String name,int maximum)throws IOException{
        if(!file.isFile()||!file.canRead())return;
        try(RandomAccessFile in=new RandomAccessFile(file,"r")){
            long size=in.length();int count=(int)Math.min(size,maximum);byte[] data=new byte[count];in.seek(size-count);
            int read=0,n;while(read<count&&(n=in.read(data,read,count-read))>0)read+=n;
            entry(zip,name,read==count?data:java.util.Arrays.copyOf(data,read));
            if(name.startsWith("logs/")){
                String details="original_bytes="+size+"\ncaptured_bytes="+read+"\nomitted_prefix_bytes="+(size-count)+
                    "\ntruncated="+(size>read)+"\n";
                entry(zip,name+".meta.txt",details.getBytes(java.nio.charset.StandardCharsets.UTF_8));
            }
        }catch(FileNotFoundException ignored){/* A live log may rotate between the check and open. */}
    }
}
