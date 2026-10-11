package com.ptport.app;

import android.app.AlertDialog;
import android.content.Context;
import android.content.res.ColorStateList;
import android.graphics.Color;
import android.graphics.Typeface;
import android.graphics.drawable.GradientDrawable;
import android.graphics.drawable.StateListDrawable;
import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.TextView;

/** Quiet, readable horror menus with a red focused row. No continuous UI animation. */
final class MenuStyle {
    static final int BACKGROUND=Color.rgb(12,13,15),TEXT=Color.rgb(231,228,219),MUTED=Color.rgb(167,164,159),RED=Color.rgb(151,35,36);
    static int dp(Context c,float value){return Math.round(value*c.getResources().getDisplayMetrics().density);}
    static GradientDrawable background(){return new GradientDrawable(GradientDrawable.Orientation.TL_BR,new int[]{Color.rgb(4,5,6),Color.rgb(26,27,30),Color.rgb(7,8,10)});}
    static void button(Button b) {
        Context c=b.getContext();b.setAllCaps(false);b.setTextColor(TEXT);b.setTextSize(17);b.setTypeface(Typeface.create("serif",Typeface.NORMAL));
        b.setGravity(Gravity.START|Gravity.CENTER_VERTICAL);b.setPadding(dp(c,20),0,dp(c,12),0);b.setMinHeight(dp(c,48));b.setMinimumHeight(dp(c,48));b.setFocusable(true);
        StateListDrawable states=new StateListDrawable();
        states.addState(new int[]{-android.R.attr.state_enabled},row(c,false,true));
        states.addState(new int[]{android.R.attr.state_pressed},row(c,true,false));
        states.addState(new int[]{android.R.attr.state_focused},row(c,true,false));
        states.addState(new int[]{android.R.attr.state_hovered},row(c,true,false));
        states.addState(new int[]{},row(c,false,false));b.setBackground(states);
    }
    private static GradientDrawable row(Context c,boolean active,boolean disabled){
        GradientDrawable d=new GradientDrawable(GradientDrawable.Orientation.LEFT_RIGHT,active?new int[]{Color.rgb(115,26,28),Color.rgb(60,21,23),Color.rgb(19,15,18)}:new int[]{Color.rgb(22,23,26),Color.rgb(15,16,19)});
        d.setCornerRadius(dp(c,3));d.setStroke(dp(c,1),active?RED:Color.rgb(40,40,43));if(disabled)d.setAlpha(100);return d;
    }
    static ColorStateList accent(){return ColorStateList.valueOf(RED);}
    static void section(android.widget.LinearLayout parent,String label){
        TextView t=new TextView(parent.getContext());t.setText(label.toUpperCase(java.util.Locale.ROOT));t.setTextColor(MUTED);t.setTextSize(12);t.setLetterSpacing(.15f);
        t.setPadding(0,dp(t.getContext(),20),0,dp(t.getContext(),10));parent.addView(t);
    }
    static void styleDialog(AlertDialog d){
            if(d.getWindow()!=null)d.getWindow().setBackgroundDrawable(row(d.getContext(),false,false));
            for(int id:new int[]{AlertDialog.BUTTON_POSITIVE,AlertDialog.BUTTON_NEGATIVE,AlertDialog.BUTTON_NEUTRAL}){
                Button b=d.getButton(id);if(b!=null){b.setTextColor(TEXT);b.setAllCaps(false);}
            }
    }
}
