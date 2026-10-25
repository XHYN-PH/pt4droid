#!/usr/bin/env python3
"""Run the production log-tail export and resolution caption with file fixtures."""
from pathlib import Path
import os, subprocess, tempfile

repo = Path(__file__).resolve().parents[3]
def extract(path, signature):
    text = (repo / path).read_text()
    start = text.index(signature)
    end = text.index('{', start) + 1
    depth = 1
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    return text[start:end]

methods = '\n'.join(extract('android/app/src/main/java/com/ptport/app/Diagnostics.java', s)
                    for s in ['private static void entry(', 'private static void copy('])
methods += '\n' + extract('android/app/src/main/java/com/ptport/app/SettingsPages.java',
                          'private String resolutionCaption(')
fixture = r'''
import java.io.*;import java.nio.file.*;import java.util.*;import java.util.zip.*;
class Export24Test {
 Map<String,Object> values=new HashMap<>();
 METHODS
 static void ok(boolean value){if(!value)throw new AssertionError();}
 public static void main(String[]args)throws Exception{
  Path root=Path.of(args[0]);File log=root.resolve("pt.log").toFile();
  Files.writeString(log.toPath(),"header\n"+"x".repeat(12000)+"\nlast\n");
  Path out=root.resolve("out.zip");
  try(ZipOutputStream z=new ZipOutputStream(Files.newOutputStream(out))){
   copy(z,log,"logs/0-pt.log",2048);copy(z,new File(root.toFile(),"missing"),"logs/missing",2048);
  }
  try(ZipFile z=new ZipFile(out.toFile())){
   String tail=new String(z.getInputStream(z.getEntry("logs/0-pt.log")).readAllBytes());
   String meta=new String(z.getInputStream(z.getEntry("logs/0-pt.log.meta.txt")).readAllBytes());
   ok(tail.length()==2048&&tail.endsWith("\nlast\n"));
   ok(meta.contains("original_bytes=12013")&&meta.contains("captured_bytes=2048")&&meta.contains("truncated=true"));
   ok(z.getEntry("logs/missing")==null);
  }
  Files.writeString(log.toPath(),"small\n");
  try(ZipOutputStream z=new ZipOutputStream(Files.newOutputStream(out))){copy(z,log,"logs/0-pt.log",2048);}
  try(ZipFile z=new ZipFile(out.toFile())){
   String meta=new String(z.getInputStream(z.getEntry("logs/0-pt.log.meta.txt")).readAllBytes());
   ok(meta.contains("truncated=false")&&meta.contains("omitted_prefix_bytes=0"));
  }
  Export24Test t=new Export24Test();t.values.put("output_scale_index",1);
  ok(t.resolutionCaption(.5f).contains("45%"));
  t.values.put("output_scale_index",2);ok(t.resolutionCaption(.5f).contains("40%"));
  t.values.put("output_scale_index",0);ok(t.resolutionCaption(1f).contains("100%"));
  System.out.println("Test24 diagnostic export: exact bounded tail, truncation metadata, missing files and combined-resolution caption passed");
 }
}
'''.replace('METHODS', methods)
jdk = Path(os.environ.get('JAVA_HOME', '/workspace/toolchains/jdk-17.0.16+8')) / 'bin'
with tempfile.TemporaryDirectory() as directory:
    root = Path(directory)
    source = root / 'Export24Test.java'
    source.write_text(fixture)
    subprocess.run([str(jdk / 'javac'), '-d', str(root), str(source)], check=True, timeout=30)
    subprocess.run([str(jdk / 'java'), '-cp', str(root), 'Export24Test', str(root)], check=True, timeout=15)
