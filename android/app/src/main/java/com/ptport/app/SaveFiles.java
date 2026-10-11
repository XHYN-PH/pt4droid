package com.ptport.app;
import java.io.*;
final class SaveFiles {
    static int delete(File directory) throws IOException {
        int count=0;
        for(String name:new String[]{"PT_Save_Data0","PT_Save_Data1"}) {
            File slot=new File(directory,name);if(!slot.exists())continue;
            if(!slot.isFile() || !slot.delete())throw new IOException("Could not delete "+name);
            count++;
        }
        return count;
    }
}
