#!/usr/bin/env python3
"""Run the production post-import and ready-screen actions without launching SDL."""
from pathlib import Path
import os, subprocess, tempfile
repo=Path(__file__).resolve().parents[3]
source=(repo/'android/app/src/main/java/com/ptport/app/SetupActivity.java').read_text()
def method(signature):
    start=source.index(signature);end=source.index('{',start)+1;depth=1
    while depth:
        depth+=(source[end]=='{')-(source[end]=='}');end+=1
    return source[start:end]
code=r'''
import java.util.*;
import java.util.function.Consumer;
public class SetupMenuTest {
 int starts=0,checks=0;List<Button> buttons=new ArrayList<>();
 class Panel {void removeAllViews(){buttons.clear();}} Panel panel=new Panel();
 class TextView {void setTextSize(int n){}void setPadding(int a,int b,int c,int d){}}
 class Button {String label;Consumer<Button> action;Button(String l,Consumer<Button> a){label=l;action=a;}void requestFocus(){}void click(){action.accept(this);}}
 class MobileSettings {void show(){}void deleteSaves(){}} MobileSettings mobileSettings=new MobileSettings();
 static class Uri {}
 static class DocumentsContract {static boolean deleted=false,fail=false;static int calls=0;
  static boolean deleteDocument(Object resolver,Uri uri){calls++;if(fail)throw new RuntimeException();return deleted;}}
 Button button(String l,Consumer<Button> a){Button b=new Button(l,a);buttons.add(b);return b;}
 Button named(String l){return buttons.stream().filter(b->b.label.equals(l)).findFirst().orElseThrow();}
 TextView text(String t){return new TextView();}void brand(){}void title(String t){}
 void startGame(){starts++;}void openGithub(){}void finish(){}int dp(int n){return n;}
 String versionName(){return "test";}Object getContentResolver(){return this;}
 void ok(boolean condition,String why){if(!condition)throw new AssertionError(why);checks++;}
 METHODS
 public static void main(String[] a) {
  SetupMenuTest s=new SetupMenuTest();Uri pkg=new Uri();
  s.showDone(pkg);s.ok(s.starts==0,"import completion does not launch the game");
  s.named("Enter menu").click();s.ok(s.starts==0,"enter menu does not launch the game");
  s.named("Settings").click();s.ok(s.starts==0,"settings remains accessible before first game start");
  s.named("Start").click();s.ok(s.starts==1,"only Start launches the game");
  DocumentsContract.deleted=true;s.showDone(pkg);s.named("Delete the .pkg and enter menu").click();
  s.ok(DocumentsContract.calls==1&&s.starts==1,"successful package deletion enters menu without auto-start");s.named("Start");
  DocumentsContract.deleted=false;s.showDone(pkg);s.named("Delete the .pkg and enter menu").click();
  s.ok(s.starts==1,"provider refusal does not start game");s.named("Enter menu").click();s.named("Start");
  DocumentsContract.fail=true;s.showDone(pkg);s.named("Delete the .pkg and enter menu").click();
  s.ok(s.starts==1,"provider exception leaves Enter menu available");s.named("Enter menu").click();s.named("Start");
  s.ok(s.buttons.size()==5,"ready screen retains Start, Settings, Delete saves, GitHub and Quit");
  System.out.println("Post-import menu checks passed: "+s.checks);
 }
}
'''.replace('METHODS',method('private void showReady()')+'\n'+method('private void showDone('))
java=Path(os.environ.get('JAVA_HOME','/usr/lib/jvm/java-17-openjdk-amd64'))/'bin'
with tempfile.TemporaryDirectory() as folder:
    root=Path(folder);file=root/'SetupMenuTest.java';file.write_text(code)
    subprocess.run([str(java/'javac'),str(file)],check=True,timeout=30)
    subprocess.run([str(java/'java'),'-cp',str(root),'SetupMenuTest'],check=True,timeout=10)
