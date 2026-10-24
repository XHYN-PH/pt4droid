#!/usr/bin/env python3
"""Exercise production glare gating, filter descriptors and visible SGSR2 dependency."""
from pathlib import Path
import os,shlex,subprocess,tempfile
repo=Path(__file__).resolve().parents[3]
def extract(path,sig):
 base=Path(os.environ.get('PT_LIGHTING_CPP_ROOT',str(repo)))if path.endswith('.cpp')else repo
 s=(base/path).read_text();a=s.index(sig);i=s.index('{',a)+1;d=1
 while d:d+=(s[i]=='{')-(s[i]=='}');i+=1
 return s[a:i]
post=(Path(os.environ.get('PT_LIGHTING_CPP_ROOT',str(repo)))/'src/engine/render/scene_post.cpp').read_text()
a=post.index('    UseTargets(cmd, {{&flare_');b=post.index('    Stamp(cmd,22);',a)+len('    Stamp(cmd,22);')
flare=post[a:b]
a=post.index('    if (vfx_filter && vr_eye_ < 0) {',post.index('const bool merge_grade='));b=post.index('\n    vkCmdEndRendering(cmd);',a)
screen=post[a:b]
context=extract('src/engine/render/scene_renderer.h','struct SceneFilterContext')+';'
renderer=(Path(os.environ.get('PT_LIGHTING_CPP_ROOT',str(repo)))/'src/engine/render/renderer.cpp').read_text();a=renderer.index('#ifdef __ANDROID__',renderer.index('bool Renderer::BeginFrame('));b=renderer.index('#endif',a)+len('#endif');target_policy="\n"+renderer[a:b]+"\n"
methods=[extract('src/engine/render/renderer.cpp','bool Renderer::CreateTargets('),extract('src/engine/render/scene_post.cpp','SceneFilterContext SceneRenderer::FilterContext('),extract('src/engine/render/vfx_pass.cpp','void VfxPass::WriteSet('),extract('src/engine/render/vfx_pass.cpp','void VfxPass::RecordFilter(')]
prefix=(repo/'tests/lighting_filter21_test.cpp.in').read_text().replace('/* FILTER_CONTEXT */',context).replace('/* FLARE_BLOCK */',flare).replace('/* SCREEN_BLOCK */',screen).replace('/* TARGET_POLICY */',target_policy).replace('/* PRODUCTION_METHODS */','\n'.join(methods))
with tempfile.TemporaryDirectory()as tmp:
 t=Path(tmp);s=t/'lighting.cpp';s.write_text(prefix);binary=t/'lighting'
 cmd=[os.environ.get('CXX','g++'),'-std=c++20','-O1','-D__ANDROID__']+shlex.split(os.environ.get('PT_TEST_CXX_FLAGS',''))+['-I'+str(repo/p)for p in ['src','build/android/_deps/glm-src','build/android/_deps/vulkan_headers-src/include']]+[str(s),'-o',str(binary)]
 subprocess.run(cmd,check=True,timeout=90);subprocess.run([str(binary)],check=True,timeout=15)
 # Execute the actual production checkbox listeners with Android's change-only
 # callback behavior, including normalization of older inconsistent saved state.
 settings=(repo/'android/app/src/main/java/com/ptport/app/SettingsPages.java').read_text();a=settings.index('            CheckBox temporal=');b=settings.index('            note("Turning SGSR2',a);body=settings[a:b]
 body=body.replace('CheckBox temporal=check(', 'temporal=check(').replace('CheckBox dynamic=check(', 'dynamic=check(')
 java='''import java.util.*;
class DependencyTest {
 interface Listener{void changed(CheckBox b,boolean value);}
 static class CheckBox{boolean checked;Listener listener;CheckBox(boolean b){checked=b;}boolean isChecked(){return checked;}void setOnCheckedChangeListener(Listener l){listener=l;}void setChecked(boolean b){if(checked==b)return;checked=b;if(listener!=null)listener.changed(this,b);}}
 Map<String,Object> values=new HashMap<>();Map<String,Boolean>prefs=new HashMap<>();CheckBox temporal,dynamic;
 CheckBox check(String title,String key,boolean fallback){CheckBox c=new CheckBox(prefs.getOrDefault(key,fallback));values.put(key,c.checked);c.listener=(v,on)->values.put(key,on);return c;}
 void bind(){BODY}
 static void ok(boolean value){if(!value)throw new AssertionError();}
 public static void main(String[] args){for(boolean temporal:new boolean[]{false,true})for(boolean dynamic:new boolean[]{false,true}){
 DependencyTest t=new DependencyTest();t.prefs.put("temporal_upscaling",temporal);t.prefs.put("dynamic_resolution",dynamic);t.bind();ok(!t.dynamic.checked||t.temporal.checked);
 t.dynamic.setChecked(true);ok(t.temporal.checked&&Boolean.TRUE.equals(t.values.get("temporal_upscaling")));
 t.temporal.setChecked(false);ok(!t.dynamic.checked&&Boolean.FALSE.equals(t.values.get("dynamic_resolution")));
 t.temporal.setChecked(true);t.dynamic.setChecked(false);ok(t.temporal.checked);
 }System.out.println("SGSR2/DRS dependency: visible normalization, enabling DRS, disabling SGSR2 and independent SGSR2 all passed");}
}'''.replace('BODY',body)
 source=t/'DependencyTest.java';source.write_text(java);jdk=Path(os.environ.get('JAVA_HOME','/workspace/toolchains/jdk-17.0.16+8'))
 subprocess.run([str(jdk/'bin/javac'),'-d',str(t),str(source)],check=True,timeout=30);subprocess.run([str(jdk/'bin/java'),'-cp',str(t),'DependencyTest'],check=True,timeout=15)
