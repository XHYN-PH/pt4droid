package com.ptport.app;

import android.content.Context;
import android.util.Log;
import java.io.File;
import java.io.FileWriter;
import java.io.IOException;
import java.io.PrintWriter;
import java.util.Date;

/** One switch for app, SDL, importer and Java crash diagnostics. */
public final class AppLog {
    private static volatile boolean enabled;
    private static Context app;
    private AppLog() {}
    static synchronized void initialize(Context context) {
        app=context.getApplicationContext();
        enabled=app.getSharedPreferences("touch_layout_v1",0).getBoolean("logs_enabled",false);
        PTApplication.nativeConfigureLogging(enabled);
        Thread.UncaughtExceptionHandler previous=Thread.getDefaultUncaughtExceptionHandler();
        Thread.setDefaultUncaughtExceptionHandler((thread,error)->{
            synchronized(AppLog.class) {
                if(enabled) {
                    File dir=new File("/storage/emulated/0/Download/PT-logs-test");
                    if(!dir.isDirectory() && !dir.mkdirs()) {dir=app.getExternalFilesDir(null);if(dir==null)dir=app.getFilesDir();}
                    try(PrintWriter out=new PrintWriter(new FileWriter(new File(dir,"pt-java-crash.txt")))) {
                        out.println("thread "+thread.getName());error.printStackTrace(out);
                    } catch(IOException ignored) {}
                }
            }
            if(previous!=null)previous.uncaughtException(thread,error);
        });
    }
    public static boolean enabled(){return enabled;}
    static synchronized void setEnabled(boolean value) throws IOException {
        if(!app.getSharedPreferences("touch_layout_v1",0).edit().putBoolean("logs_enabled",value).commit())throw new IOException("Could not save logging preference");
        enabled=value;
        PTApplication.nativeConfigureLogging(value);
        if(PTActivity.gameRunning())PTActivity.nativeSetLoggingEnabled(value);
    }
    static synchronized void append(Context context,String name,String message) {
        if(!enabled)return;
        File base=context.getExternalFilesDir(null);if(base==null)base=context.getFilesDir();
        File file=new File(base,"data/"+name);File parent=file.getParentFile();
        if(!parent.isDirectory() && !parent.mkdirs())return;
        if(file.length()>1024*1024){File previous=new File(file.getPath()+".previous");if(previous.exists())previous.delete();file.renameTo(previous);}
        try(FileWriter out=new FileWriter(file,true)){out.write(new Date()+" "+message+"\n");}catch(IOException ignored){}
    }
    public static int v(String tag,String text){return enabled?Log.v(tag,text):0;}
    public static int v(String tag,String text,Throwable error){return enabled?Log.v(tag,text,error):0;}
    public static int d(String tag,String text){return enabled?Log.d(tag,text):0;}
    public static int d(String tag,String text,Throwable error){return enabled?Log.d(tag,text,error):0;}
    public static int i(String tag,String text){return enabled?Log.i(tag,text):0;}
    public static int i(String tag,String text,Throwable error){return enabled?Log.i(tag,text,error):0;}
    public static int w(String tag,String text){return enabled?Log.w(tag,text):0;}
    public static int w(String tag,String text,Throwable error){return enabled?Log.w(tag,text,error):0;}
    public static int e(String tag,String text){return enabled?Log.e(tag,text):0;}
    public static int e(String tag,String text,Throwable error){return enabled?Log.e(tag,text,error):0;}
    public static String getStackTraceString(Throwable error){return enabled?Log.getStackTraceString(error):"";}
    public static void exception(Throwable error){if(enabled)error.printStackTrace();}
    public static void stderr(String text){if(enabled)System.err.println(text);}
}
