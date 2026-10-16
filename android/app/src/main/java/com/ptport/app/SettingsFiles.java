package com.ptport.app;

import android.content.SharedPreferences;
import java.io.File;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.util.LinkedHashMap;
import java.util.Map;

/** Roll back an immediate write failure before asking the game to reload. */
final class SettingsFiles {
    static byte[] text(String value){return (value+"\n").getBytes(StandardCharsets.UTF_8);}
    private static void preference(SharedPreferences.Editor editor,String key,Object value){
        if(value==null)editor.remove(key);
        else if(value instanceof Boolean)editor.putBoolean(key,(Boolean)value);
        else if(value instanceof Integer)editor.putInt(key,(Integer)value);
        else if(value instanceof Long)editor.putLong(key,(Long)value);
        else if(value instanceof Float)editor.putFloat(key,(Float)value);
        else if(value instanceof String)editor.putString(key,(String)value);
        else throw new IllegalArgumentException("Unsupported preference: "+key);
    }
    static synchronized void commit(Map<File,byte[]> changes,SharedPreferences prefs,Map<String,Object> values)throws IOException{
        Map<File,byte[]> previous=new LinkedHashMap<>();
        for(File file:changes.keySet())previous.put(file,file.exists()?Files.readAllBytes(file.toPath()):null);
        Map<String,?> old=prefs.getAll();
        try{
            for(Map.Entry<File,byte[]> entry:changes.entrySet())write(entry.getKey(),entry.getValue());
            SharedPreferences.Editor editor=prefs.edit();
            for(Map.Entry<String,Object> entry:values.entrySet())preference(editor,entry.getKey(),entry.getValue());
            if(!editor.commit())throw new IOException("Could not store settings preferences");
        }catch(Exception failure){
            IOException error=failure instanceof IOException?(IOException)failure:new IOException(failure);
            for(Map.Entry<File,byte[]> entry:previous.entrySet())try{write(entry.getKey(),entry.getValue());}catch(IOException rollback){error.addSuppressed(rollback);}
            SharedPreferences.Editor restore=prefs.edit();
            for(String key:values.keySet())preference(restore,key,old.get(key));
            if(!restore.commit())error.addSuppressed(new IOException("Could not restore settings preferences"));
            throw error;
        }
    }
    private static void write(File file,byte[] data)throws IOException{
        if(data==null)Files.deleteIfExists(file.toPath());else DriverImportJob.atomicWrite(file,data);
    }
}
