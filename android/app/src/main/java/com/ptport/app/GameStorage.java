package com.ptport.app;
import android.content.Context;
import java.io.File;
import java.io.IOException;
/** App-scoped storage never depends on the broad shared-folder permission. */
final class GameStorage {
    static File dataRoot(Context context){
        File external=context.getExternalFilesDir(null);
        if(external!=null)try{
            if(external.isDirectory()||external.mkdirs()){
                File probe=File.createTempFile("pt-storage-",".probe",external);probe.delete();return external;
            }
        }catch(IOException|SecurityException unavailable){}
        return context.getFilesDir();
    }
}
