package com.ptport.app;
import java.io.*;
import java.util.*;
import java.util.zip.*;

/** Archive handling independent of the activity and file provider. */
final class DriverArchive {
    static final long MAX_COMPRESSED=64L*1024*1024, MAX_EXPANDED=256L*1024*1024;
    interface Checkpoint { void check() throws IOException; }
    interface Progress { void update(long bytes); }
    static File contained(File root,String name) throws IOException {
        if(name.isEmpty() || name.startsWith("/") || name.indexOf('\\')>=0 || name.indexOf(':')>=0 || name.indexOf('\0')>=0 || name.split("/",-1).length>16)
            throw new IOException("Invalid archive path");
        for(String part:name.split("/"))if(part.equals("..") || part.equals("."))throw new IOException("Archive path escapes driver folder");
        File base=root.getCanonicalFile(), result=new File(base,name).getCanonicalFile();
        if(!result.getPath().startsWith(base.getPath()+File.separator))throw new IOException("Archive path escapes driver folder");
        return result;
    }
    static List<File> extract(File zip,File root,Checkpoint token,Progress progress) throws IOException {
        if(zip.length()>MAX_COMPRESSED)throw new IOException("Driver ZIP exceeds 64 MB");
        List<File> files=new ArrayList<>(); Set<String> seen=new HashSet<>(); long total=0;int count=0;
        try(ZipInputStream in=new ZipInputStream(new FileInputStream(zip))) {
            ZipEntry entry;
            while((entry=in.getNextEntry())!=null) {
                token.check();if(++count>128)throw new IOException("Too many files in driver ZIP");
                File file=contained(root,entry.getName());
                if(!seen.add(file.getPath()))throw new IOException("Duplicate archive path");
                if(entry.isDirectory()){if(!file.isDirectory() && !file.mkdirs())throw new IOException("Cannot create driver directory");continue;}
                if(!file.getParentFile().isDirectory() && !file.getParentFile().mkdirs())throw new IOException("Cannot create driver directory");
                try(OutputStream out=new FileOutputStream(file)) {
                    byte[] buffer=new byte[32768];int n,empty=0;
                    while(true) {
                        token.check();n=in.read(buffer);token.check();if(n<0)break;
                        if(n==0){if(++empty>128)throw new IOException("Driver stream stalled");continue;}empty=0;
                        total+=n;if(total>MAX_EXPANDED)throw new IOException("Expanded driver exceeds 256 MB");
                        out.write(buffer,0,n);progress.update(total);
                    }
                }
                files.add(file);in.closeEntry();
            }
        }
        if(files.isEmpty())throw new IOException("Driver ZIP is empty");return files;
    }
    static void validateElf(File file) throws IOException {
        byte[] h=new byte[64];
        try(DataInputStream in=new DataInputStream(new FileInputStream(file))){in.readFully(h);}
        if(h[0]!=0x7f || h[1]!='E' || h[2]!='L' || h[3]!='F' || h[4]!=2 || h[5]!=1 || h[6]!=1 || h[16]!=3 || h[17]!=0 || (h[18]&255)!=183 || h[19]!=0)
            throw new IOException("Driver must be an ARM64 ELF shared library");
    }
    static void deleteTree(File file) {
        if(file==null)return;File[] children=file.listFiles();if(children!=null)for(File child:children)deleteTree(child);file.delete();
    }
}
