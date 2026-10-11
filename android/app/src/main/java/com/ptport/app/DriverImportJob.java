package com.ptport.app;
import android.content.Context;
import android.content.res.AssetFileDescriptor;
import android.net.Uri;
import android.os.Build;
import android.os.CancellationSignal;
import org.json.JSONObject;
import java.io.*;
import java.nio.file.*;
import java.security.*;
import java.util.*;
import java.util.concurrent.*;

/** Application-owned work survives activity recreation; the UI observes only a snapshot. */
final class DriverImportJob {
    private static final Object LOCK=new Object();
    private static DriverImportJob current;
    private static final ScheduledExecutorService TIMER=Executors.newSingleThreadScheduledExecutor(r->{Thread t=new Thread(r,"driver-deadline");t.setDaemon(true);return t;});
    private final Context app;
    private final Uri uri;
    private final CancellationSignal signal=new CancellationSignal();
    private volatile AssetFileDescriptor descriptor;
    private final Object commit=new Object();
    private volatile boolean cancelled;
    volatile boolean done,success,acknowledged;
    volatile String phase="Opening file",result="";
    volatile long bytes,total=-1;
    private DriverImportJob(Context c,Uri u){app=c.getApplicationContext();uri=u;}
    static DriverImportJob get(){synchronized(LOCK){return current;}}
    static DriverImportJob start(Context c,Uri u){synchronized(LOCK){if(current!=null && !current.done)return current;current=new DriverImportJob(c,u);Thread t=new Thread(current::run,"driver-import");t.setDaemon(true);t.start();return current;}}
    void cancel(String reason){
        synchronized(commit){if(done)return;cancelled=true;result=reason;done=true;}
        Thread closer=new Thread(()->{try{signal.cancel();}catch(Throwable ignored){}try{AssetFileDescriptor fd=descriptor;if(fd!=null)fd.close();}catch(Throwable ignored){}},"driver-cancel");closer.setDaemon(true);closer.start();
    }
    private void check() throws IOException{if(cancelled || Thread.currentThread().isInterrupted())throw new IOException("Import cancelled");}
    static void atomicWrite(File path,byte[] data) throws IOException {
        File parent=path.getParentFile();if(!parent.isDirectory() && !parent.mkdirs())throw new IOException("Cannot create settings folder");
        File temp=File.createTempFile(".write-",".tmp",parent);
        try{
            try(FileOutputStream out=new FileOutputStream(temp)){out.write(data);out.getFD().sync();}
            try{Files.move(temp.toPath(),path.toPath(),StandardCopyOption.ATOMIC_MOVE,StandardCopyOption.REPLACE_EXISTING);}
            catch(AtomicMoveNotSupportedException ex){Files.move(temp.toPath(),path.toPath(),StandardCopyOption.REPLACE_EXISTING);}
        }finally{temp.delete();}
    }
    private static String hex(byte[] bytes){StringBuilder out=new StringBuilder();for(byte value:bytes)out.append(String.format(Locale.ROOT,"%02x",value&255));return out.toString();}
    private static byte[] hash(File file) throws Exception {MessageDigest h=MessageDigest.getInstance("SHA-256");try(InputStream in=new FileInputStream(file)){byte[] b=new byte[32768];int n;while((n=in.read(b))!=-1)h.update(b,0,n);}return h.digest();}
    private void log(String text){AppLog.append(app,"driver-import.log",text);}
    private void run(){
        File zip=null,stage=null;ScheduledFuture<?> deadline=TIMER.schedule(()->cancel("Import timed out after 60 seconds. Try a ZIP copied to local Downloads."),60,TimeUnit.SECONDS);
        try{
            File root=new File(app.getFilesDir(),"drivers").getCanonicalFile();if(!root.isDirectory() && !root.mkdirs())throw new IOException("Cannot create drivers folder");
            zip=File.createTempFile(".import-",".zip",root);
            stage=new File(root,".staging-"+UUID.randomUUID()).getCanonicalFile();if(!stage.mkdir())throw new IOException("Cannot create import folder");
            descriptor=app.getContentResolver().openAssetFileDescriptor(uri,"r",signal);check();
            if(descriptor==null)throw new IOException("The file provider did not open the ZIP");
            total=descriptor.getLength();if(total>DriverArchive.MAX_COMPRESSED)throw new IOException("Driver ZIP exceeds 64 MB");
            phase="Reading ZIP";MessageDigest digest=MessageDigest.getInstance("SHA-256");
            try(InputStream in=descriptor.createInputStream();OutputStream out=new FileOutputStream(zip)){
                byte[] buffer=new byte[32768];int n,empty=0;
                while(true){check();n=in.read(buffer);check();if(n<0)break;if(n==0){if(++empty>128)throw new IOException("File provider stalled");continue;}empty=0;bytes+=n;if(bytes>DriverArchive.MAX_COMPRESSED)throw new IOException("Driver ZIP exceeds 64 MB");digest.update(buffer,0,n);out.write(buffer,0,n);}
            }
            check();String sha=hex(digest.digest());String id=sha.substring(0,24);phase="Extracting ZIP";bytes=0;total=-1;
            List<File> files=DriverArchive.extract(zip,stage,this::check,value->bytes=value);
            phase="Checking driver";File meta=null;
            for(File file:files)if(file.getName().equals("meta.json")){if(meta!=null)throw new IOException("Multiple meta.json files");meta=file;}
            if(meta==null || meta.length()>65536)throw new IOException("Missing or oversized meta.json");
            JSONObject metadata=new JSONObject(new String(Files.readAllBytes(meta.toPath()),java.nio.charset.StandardCharsets.UTF_8));
            if(metadata.optInt("minApi",29)>Build.VERSION.SDK_INT)throw new IOException("This driver needs a newer Android version");
            String libraryName=metadata.getString("libraryName");if(!libraryName.endsWith(".so"))throw new IOException("Invalid driver libraryName");
            File library=DriverArchive.contained(meta.getParentFile(),libraryName);
            if(!files.contains(library) || !library.isFile())throw new IOException("Driver library is missing");
            DriverArchive.validateElf(library);check();
            // Both paths must be canonical: Android can expose /data/data and /data/user/0 aliases.
            String relative=stage.toPath().relativize(library.getCanonicalFile().toPath()).toString().replace(File.separatorChar,'/');
            DriverArchive.contained(stage,relative);
            JSONObject info=new JSONObject();info.put("name",metadata.optString("name","Imported driver"));info.put("library",relative);info.put("sha256",sha);
            File target=new File(root,id);phase="Installing driver";
            synchronized(commit){
                check();
                if(target.exists()) {
                    File old=DriverArchive.contained(target,relative);DriverArchive.validateElf(old);
                    if(!Arrays.equals(hash(old),hash(library)))throw new IOException("Existing driver package is damaged; remove it and re-import");
                    atomicWrite(new File(target,"package.json"),info.toString().getBytes(java.nio.charset.StandardCharsets.UTF_8));
                }else{
                    atomicWrite(new File(stage,"package.json"),info.toString().getBytes(java.nio.charset.StandardCharsets.UTF_8));
                    check();if(!stage.renameTo(target))throw new IOException("Could not install driver package");stage=null;
                }
                check();atomicWrite(new File(app.getFilesDir(),"driver.txt"),("imported/"+id+"/"+relative+"\n").getBytes(java.nio.charset.StandardCharsets.UTF_8));
                success=true;result="Driver imported. Fully close and reopen the game to load it.";done=true;
            }
            log("Imported "+id+"/"+relative);
        }catch(Throwable error){synchronized(commit){if(!done){result="Import failed: "+error.getClass().getSimpleName()+": "+String.valueOf(error.getMessage());done=true;}}log(result);}
        finally{deadline.cancel(false);try{if(descriptor!=null)descriptor.close();}catch(Throwable ignored){}if(zip!=null)zip.delete();DriverArchive.deleteTree(stage);}
    }
}
