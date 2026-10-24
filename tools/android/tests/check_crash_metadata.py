#!/usr/bin/env python3
"""Exercise installed-APK identity and Java crash timestamps before native startup."""
from pathlib import Path
import os
import shutil
import subprocess
import tempfile

repo=Path(__file__).resolve().parents[3]
java=Path(os.environ.get("JAVA_HOME","/usr/lib/jvm/java-17-openjdk-amd64"))/"bin"
sources={
"android/app/Application.java":'''package android.app;public class Application{public static String getProcessName(){return "com.ptport.app.test:game";}}''',
"android/content/pm/PackageInfo.java":'''package android.content.pm;public class PackageInfo{public String versionName="0.2.1-test.23";public long getLongVersionCode(){return 66;}}''',
"android/content/pm/PackageManager.java":'''package android.content.pm;public class PackageManager{public boolean fail;public PackageInfo getPackageInfo(String name,int flags)throws Exception{if(fail)throw new Exception("unavailable");return new PackageInfo();}}''',
"android/content/res/AssetManager.java":'''package android.content.res;import java.io.*;public class AssetManager{public String stamp="# source-fixture23 20261024T000000Z\\nshaders/example.spv\\n";public boolean fail;public InputStream open(String name)throws IOException{if(!name.equals("pt_assets.txt")||fail)throw new IOException("unavailable");return new ByteArrayInputStream(stamp.getBytes(java.nio.charset.StandardCharsets.UTF_8));}}''',
"android/content/Context.java":'''package android.content;public class Context{public android.content.pm.PackageManager packages=new android.content.pm.PackageManager();public android.content.res.AssetManager assets=new android.content.res.AssetManager();public String getPackageName(){return "com.ptport.app.test";}public android.content.pm.PackageManager getPackageManager(){return packages;}public android.content.res.AssetManager getAssets(){return assets;}}''',
"com/ptport/app/CrashMetadataTest.java":r'''package com.ptport.app;
import android.content.Context;import java.time.Instant;public class CrashMetadataTest{
 static void ok(boolean value){if(!value)throw new AssertionError();}
 public static void main(String[]args){Context c=new Context();String report=CrashMetadata.report(c);
 ok(report.contains("package com.ptport.app.test\npt4droid 0.2.1-test.23 (code 66)\nassets source-fixture23 20261024T000000Z\n"));
 ok(report.contains("process com.ptport.app.test:game"));ok(!report.contains("example.spv"));
 String time=report.substring(report.indexOf("timestamp_utc ")+14).split("\n")[0];ok(Math.abs(Instant.now().getEpochSecond()-Instant.parse(time).getEpochSecond())<30);
 c.assets.fail=true;ok(CrashMetadata.identity(c).contains("assets stamp unavailable"));
 c.packages.fail=true;ok(CrashMetadata.identity(c).contains("version unavailable"));
 c.assets.fail=false;c.assets.stamp="# "+"x".repeat(10000);ok(CrashMetadata.identity(c).length()<300);
 c.assets.stamp="not a manifest header\n";ok(!CrashMetadata.identity(c).contains("not a manifest header"));
 System.out.println("APK identity/crash metadata: package/version/code/source stamp, UTC timestamp, process, missing metadata and bounded header passed");
 }
}'''
}
with tempfile.TemporaryDirectory()as directory:
    root=Path(directory)
    for name,source in sources.items():
        p=root/name;p.parent.mkdir(parents=True,exist_ok=True);p.write_text(source)
    shutil.copy2(repo/'android/app/src/main/java/com/ptport/app/CrashMetadata.java',root/'com/ptport/app/CrashMetadata.java')
    subprocess.run([str(java/'javac'),'-d',str(root/'classes')]+[str(p)for p in root.rglob('*.java')],check=True,timeout=30)
    subprocess.run([str(java/'java'),'-cp',str(root/'classes'),'com.ptport.app.CrashMetadataTest'],check=True,timeout=15)
