package com.ptport.app;

import android.view.*;
import android.widget.*;

/** Compact camera controls beside the touch Settings button. */
final class CameraControls extends HorizontalScrollView {
    private final LinearLayout row;
    private final Button free,photo,third,shot,panel,up,down,hide;
    private boolean expanded=true;
    private boolean freeActive,photoActive,usable;
    CameraControls(PTActivity activity){
        super(activity);setFillViewport(false);setHorizontalScrollBarEnabled(false);setBackgroundColor(0x99303030);
        row=new LinearLayout(activity);row.setOrientation(LinearLayout.HORIZONTAL);addView(row);
        free=button("Freecam",()->PTActivity.nativeCameraAction(1));photo=button("Photo",()->PTActivity.nativeCameraAction(2));third=button("Third person",()->PTActivity.nativeCameraAction(4));shot=button("Shutter",()->PTActivity.nativeCameraAction(16));
        panel=button("Panel",()->PTActivity.nativeCameraAction(8));
        up=button("Up",()->{});down=button("Down",()->{});rise(up,1);rise(down,-1);
        hide=button("Hide",()->{expanded=!expanded;update();});update();
    }
    private Button button(String title,Runnable action){Button b=new Button(getContext());b.setText(title);MenuStyle.button(b);b.setTextSize(12);b.setMinWidth(0);b.setMinimumWidth(0);b.setMinHeight(0);b.setMinimumHeight(0);b.setGravity(Gravity.CENTER);b.setPadding(MenuStyle.dp(getContext(),8),0,MenuStyle.dp(getContext(),8),0);b.setOnClickListener(v->action.run());row.addView(b,new LinearLayout.LayoutParams(-2,-1));return b;}
    private void rise(Button button,float amount){button.setOnTouchListener((v,event)->{switch(event.getActionMasked()){case MotionEvent.ACTION_DOWN:PTActivity.nativeCameraRise(amount);return true;case MotionEvent.ACTION_UP:case MotionEvent.ACTION_CANCEL:PTActivity.nativeCameraRise(0);v.performClick();return true;default:return true;}});}
    void state(int bits){usable=(bits&8)!=0;freeActive=(bits&1)!=0;photoActive=(bits&2)!=0;free.setText(freeActive?"Freecam: on":"Freecam");photo.setText(photoActive?"Photo: on":"Photo");third.setText((bits&4)!=0?"First person":"Third person");update();}
    void allowed(boolean shown){if(!shown)PTActivity.nativeCameraRise(0);setVisibility(shown?VISIBLE:GONE);}
    void clear(){PTActivity.nativeCameraRise(0);PTActivity.nativeCameraAction(0);}
    private void update(){for(Button b:new Button[]{free,photo,third,shot,panel,up,down})b.setEnabled(usable);for(Button b:new Button[]{free,photo,third})b.setVisibility(expanded?VISIBLE:GONE);shot.setVisibility(expanded&&photoActive?VISIBLE:GONE);panel.setVisibility(expanded&&photoActive?VISIBLE:GONE);up.setVisibility(expanded&&freeActive?VISIBLE:GONE);down.setVisibility(expanded&&freeActive?VISIBLE:GONE);hide.setText(expanded?"Hide":"Camera");if(!expanded)PTActivity.nativeCameraRise(0);}
}
