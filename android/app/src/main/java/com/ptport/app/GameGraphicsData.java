package com.ptport.app;

import android.content.Context;
import java.io.File;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.util.LinkedHashMap;
import java.util.Map;

/** Graphics-only INI overlay. The native settings parser supplies validation and persistence. */
final class GameGraphicsData {
    final Map<String,String> values=new LinkedHashMap<>();
    boolean rayTracing;
    String rayReason="Start the game to check the selected driver's ray-query support.";
    GameGraphicsData(){original();put("camera.third_person",0);put("graphics.shadow_quality",2);put("graphics.ambient_occlusion",0);put("graphics.motion_blur",0);put("graphics.depth_of_field",0);put("display.vsync",1);put("display.fps_limit",0);put("display.letterbox",0);}
    void original(){
        put("graphics.anisotropy",0);put("graphics.enhanced_textures",0);put("graphics.shadow_quality",3);
        for(String key:new String[]{"ambient_occlusion","reflections","bloom","depth_of_field","motion_blur","lens_distortion","lens_ghosts"})put("graphics."+key,1);
        put("graphics.film_grain",1f);put("graphics.clarity",0f);put("graphics.texture_detail",1);put("graphics.ray_quality",1);
        put("raytracing.shadows",0);put("raytracing.ambient_occlusion",0);put("raytracing.contact_shadows",0);put("raytracing.reflections",0);
    }
    void preset(int profile){
        if(profile<1 || profile>3)return;if(profile==3){original();return;}
        put("graphics.enhanced_textures",0);
        for(String key:new String[]{"shadows","ambient_occlusion","contact_shadows","reflections"})put("raytracing."+key,0);
        put("display.fps_limit",profile==1?0:60);put("display.vsync",0);put("graphics.shadow_quality",profile==1?1:2);
        put("graphics.anisotropy",profile==1?0:2);put("graphics.texture_detail",profile==1?0:1);
        for(String key:new String[]{"ambient_occlusion","depth_of_field","motion_blur"})put("graphics."+key,0);
        for(String key:new String[]{"reflections","bloom","lens_distortion","lens_ghosts"})put("graphics."+key,profile==1?0:1);
        put("graphics.film_grain",profile==1?0f:1f);
    }
    void parse(String text){
        String section="";
        for(String line:text.split("\\r?\\n")){
            line=line.trim();if(line.isEmpty() || line.startsWith(";") || line.startsWith("#"))continue;
            if(line.startsWith("[") && line.endsWith("]")){section=line.substring(1,line.length()-1).trim();continue;}
            int eq=line.indexOf('=');if(eq<0)continue;
            String key=section+"."+line.substring(0,eq).trim(),value=line.substring(eq+1).trim();
            if(values.containsKey(key))values.put(key,value);
            if(key.equals("android_capabilities.ray_tracing"))rayTracing=value.equals("1");
            if(key.equals("android_capabilities.ray_reason") && !value.isEmpty())rayReason=value;
        }
    }
    static GameGraphicsData load(Context context,String snapshot)throws IOException {
        GameGraphicsData data=new GameGraphicsData();
        if(snapshot!=null && !snapshot.isEmpty())data.parse(snapshot);
        else {File external=context.getExternalFilesDir(null);File file=external==null?null:new File(external,"data/pt.ini");if(file==null || !file.isFile())file=new File(context.getFilesDir(),"data/pt.ini");if(file.isFile())data.parse(read(file));}
        File profile=new File(context.getFilesDir(),"performance_profile_pending.txt");
        if(profile.isFile())try{data.preset(Integer.parseInt(read(profile).trim()));}catch(NumberFormatException ignored){}
        File applying=new File(context.getFilesDir(),"game_graphics_applying.ini");if(applying.isFile())data.parse(read(applying));
        File pending=new File(context.getFilesDir(),"game_graphics_pending.ini");if(pending.isFile())data.parse(read(pending));
        return data;
    }
    private static String read(File file)throws IOException{return new String(Files.readAllBytes(file.toPath()),StandardCharsets.UTF_8);}
    int integer(String key,int fallback){try{return Integer.parseInt(values.get(key));}catch(Exception ignored){return fallback;}}
    float number(String key,float fallback){try{float v=Float.parseFloat(values.get(key));return Float.isFinite(v)?Math.max(0,Math.min(1,v)):fallback;}catch(Exception ignored){return fallback;}}
    boolean enabled(String key){String value=values.get(key);return "1".equals(value)||"true".equals(value)||"yes".equals(value)||"on".equals(value);}
    void put(String key,int value){values.put(key,Integer.toString(value));}
    void put(String key,float value){values.put(key,Float.toString(value));}
    String text(){
        StringBuilder out=new StringBuilder("; pt4droid graphics changes\n");
        for(String section:new String[]{"graphics","raytracing","display","camera"}){
            out.append('\n').append('[').append(section).append("]\n");
            for(Map.Entry<String,String> entry:values.entrySet())if(entry.getKey().startsWith(section+"."))out.append(entry.getKey().substring(section.length()+1)).append(" = ").append(entry.getValue()).append('\n');
        }
        return out.toString();
    }
    void save(Context context)throws IOException {
        File root=context.getFilesDir();
        DriverImportJob.atomicWrite(new File(root,"performance_profile_pending.txt"),"0\n".getBytes(StandardCharsets.UTF_8));
        DriverImportJob.atomicWrite(new File(root,"game_graphics_pending.ini"),text().getBytes(StandardCharsets.UTF_8));
        DriverImportJob.atomicWrite(new File(root,"performance_profile.txt"),"0\n".getBytes(StandardCharsets.UTF_8));
        context.getSharedPreferences("touch_layout_v1",0).edit().putInt("performance_profile",0).apply();
    }
}
