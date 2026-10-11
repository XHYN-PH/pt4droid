package com.ptport.app;

import android.app.Activity;
import android.app.AlertDialog;
import android.view.View;
import android.widget.*;
import java.util.ArrayList;
import java.util.List;

final class GameGraphicsSettings {
    private final Activity activity;
    private final GameGraphicsData data;
    private final LinearLayout column;
    private GameGraphicsSettings(Activity activity,GameGraphicsData data){this.activity=activity;this.data=data;column=MobileSettings.column(activity);}
    private void note(String value){TextView t=new TextView(activity);t.setText(value);t.setTextColor(MenuStyle.MUTED);t.setTextSize(14);t.setPadding(0,6,0,12);column.addView(t);}
    private void toggle(String title,String key,boolean available){CheckBox c=new CheckBox(activity);c.setText(title);c.setTextColor(MenuStyle.TEXT);c.setButtonTintList(MenuStyle.accent());c.setChecked(data.enabled(key));c.setEnabled(available);c.setOnCheckedChangeListener((v,on)->data.put(key,on?1:0));column.addView(c);}
    private void choice(String title,String key,String[] names,int[] numbers,boolean available){
        note(title);Spinner s=new Spinner(activity);s.setAdapter(new ArrayAdapter<>(activity,android.R.layout.simple_spinner_dropdown_item,names));
        int current=data.integer(key,numbers[0]),selected=0;for(int i=0;i<numbers.length;i++)if(numbers[i]==current)selected=i;
        s.setSelection(selected);s.setEnabled(available);column.addView(s);
        s.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener(){public void onNothingSelected(AdapterView<?> v){}public void onItemSelected(AdapterView<?> v,View row,int position,long id){data.put(key,numbers[position]);}});
    }
    private void amount(String title,String key){float current=data.number(key,0);TextView label=new TextView(activity);label.setTextColor(MenuStyle.TEXT);label.setText(title+": "+Math.round(current*100)+"%");column.addView(label);MobileSettings.slider(column,title,Math.round(current*100),100,value->{data.put(key,value/100f);label.setText(title+": "+value+"%");});}
    private void show(){
        MenuStyle.section(column,"Display");
        List<Integer> fps=new ArrayList<>();fps.add(0);float maximum=DisplayRefresh.maximum(activity);
        for(int rate:new int[]{30,40,45,60,90,120,144,165,240})if(rate<=maximum+.5f)fps.add(rate);
        int stored=data.integer("display.fps_limit",0);if(stored>0 && !fps.contains(stored))fps.add(stored);
        int[] rates=new int[fps.size()];String[] labels=new String[fps.size()];for(int i=0;i<rates.length;i++){rates[i]=fps.get(i);labels[i]=rates[i]==0?"Unlimited":rates[i]+" FPS";}
        choice("Frame limit","display.fps_limit",labels,rates,true);toggle("VSync","display.vsync",true);
        note("Display refresh rate is set separately in Android settings. A higher rate does not guarantee the game can render that many frames.");
        choice("Letterbox","display.letterbox",new String[]{"Off","Cinema • 2.39:1","Cinema • 1.85:1"},new int[]{0,1,2},true);
        MenuStyle.section(column,"Textures and lighting");
        choice("Texture detail","graphics.texture_detail",new String[]{"Reduced","Original","Fine"},new int[]{0,1,2},true);
        choice("Anisotropic filtering","graphics.anisotropy",new String[]{"Off / trilinear","2×","4×","8×","16×"},new int[]{0,2,4,8,16},true);
        choice("Shadow resolution","graphics.shadow_quality",new String[]{"Off","512 px","1024 px","Original • 2048 px"},new int[]{0,1,2,3},true);
        toggle("Ambient occlusion","graphics.ambient_occlusion",true);toggle("Local reflections","graphics.reflections",true);
        MenuStyle.section(column,"Effects and image");
        toggle("Bloom","graphics.bloom",true);toggle("Depth of field","graphics.depth_of_field",true);toggle("Motion blur","graphics.motion_blur",true);
        toggle("Lens distortion","graphics.lens_distortion",true);toggle("Lens flare ghosts","graphics.lens_ghosts",true);
        amount("Film grain","graphics.film_grain");amount("Clarity","graphics.clarity");
        MenuStyle.section(column,"Ray tracing");
        if(!data.rayTracing)note("Unavailable: "+data.rayReason);
        choice("Traced shadows","raytracing.shadows",new String[]{"Off","Sharp","Soft"},new int[]{0,1,2},data.rayTracing);
        toggle("Traced ambient occlusion","raytracing.ambient_occlusion",data.rayTracing);toggle("Traced contact shadows","raytracing.contact_shadows",data.rayTracing);toggle("Traced reflections","raytracing.reflections",data.rayTracing);
        choice("Ray quality","graphics.ray_quality",new String[]{"Balanced","High","Ultra"},new int[]{0,1,2},data.rayTracing);
        MenuStyle.section(column,"Build support");
        note("HDR, enhanced texture generation, desktop upscalers and frame generation are unavailable in this Android build. Scene resolution and optimized mobile bloom are in Android settings.");
        AlertDialog dialog=new AlertDialog.Builder(activity).setTitle("Game graphics").setView(MobileSettings.scroll(column)).setPositiveButton("Apply",null).setNegativeButton("Cancel",null).create();
        dialog.setOnShowListener(d->dialog.getButton(AlertDialog.BUTTON_POSITIVE).setOnClickListener(v->{
            try{data.save(activity);if(PTActivity.gameRunning())PTActivity.nativeApplyGameGraphicsSettings();dialog.dismiss();Toast.makeText(activity,PTActivity.gameRunning()?"Graphics saved. Applying to the game.":"Graphics saved for the next game start.",Toast.LENGTH_LONG).show();}
            catch(Exception error){new AlertDialog.Builder(activity).setTitle("Could not save graphics").setMessage(error.getMessage()).setPositiveButton("OK",null).show();}
        }));dialog.show();MenuStyle.styleDialog(dialog);
    }
    static void open(Activity activity){
        try{String snapshot=PTActivity.gameRunning()?PTActivity.nativeGetGameGraphicsSettings():"";new GameGraphicsSettings(activity,GameGraphicsData.load(activity,snapshot)).show();}
        catch(Exception error){new AlertDialog.Builder(activity).setTitle("Could not read graphics").setMessage(error.getMessage()).setPositiveButton("OK",null).show();}
    }
}
