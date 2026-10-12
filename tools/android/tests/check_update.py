#!/usr/bin/env python3
"""Execute production importer/archive/save code with file-provider boundary stubs."""
from pathlib import Path
import tempfile,subprocess,os,shutil
repo=Path(__file__).resolve().parents[3]
workspace=repo.parent
java_home=Path(os.environ.get('JAVA_HOME',str(workspace/'toolchain/host/usr/lib/jvm/java-17-openjdk-amd64')))
jar=Path(os.environ.get('PT_TEST_JSON_JAR',str(workspace/'recovery/json.jar')))
driver=Path(os.environ.get('PT_TEST_DRIVER_SO',str(workspace/'recovery/apk/lib/arm64-v8a/libvulkan_turnip.so')))
with tempfile.TemporaryDirectory() as tmp:
 t=Path(tmp)
 stubs={
 'com/ptport/app/AppLog.java':'''package com.ptport.app;import android.content.Context;public final class AppLog{static void append(Context c,String name,String text){}}''',
 'android/content/Context.java':'''package android.content;import java.io.File;public abstract class Context{public Context getApplicationContext(){return this;}public abstract File getFilesDir();public File getExternalFilesDir(String x){return getFilesDir();}public abstract ContentResolver getContentResolver();}''',
 'android/content/ContentResolver.java':'''package android.content;import android.net.Uri;import android.os.CancellationSignal;import android.content.res.AssetFileDescriptor;import java.io.IOException;public abstract class ContentResolver{public abstract AssetFileDescriptor openAssetFileDescriptor(Uri u,String m,CancellationSignal c)throws IOException;}''',
 'android/content/res/AssetFileDescriptor.java':'''package android.content.res;import java.io.*;public class AssetFileDescriptor implements AutoCloseable{public final File file;public AssetFileDescriptor(File f){file=f;}public long getLength(){return file.length();}public InputStream createInputStream()throws IOException{return new FileInputStream(file);}public void close()throws IOException{}}''',
 'android/net/Uri.java':'''package android.net;public class Uri{}''',
 'android/os/Build.java':'''package android.os;public class Build{public static class VERSION{public static final int SDK_INT=35;}}''',
 'android/os/CancellationSignal.java':'''package android.os;public class CancellationSignal{public volatile boolean cancelled;public void cancel(){cancelled=true;}}''',
 'com/ptport/app/UpdateTest.java':r'''package com.ptport.app;
import java.io.*;import java.nio.file.*;import java.util.*;import java.util.zip.*;import org.json.JSONObject;import android.content.*;import android.content.res.*;import android.net.*;import android.os.*;
public class UpdateTest {
 static int checks=0;static void ok(boolean b,String why){if(!b)throw new AssertionError(why);checks++;}
 interface Op{void run()throws Exception;}static void fails(Op op,String why)throws Exception{try{op.run();throw new AssertionError(why);}catch(IOException expected){checks++;}}
 static byte[] so;static File root;
 static File zip(String name,String library)throws Exception{return zipMeta(name,"{\"name\":\"Test Turnip\",\"minApi\":29,\"libraryName\":\""+library+"\"}");}
 static File zipMeta(String name,String metadata)throws Exception{File z=new File(root,name);try(ZipOutputStream out=new ZipOutputStream(new FileOutputStream(z))){out.putNextEntry(new ZipEntry("nested/meta.json"));out.write(metadata.getBytes());out.closeEntry();out.putNextEntry(new ZipEntry("nested/driver.so"));out.write(so);out.closeEntry();}return z;}
 static class Ctx extends Context{File dir,zip;boolean linkError,block;Ctx(File d,File z){dir=d;zip=z;}public File getFilesDir(){return dir;}public ContentResolver getContentResolver(){return new ContentResolver(){public AssetFileDescriptor openAssetFileDescriptor(Uri u,String m,CancellationSignal c)throws IOException{if(linkError)throw new LinkageError("provider failure");while(block && !c.cancelled)try{Thread.sleep(5);}catch(InterruptedException e){throw new IOException(e);}if(c.cancelled)throw new IOException("cancelled");return new AssetFileDescriptor(zip);}};}}
 static void await(DriverImportJob j)throws Exception{long end=System.nanoTime()+10_000_000_000L;while(!j.done && System.nanoTime()<end)Thread.sleep(10);ok(j.done,"job completed");}
 public static void main(String[] args)throws Exception{
  root=Files.createTempDirectory("pt-import-test").toFile();so=Files.readAllBytes(Paths.get(args[0]));File stage=new File(root,"stage");stage.mkdir();
  fails(()->DriverArchive.contained(stage,"../bad"),"traversal");fails(()->DriverArchive.contained(stage,"/bad"),"absolute path");fails(()->DriverArchive.contained(stage,"a\\b"),"backslash");fails(()->DriverArchive.contained(stage,"a:b"),"colon");fails(()->DriverArchive.contained(stage,"a/./b"),"dot path");
  File valid=zip("valid.zip","driver.so");List<File> files=DriverArchive.extract(valid,stage,()->{},n->{});ok(files.size()==2,"nested ZIP accepted");DriverArchive.validateElf(new File(stage,"nested/driver.so"));checks++;
  fails(()->DriverArchive.extract(valid,new File(root,"cancel"),()->{throw new IOException("cancelled");},n->{}),"cancel check");
  byte[] wrong=so.clone();wrong[18]=62;File bad=new File(root,"x86.so");Files.write(bad.toPath(),wrong);fails(()->DriverArchive.validateElf(bad),"wrong architecture");
  File malicious=new File(root,"escape.zip");try(ZipOutputStream out=new ZipOutputStream(new FileOutputStream(malicious))){out.putNextEntry(new ZipEntry("../escaped"));out.write(1);}fails(()->DriverArchive.extract(malicious,new File(root,"escape-stage"),()->{},n->{}),"ZIP containment");ok(!new File(root,"escaped").exists(),"no escaped file");
  File real=new File(root,"real/files");real.mkdirs();File alias=new File(root,"alias");Files.createSymbolicLink(alias.toPath(),real.toPath());Ctx c=new Ctx(alias,valid);
  File oldStage=new File(real,"drivers/.staging-abandoned"),oldZip=new File(real,"drivers/.import-abandoned.zip"),keep=new File(real,"drivers/installed-driver");oldStage.mkdirs();keep.mkdirs();Files.write(new File(oldStage,"part.so").toPath(),so);Files.write(oldZip.toPath(),so);Files.write(new File(keep,"driver.so").toPath(),so);
  DriverImportJob.cleanupAbandoned(c);ok(!oldStage.exists() && !oldZip.exists(),"fresh game removes abandoned import work");ok(new File(keep,"driver.so").exists(),"cleanup preserves installed drivers");
  DriverImportJob job=DriverImportJob.start(c,new Uri());await(job);ok(job.success,job.result);String selection=Files.readString(new File(real,"driver.txt").toPath()).trim();ok(!selection.contains(".."),"relative library path canonical");ok(selection.endsWith("/nested/driver.so"),"nested relative library preserved");String id=selection.split("/")[1];File info=new File(real,"drivers/"+id+"/package.json");JSONObject data=new JSONObject(Files.readString(info.toPath()));ok(data.getString("library").equals("nested/driver.so"),"metadata canonical");data.put("library","../../broken");Files.writeString(info.toPath(),data.toString());
  job=DriverImportJob.start(c,new Uri());await(job);ok(job.success,"same ZIP reimports");ok(Files.readString(new File(real,"driver.txt").toPath()).trim().equals(selection),"same package ID reused");ok(new JSONObject(Files.readString(info.toPath())).getString("library").equals("nested/driver.so"),"bad old metadata repaired");
  c.zip=zip("badmeta.zip","../outside.so");job=DriverImportJob.start(c,new Uri());await(job);ok(!job.success,"bad metadata rejected");ok(Files.readString(new File(real,"driver.txt").toPath()).trim().equals(selection),"selection preserved after failure");
  c.linkError=true;job=DriverImportJob.start(c,new Uri());await(job);ok(!job.success && job.result.contains("LinkageError"),"provider linkage failure visible");c.linkError=false;
  c.block=true;job=DriverImportJob.start(c,new Uri());Thread.sleep(25);job.cancel("test cancellation");await(job);ok(!job.success && job.result.equals("test cancellation"),"cancel completes UI immediately");Thread.sleep(50);ok(Files.readString(new File(real,"driver.txt").toPath()).trim().equals(selection),"cancel cannot activate late");
  c.block=true;job=DriverImportJob.start(c,new Uri());long until=System.nanoTime()+65_000_000_000L;while(!job.done && System.nanoTime()<until)Thread.sleep(20);ok(job.done && !job.success && job.result.contains("timed out after 60 seconds"),"absolute timeout releases blocked provider");ok(Files.readString(new File(real,"driver.txt").toPath()).trim().equals(selection),"timeout preserves selection");
  File oversized=new File(root,"too-big.zip");try(RandomAccessFile out=new RandomAccessFile(oversized,"rw")){out.setLength(DriverArchive.MAX_COMPRESSED+1);}fails(()->DriverArchive.extract(oversized,new File(root,"large"),()->{},n->{}),"compressed bound");
  c.block=false;c.zip=zipMeta("panvk.zip","{\"name\":\"PanVK Kbase\",\"backend\":\"panvk-kbase\",\"abi\":\"android-aarch64-bionic\",\"minApi\":35,\"libraryName\":\"driver.so\"}");job=DriverImportJob.start(c,new Uri());await(job);ok(job.success&&job.result.contains("Experimental Mali"),"PanVK import marked experimental");String mali=Files.readString(new File(real,"driver.txt").toPath()).trim();String maliId=mali.split("/")[1];JSONObject maliInfo=new JSONObject(Files.readString(new File(real,"drivers/"+maliId+"/package.json").toPath()));ok(maliInfo.getBoolean("experimental")&&maliInfo.getInt("minApi")==35,"experimental and API metadata retained");
  c.zip=zipMeta("glibc.zip","{\"name\":\"PanVK\",\"abi\":\"linux-aarch64-glibc\",\"libraryName\":\"driver.so\"}");job=DriverImportJob.start(c,new Uri());await(job);ok(!job.success&&job.result.contains("Linux/glibc"),"Linux driver rejected");ok(Files.readString(new File(real,"driver.txt").toPath()).trim().equals(mali),"Linux failure preserves working selection");
  c.zip=zipMeta("new-api.zip","{\"name\":\"PanVK\",\"minApi\":99,\"libraryName\":\"driver.so\"}");job=DriverImportJob.start(c,new Uri());await(job);ok(!job.success&&job.result.contains("newer Android"),"driver API gate enforced");
  File duplicates=new File(root,"duplicates.zip");try(ZipOutputStream out=new ZipOutputStream(new FileOutputStream(duplicates))){out.putNextEntry(new ZipEntry("a//b"));out.write(1);out.closeEntry();out.putNextEntry(new ZipEntry("a/b"));out.write(2);}fails(()->DriverArchive.extract(duplicates,new File(root,"duplicate-stage"),()->{},n->{}),"duplicate canonical entries");
  File saves=new File(root,"saves");saves.mkdir();for(String name:new String[]{"PT_Save_Data0","PT_Save_Data1","pt.ini","game.bin","driver.txt"})Files.writeString(new File(saves,name).toPath(),"keep");ok(SaveFiles.delete(saves)==2,"two actual slots deleted");ok(new File(saves,"pt.ini").isFile() && new File(saves,"game.bin").isFile() && new File(saves,"driver.txt").isFile(),"unrelated data retained");ok(SaveFiles.delete(saves)==0,"empty saves succeeds");
  System.out.println("Archive/import/save checks passed: "+checks);
  DriverArchive.deleteTree(root);
 }
}'''
 }
 for name,source in stubs.items():p=t/name;p.parent.mkdir(parents=True,exist_ok=True);p.write_text(source)
 production=repo/'android/app/src/main/java/com/ptport/app'
 for name in ['DriverArchive.java','DriverImportJob.java','SaveFiles.java']:shutil.copy2(production/name,t/'com/ptport/app'/name)
 subprocess.run([str(java_home/'bin/javac'),'-cp',str(jar),'-d',str(t/'classes')]+[str(p)for p in t.rglob('*.java')],check=True)
 subprocess.run([str(java_home/'bin/java'),'-cp',str(t/'classes')+os.pathsep+str(jar),'com.ptport.app.UpdateTest',str(driver)],check=True)
