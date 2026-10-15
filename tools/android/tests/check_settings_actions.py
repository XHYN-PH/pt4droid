#!/usr/bin/env python3
"""Execute production Apply/driver-retry actions against actual files and a native boundary."""
from pathlib import Path
import os, subprocess, tempfile
repo=Path(__file__).resolve().parents[3]
source=repo/'android/app/src/main/java/com/ptport/app'
settings=(source/'MobileSettings.java').read_text()
start=settings.index('                int selected=profile.getSelectedItemPosition();')
end=settings.index('                int compatibilitySize=',start)
apply=settings[start:end]+'\nif(presetChanged)PTActivity.nativeSetPerformanceProfile(selected);'
assert 'if(presetChanged)PTActivity.nativeSetPerformanceProfile(selected);' in settings
setup=(source/'SetupActivity.java').read_text()
start=setup.index('                try{DriverImportJob.atomicWrite(new File(getFilesDir(),"driver.txt")')
end=setup.index('\n            });',start)
retry=setup[start:end]
job=(source/'DriverImportJob.java').read_text();start=job.index('    static void atomicWrite(');end=job.index('{',start)+1;depth=1
while depth:
    depth+=(job[end]=='{')-(job[end]=='}');end+=1
atomic=job[start:end]
java=r'''import java.io.*;import java.nio.file.*;import java.nio.charset.StandardCharsets;
class PTActivity {static int cap=120,calls;static void nativeSetPerformanceProfile(int p){calls++;if(p==1)cap=0;if(p==2)cap=60;}}
class DriverImportJob {ATOMIC}
public class SettingsActionsTest {
 final File root;int starts;SettingsActionsTest(Path p){root=p.toFile();}File getFilesDir(){return root;}
 class Activity{File getFilesDir(){return root;}}final Activity activity=new Activity();
 class Profile{int value;int getSelectedItemPosition(){return value;}}final Profile profile=new Profile();
 class Check{boolean isChecked(){return true;}}final Check bloom=new Check();
 void write(String name,String value)throws IOException{DriverImportJob.atomicWrite(new File(root,name),(value+"\n").getBytes(StandardCharsets.UTF_8));}
 void runApply(int originalProfile,int chosen,boolean advanced)throws Exception{profile.value=chosen;float[] scale={.8f};APPLY}
 void startGame(){starts++;}void showError(String s){throw new AssertionError(s);}
 void retry(){File driverFailure=new File(root,"driver_startup_failed.txt");RETRY}
 static void ok(boolean value,String reason){if(!value)throw new AssertionError(reason);}
 public static void main(String[]args)throws Exception{
  Path p=Files.createTempDirectory("pt-settings-actions");SettingsActionsTest test=new SettingsActionsTest(p);
  for(int preset=0;preset<=3;preset++)for(boolean advanced:new boolean[]{false,true})for(int cap:new int[]{0,30,40,45,60,90,120,144,165,240}){
   Files.writeString(p.resolve("game_graphics_pending.ini"),"pending-custom");Files.writeString(p.resolve("game_graphics_applying.ini"),"applying-custom");Files.deleteIfExists(p.resolve("performance_profile_pending.txt"));
   PTActivity.cap=cap;PTActivity.calls=0;test.runApply(preset,preset,advanced);
   ok(PTActivity.cap==cap&&PTActivity.calls==0,"unchanged preset must preserve every selected cap");
   ok(Files.readString(p.resolve("game_graphics_pending.ini")).equals("pending-custom")&&Files.readString(p.resolve("game_graphics_applying.ini")).equals("applying-custom"),"unrelated Apply retains manual changes");
   ok(!Files.exists(p.resolve("performance_profile_pending.txt")),"unrelated Apply must not queue a preset for restart");
  }
  test.runApply(2,1,true);ok(PTActivity.cap==0&&PTActivity.calls==1,"explicit Maximum preset applies");
  ok(Files.readString(p.resolve("performance_profile_pending.txt")).trim().equals("1")&&!Files.exists(p.resolve("game_graphics_pending.ini")),"explicit preset replaces old graphics overlay");
  Files.createDirectories(p.resolve("data"));Files.writeString(p.resolve("data/save.dat"),"keep-progress");Files.writeString(p.resolve("driver_startup_failed.txt"),"startup failed");
  test.retry();ok(test.starts==1&&Files.readString(p.resolve("driver.txt")).trim().equals("system"),"retry selects System before starting");
  ok(!Files.exists(p.resolve("driver_startup_failed.txt"))&&Files.readString(p.resolve("data/save.dat")).equals("keep-progress"),"retry clears its marker and preserves saves");
  System.out.println("Settings actions passed: 80 unchanged-preset/FPS combinations; explicit preset; real System retry and save preservation");
 }
}'''.replace('ATOMIC',atomic).replace('APPLY',apply).replace('RETRY',retry)
with tempfile.TemporaryDirectory() as directory:
    root=Path(directory);file=root/'SettingsActionsTest.java';file.write_text(java)
    home=Path(os.environ['JAVA_HOME'])/'bin'
    subprocess.run([str(home/'javac'),'-d',str(root),str(file)],check=True,timeout=30)
    subprocess.run([str(home/'java'),'-cp',str(root),'SettingsActionsTest'],check=True,timeout=15)
