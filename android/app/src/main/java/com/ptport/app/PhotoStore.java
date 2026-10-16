package com.ptport.app;

import android.app.Activity;
import android.content.ContentResolver;
import android.content.ContentValues;
import android.net.Uri;
import android.provider.MediaStore;
import android.widget.Toast;
import java.io.*;
import java.util.concurrent.*;

/** Publishes an already encoded PNG; scoped storage needs no broad storage permission. */
final class PhotoStore {
    private static final ThreadPoolExecutor worker=new ThreadPoolExecutor(1,1,30,TimeUnit.SECONDS,
            new ArrayBlockingQueue<Runnable>(2),r->{Thread t=new Thread(r,"pt-photo-save");t.setDaemon(true);return t;});
    static {worker.allowCoreThreadTimeOut(true);}
    private PhotoStore(){}
    static void publish(Activity activity,String path){
        final android.content.Context context=activity.getApplicationContext();
        try {worker.execute(()->{
            String message;
            try {
                File photo=new File(path).getCanonicalFile();
                File external=context.getExternalFilesDir(null);
                boolean owned=inside(photo,new File(context.getFilesDir(),"photos"))||
                        (external!=null&&inside(photo,new File(external,"photos")));
                if(!owned||!photo.isFile()||!photo.getName().endsWith(".png"))throw new IOException("Invalid app photo");
                save(context.getContentResolver(),photo);
                message="Photo saved to Pictures/PT Photos";
            } catch(Exception error){AppLog.w("pt","Gallery save failed; original PNG retained",error);message="Gallery save failed. The PNG is kept in the app's photos folder.";}
            final String result=message;activity.runOnUiThread(()->{if(!activity.isFinishing())Toast.makeText(activity,result,Toast.LENGTH_LONG).show();});
        });}catch(RejectedExecutionException busy){activity.runOnUiThread(()->{if(!activity.isFinishing())Toast.makeText(activity,"Photo kept in app photos. Wait for the current gallery saves to finish.",Toast.LENGTH_LONG).show();});}
    }
    private static boolean inside(File photo,File directory)throws IOException{return photo.getParentFile().equals(directory.getCanonicalFile());}
    static void save(ContentResolver resolver,File photo)throws IOException{
        Uri uri=null;
        try {
            ContentValues values=new ContentValues();values.put(MediaStore.Images.Media.DISPLAY_NAME,photo.getName());
            values.put(MediaStore.Images.Media.MIME_TYPE,"image/png");values.put(MediaStore.Images.Media.RELATIVE_PATH,"Pictures/PT Photos");values.put(MediaStore.Images.Media.IS_PENDING,1);
            uri=resolver.insert(MediaStore.Images.Media.EXTERNAL_CONTENT_URI,values);if(uri==null)throw new IOException("Gallery insert returned no item");
            try(InputStream in=new FileInputStream(photo);OutputStream out=resolver.openOutputStream(uri,"w")){
                if(out==null)throw new IOException("Gallery output unavailable");byte[] buffer=new byte[65536];int n;while((n=in.read(buffer))!=-1)out.write(buffer,0,n);
            }
            values.clear();values.put(MediaStore.Images.Media.IS_PENDING,0);
            if(resolver.update(uri,values,null,null)<=0)throw new IOException("Gallery item could not be published");
        }catch(Exception error){if(uri!=null)try{resolver.delete(uri,null,null);}catch(Exception cleanup){error.addSuppressed(cleanup);}if(error instanceof IOException)throw (IOException)error;throw new IOException("Gallery save failed",error);}
    }
}
