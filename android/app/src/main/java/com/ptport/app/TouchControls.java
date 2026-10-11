package com.ptport.app;

import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.SharedPreferences;
import android.content.res.Resources;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.RectF;
import android.graphics.Typeface;
import android.view.InputDevice;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;
import android.view.WindowInsets;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.CheckBox;
import android.widget.CompoundButton;
import android.widget.LinearLayout;
import android.widget.Spinner;
import android.widget.SpinnerAdapter;
import java.util.HashSet;
import java.util.function.IntConsumer;

/* loaded from: classes.dex */
public class TouchControls extends View {
    private static final int[] ACTION_BITS = {1, 8, 2, 4, 16};
    private static final String[] ACTION_NAMES = {"Use", "Zoom", "Back", "Pause", "Name"};
    private static final long HIDE_AFTER_MS = 5000;
    private final Control[] controls;
    private float deadZone;
    private float dragDx;
    private float dragDy;
    private int editPointer;
    private boolean editing;
    private final Paint fill;
    private boolean floating;
    private final Runnable hideIdle;
    private int insetBottom;
    private int insetLeft;
    private int insetRight;
    private int insetTop;
    private final Paint label;
    private int lastButtons;
    private float lastLx;
    private float lastLy;
    private float lastRx;
    private float lastRy;
    private final Stick look;
    private int mode;
    private final Stick move;
    private float opacity;
    private final SharedPreferences prefs;
    private final Paint ring;
    private final RectF safe;
    private int selected;
    private float sensitivity;
    private int lookStyle = 1;
    private float lookLastX, lookLastY;
    private static native void nativeLookDelta(float dx, float dy);
    private static native void nativeClearLook();
    private final RectF settingsHit;
    private boolean shown;
    private final RectF[] toolbar;
    private int toolbarPressed;

    static boolean isController(int i) {
        return (i & 1025) == 1025 || (i & 16777232) == 16777232;
    }

    private static native void nativeState(float f, float f2, float f3, float f4, int i);

    private static native void nativeVisible(boolean z);

    /* JADX INFO: Access modifiers changed from: private */
    static final class Control {
        int action;
        float nx;
        float ny;
        final RectF area = new RectF();
        final HashSet<Integer> fingers = new HashSet<>();
        float size = 1.0f;
        boolean visible = true;

        Control(int i) {
            this.action = i;
        }
    }

    private static final class Stick {
        float originX;
        float originY;
        int pointer;
        float x;
        float y;

        private Stick() {
            this.pointer = -1;
        }
    }

    /* renamed from: lambda$new$0$com-ptport-app-TouchControls, reason: not valid java name */
    /* synthetic */ void m28lambda$new$0$comptportappTouchControls() {
        if (this.mode != 0 || this.editing || anyFingerDown()) {
            return;
        }
        setShown(false);
    }

    public TouchControls(Context context) throws Resources.NotFoundException {
        super(context);
        this.controls = new Control[]{new Control(0), new Control(1), new Control(2), new Control(3), new Control(4), new Control(-1), new Control(-1)};
        this.move = new Stick();
        this.look = new Stick();
        this.safe = new RectF();
        this.settingsHit = new RectF();
        this.toolbar = new RectF[]{new RectF(), new RectF(), new RectF(), new RectF()};
        Paint paint = new Paint(1);
        this.ring = paint;
        this.fill = new Paint(1);
        Paint paint2 = new Paint(1);
        this.label = paint2;
        this.opacity = 0.7f;
        this.sensitivity = 1.0f;
        this.deadZone = 0.05f;
        this.floating = true;
        this.selected = -1;
        this.editPointer = -1;
        this.toolbarPressed = -1;
        this.lastButtons = -1;
        this.hideIdle = new Runnable() { // from class: com.ptport.app.TouchControls$$ExternalSyntheticLambda0
            @Override // java.lang.Runnable
            public final void run() {
                TouchControls.this.m28lambda$new$0$comptportappTouchControls();
            }
        };
        setFocusable(false);
        this.prefs = context.getSharedPreferences("touch_layout_v1", 0);
        paint.setStyle(Paint.Style.STROKE);
        paint2.setTextAlign(Paint.Align.CENTER);
        Typeface font = Typeface.MONOSPACE;
        try {
            font = context.getResources().getFont(R.font.special_elite);
        } catch (Exception unused) {
        }
        this.label.setTypeface(font);
        load();
        setShown(!controllerConnected());
        postDelayed(this.hideIdle, 10000L);
    }

    private float dp(float f) {
        return f * getResources().getDisplayMetrics().density;
    }

    private static float clamp(float f, float f2, float f3) {
        return Float.isFinite(f) ? Math.max(f2, Math.min(f3, f)) : f2;
    }

    private float radius(int i) {
        return dp(56.0f) * this.controls[i].size;
    }

    private void defaults() {
        float[] fArr = {0.92f, 0.8f, 0.92f, 0.93f, 0.81f, 0.18f, 0.62f};
        float[] fArr2 = {0.82f, 0.85f, 0.61f, 0.09f, 0.09f, 0.68f, 0.68f};
        int i = 0;
        while (true) {
            Control[] controlArr = this.controls;
            if (i < controlArr.length) {
                controlArr[i].nx = fArr[i];
                this.controls[i].ny = fArr2[i];
                this.controls[i].size = 1.0f;
                this.controls[i].visible = true;
                this.controls[i].action = i < 5 ? i : -1;
                i++;
            } else {
                this.opacity = 0.7f;
                this.sensitivity = 1.0f;
                this.deadZone = 0.05f;
                this.floating = true;
                return;
            }
        }
    }

    private void load() {
        defaults();
        lookStyle=Math.max(0, Math.min(2,prefs.getInt("look_style",1)));
        int i = 0;
        while (true) {
            Control[] controlArr = this.controls;
            if (i < controlArr.length) {
                Control control = controlArr[i];
                String str = "control." + i + ".";
                control.nx = clamp(this.prefs.getFloat(str + "x", control.nx), 0.0f, 1.0f);
                control.ny = clamp(this.prefs.getFloat(str + "y", control.ny), 0.0f, 1.0f);
                control.size = clamp(this.prefs.getFloat(str + "size", 1.0f), 0.65f, 1.75f);
                control.visible = this.prefs.getBoolean(str + "visible", true);
                if (i < 5) {
                    control.action = Math.max(0, Math.min(4, this.prefs.getInt(str + "action", i)));
                }
                i++;
            } else {
                this.opacity = clamp(this.prefs.getFloat("opacity", this.opacity), 0.15f, 1.0f);
                this.sensitivity = clamp(this.prefs.getFloat("sensitivity", this.sensitivity), 0.25f, 3.0f);
                this.deadZone = clamp(this.prefs.getFloat("dead_zone", this.deadZone), 0.0f, 0.3f);
                this.floating = this.prefs.getBoolean("floating", true);
                layout();
                return;
            }
        }
    }

    private void save() {
        SharedPreferences.Editor editorEdit = this.prefs.edit();
        int i = 0;
        while (true) {
            Control[] controlArr = this.controls;
            if (i < controlArr.length) {
                Control control = controlArr[i];
                String str = "control." + i + ".";
                editorEdit.putFloat(str + "x", control.nx).putFloat(str + "y", control.ny).putFloat(str + "size", control.size).putBoolean(str + "visible", control.visible).putInt(str + "action", control.action);
                i++;
            } else {
                editorEdit.putFloat("opacity", this.opacity).putFloat("sensitivity", this.sensitivity).putFloat("dead_zone", this.deadZone).putBoolean("floating", this.floating).apply();
                return;
            }
        }
    }

    @Override // android.view.View
    public WindowInsets onApplyWindowInsets(WindowInsets windowInsets) {
        this.insetLeft = windowInsets.getSystemWindowInsetLeft();
        this.insetTop = windowInsets.getSystemWindowInsetTop();
        this.insetRight = windowInsets.getSystemWindowInsetRight();
        this.insetBottom = windowInsets.getSystemWindowInsetBottom();
        if (windowInsets.getDisplayCutout() != null) {
            this.insetLeft = Math.max(this.insetLeft, windowInsets.getDisplayCutout().getSafeInsetLeft());
            this.insetTop = Math.max(this.insetTop, windowInsets.getDisplayCutout().getSafeInsetTop());
            this.insetRight = Math.max(this.insetRight, windowInsets.getDisplayCutout().getSafeInsetRight());
            this.insetBottom = Math.max(this.insetBottom, windowInsets.getDisplayCutout().getSafeInsetBottom());
        }
        layout();
        return windowInsets;
    }

    @Override // android.view.View
    protected void onSizeChanged(int i, int i2, int i3, int i4) {
        releaseAll();
        layout();
    }

    private void layout() {
        if (getWidth() == 0 || getHeight() == 0) {
            return;
        }
        this.safe.set(this.insetLeft + dp(10.0f), this.insetTop + dp(10.0f), (getWidth() - this.insetRight) - dp(10.0f), (getHeight() - this.insetBottom) - dp(10.0f));
        if (this.safe.width() <= 0.0f || this.safe.height() <= 0.0f) {
            this.safe.set(0.0f, 0.0f, getWidth(), getHeight());
        }
        int i = 0;
        while (true) {
            Control[] controlArr = this.controls;
            if (i >= controlArr.length) {
                break;
            }
            Control control = controlArr[i];
            float f = 112.0f;
            float fDp = dp(i < 3 ? 64.0f : i < 5 ? 76.0f : 112.0f) * control.size;
            if (i < 3) {
                f = 64.0f;
            } else if (i < 5) {
                f = 40.0f;
            }
            float fDp2 = dp(f) * control.size;
            float f2 = fDp / 2.0f;
            float fClamp = clamp(this.safe.left + (control.nx * this.safe.width()), this.safe.left + f2, this.safe.right - f2);
            float f3 = fDp2 / 2.0f;
            float fClamp2 = clamp(this.safe.top + (control.ny * this.safe.height()), this.safe.top + f3, this.safe.bottom - f3);
            control.area.set(fClamp - f2, fClamp2 - f3, fClamp + f2, fClamp2 + f3);
            i++;
        }
        this.settingsHit.set(this.safe.left, this.safe.top, this.safe.left + dp(90.0f), this.safe.top + dp(36.0f));
        float fMin = Math.min(dp(104.0f), this.safe.width() / 5.0f);
        for (int i2 = 0; i2 < 4; i2++) {
            float f4 = i2;
            this.toolbar[i2].set(this.safe.left + ((dp(6.0f) + fMin) * f4), this.safe.top, this.safe.left + (f4 * (dp(6.0f) + fMin)) + fMin, this.safe.top + dp(38.0f));
        }
        invalidate();
    }

    void setMode(int i) {
        this.mode = Math.max(0, Math.min(2, i));
        releaseAll();
        setVisibility((this.mode != 1 || this.editing) ? View.VISIBLE : View.GONE);
        int i2 = this.mode;
        if (i2 == 1) {
            setShown(false);
        } else if (i2 == 2) {
            setShown(true);
        }
    }

    void setAllowed(boolean z) {
        setMode(!z ? 1 : 0);
    }

    /* JADX WARN: Removed duplicated region for block: B:7:0x000a  */
    /*
        Code decompiled incorrectly, please refer to instructions dump.
    */
    private void setShown(boolean value) {
        shown=value;
        try { nativeVisible(value && !editing && mode!=1); } catch (UnsatisfiedLinkError ignored) {}
        postInvalidate();
    }
    void reloadPreferences() { releaseAll(); load(); layout(); invalidate(); }

    void releaseAll() {
        removeCallbacks(this.hideIdle);
        try { nativeClearLook(); } catch (UnsatisfiedLinkError ignored) {}
        Stick stick = this.move;
        this.look.pointer = -1;
        stick.pointer = -1;
        for (Control control : this.controls) {
            control.fingers.clear();
        }
        send();
        invalidate();
    }

    void controllerUsed() {
        releaseAll();
        if (this.mode == 2 || this.editing) {
            return;
        }
        setShown(false);
    }

    void focusReturned() {
        removeCallbacks(this.hideIdle);
        if (this.mode == 0 && this.shown && !this.editing) {
            postDelayed(this.hideIdle, HIDE_AFTER_MS);
        }
    }

    @Override // android.view.View
    protected void onDetachedFromWindow() {
        releaseAll();
        super.onDetachedFromWindow();
    }

    @Override // android.view.View
    protected void onWindowVisibilityChanged(int i) {
        super.onWindowVisibilityChanged(i);
        if (this.controls == null || i == 0) {
            return;
        }
        releaseAll();
    }

    private boolean anyFingerDown() {
        if (this.move.pointer >= 0 || this.look.pointer >= 0) {
            return true;
        }
        for (Control control : this.controls) {
            if (!control.fingers.isEmpty()) {
                return true;
            }
        }
        return false;
    }

    static boolean isFingerEvent(MotionEvent motionEvent) {
        return motionEvent.isFromSource(4098) && !motionEvent.isFromSource(8194) && motionEvent.getToolType(motionEvent.getActionIndex()) == MotionEvent.TOOL_TYPE_FINGER;
    }

    /* JADX WARN: Removed duplicated region for block: B:46:0x008b  */
    /* JADX WARN: Removed duplicated region for block: B:47:0x0093  */
    @Override // android.view.View
    /*
        Code decompiled incorrectly, please refer to instructions dump.
    */
    public boolean onTouchEvent(MotionEvent event) {
        int action=event.getActionMasked(), index=event.getActionIndex();
        if(action==MotionEvent.ACTION_CANCEL){if(editing)return editEvent(event);if(anyFingerDown()){releaseAll();return true;}return false;}
        if(!isFingerEvent(event) || (mode==1 && !editing))return false;
        if(editing)return editEvent(event);
        removeCallbacks(hideIdle);if(!shown)setShown(true);
        if(action==MotionEvent.ACTION_DOWN && settingsHit.contains(event.getX(),event.getY())) {
            releaseAll();PTActivity.openMobileSettings();return true;
        }
        if(action==MotionEvent.ACTION_DOWN || action==MotionEvent.ACTION_POINTER_DOWN) {
            press(event.getPointerId(index),event.getX(index),event.getY(index));
        } else if(action==MotionEvent.ACTION_MOVE) {
            for(int i=0;i<event.getPointerCount();i++) {
                int id=event.getPointerId(i);
                if(move.pointer==id) {move.x=event.getX(i);move.y=event.getY(i);}
                if(look.pointer==id)updateLook(event.getX(i),event.getY(i));
            }
        } else if(action==MotionEvent.ACTION_UP || action==MotionEvent.ACTION_POINTER_UP) {
            int id=event.getPointerId(index);
            if(look.pointer==id)updateLook(event.getX(index),event.getY(index));
            release(id);
        } else if(action==MotionEvent.ACTION_CANCEL)releaseAll();
        send();invalidate();
        if(mode==0 && !anyFingerDown())postDelayed(hideIdle,HIDE_AFTER_MS);
        return true;
    }
    private void updateLook(float x,float y) {
        if(lookStyle==1) {
            float factor=720.0f/Math.max(1.0f,safe.height())*sensitivity;
            try {nativeLookDelta((x-lookLastX)*factor,(y-lookLastY)*factor);}catch(UnsatisfiedLinkError ignored){}
            lookLastX=x;lookLastY=y;
        }
        look.x=x;look.y=y;
    }
    private void press(int id,float x,float y) {
        for(int i=0;i<5;i++) {
            Control c=controls[i];
            if(c.visible && x>=c.area.left-dp(6) && x<=c.area.right+dp(6) && y>=c.area.top-dp(6) && y<=c.area.bottom+dp(6)) {
                c.fingers.add(id);return;
            }
        }
        int slot;
        boolean invisibleLook=lookStyle!=0 && x>=safe.centerX() && safe.contains(x,y);
        if(invisibleLook)slot=6;
        else if(floating)slot=x<safe.centerX()?5:6;
        else if(controls[5].visible && controls[5].area.contains(x,y))slot=5;
        else if(controls[6].visible && controls[6].area.contains(x,y))slot=6;
        else return;
        Control c=controls[slot];Stick stick=slot==5?move:look;
        if((!c.visible && !invisibleLook) || stick.pointer>=0)return;
        if(floating || invisibleLook || c.area.contains(x,y)) {
            stick.pointer=id;stick.originX=floating||invisibleLook?x:c.area.centerX();
            stick.originY=floating||invisibleLook?y:c.area.centerY();stick.x=x;stick.y=y;
            if(slot==6){lookLastX=x;lookLastY=y;}
        }
    }

    private void release(int i) {
        if (this.move.pointer == i) {
            this.move.pointer = -1;
        }
        if (this.look.pointer == i) {
            this.look.pointer = -1;
        }
        for (Control control : this.controls) {
            control.fingers.remove(Integer.valueOf(i));
        }
    }

    private float[] axes(Stick stick, int i) {
        if (stick.pointer < 0) {
            return new float[]{0.0f, 0.0f};
        }
        float fRadius = (stick.x - stick.originX) / radius(i);
        float fRadius2 = (stick.y - stick.originY) / radius(i);
        float fHypot = (float) Math.hypot(fRadius, fRadius2);
        if (fHypot <= this.deadZone) {
            return new float[]{0.0f, 0.0f};
        }
        float fMin = Math.min(1.0f, fHypot);
        float f = this.deadZone;
        float fMin2 = (fMin - f) / (1.0f - f);
        if (i == 6) {
            fMin2 = Math.min(1.0f, fMin2 * this.sensitivity);
        }
        return new float[]{(fRadius / fHypot) * fMin2, (fRadius2 / fHypot) * fMin2};
    }

    private void send() {
        if (this.controls == null) {
            return;
        }
        int i = 0;
        for (int i2 = 0; i2 < 5; i2++) {
            if (!this.controls[i2].fingers.isEmpty()) {
                i |= ACTION_BITS[this.controls[i2].action];
            }
        }
        float[] fArrAxes = axes(this.move, 5);
        float[] fArrAxes2 = lookStyle==1 ? new float[]{0,0} : axes(this.look, 6);
        int i3 = this.lastButtons;
        if (i3 < 0 && i == 0 && fArrAxes[0] == 0.0f && fArrAxes[1] == 0.0f && fArrAxes2[0] == 0.0f && fArrAxes2[1] == 0.0f) {
            return;
        }
        if (i == i3 && fArrAxes[0] == this.lastLx && fArrAxes[1] == this.lastLy && fArrAxes2[0] == this.lastRx && fArrAxes2[1] == this.lastRy) {
            return;
        }
        try {
            nativeState(fArrAxes[0], fArrAxes[1], fArrAxes2[0], fArrAxes2[1], i);
            this.lastButtons = i;
            this.lastLx = fArrAxes[0];
            this.lastLy = fArrAxes[1];
            this.lastRx = fArrAxes2[0];
            this.lastRy = fArrAxes2[1];
        } catch (UnsatisfiedLinkError unused) {
        }
    }

    void beginEdit() {
        releaseAll();
        this.editing = true;
        this.selected = -1;
        this.editPointer = -1;
        this.toolbarPressed = -1;
        setVisibility(View.VISIBLE);
        setShown(true);
        layout();
    }

    boolean cancelEditor(KeyEvent keyEvent) {
        if (!this.editing) {
            return false;
        }
        if (keyEvent.getAction() == 1) {
            finishEdit(false);
        }
        return true;
    }

    private void finishEdit(boolean z) {
        if (z) {
            save();
        } else {
            load();
        }
        this.editing = false;
        this.editPointer = -1;
        setMode(this.mode);
        setShown(this.mode != 1);
        if (this.mode == 0) {
            postDelayed(this.hideIdle, HIDE_AFTER_MS);
        }
    }

    private boolean editEvent(MotionEvent event) {
        int action=event.getActionMasked(),index=event.getActionIndex();
        if(action==MotionEvent.ACTION_CANCEL){editPointer=-1;toolbarPressed=-1;invalidate();return true;}
        if(action==MotionEvent.ACTION_DOWN) {
            editPointer=event.getPointerId(index);toolbarPressed=-1;selected=-1;
            float x=event.getX(index),y=event.getY(index);
            for(int i=0;i<toolbar.length;i++)if(toolbar[i].contains(x,y)){toolbarPressed=i;invalidate();return true;}
            for(int i=controls.length-1;i>=0;i--)if(controls[i].area.contains(x,y)){selected=i;break;}
            if(selected>=0){dragDx=controls[selected].area.centerX()-x;dragDy=controls[selected].area.centerY()-y;}
        } else if(action==MotionEvent.ACTION_MOVE && toolbarPressed<0 && selected>=0) {
            int pointer=event.findPointerIndex(editPointer);
            if(pointer>=0 && safe.width()>0 && safe.height()>0) {
                controls[selected].nx=clamp((event.getX(pointer)+dragDx-safe.left)/safe.width(),0f,1f);
                controls[selected].ny=clamp((event.getY(pointer)+dragDy-safe.top)/safe.height(),0f,1f);
                layout();
            }
        } else if((action==MotionEvent.ACTION_UP || action==MotionEvent.ACTION_POINTER_UP) && event.getPointerId(index)==editPointer) {
            int pressed=toolbarPressed;editPointer=-1;toolbarPressed=-1;
            if(pressed>=0 && toolbar[pressed].contains(event.getX(index),event.getY(index))) {
                if(pressed==0)finishEdit(true);
                else if(pressed==1)finishEdit(false);
                else if(pressed==2)showEditorOptions();
                else {defaults();selected=-1;layout();}
            }
        }
        invalidate();return true;
    }

    private void showEditorOptions() {
        LinearLayout linearLayoutColumn = MobileSettings.column(getContext());
        int i = this.selected;
        if (i >= 0) {
            final Control control = this.controls[i];
            MobileSettings.slider(linearLayoutColumn, "Control size", Math.round((control.size - 0.65f) * 100.0f), 110, new IntConsumer() { // from class: com.ptport.app.TouchControls$$ExternalSyntheticLambda1
                @Override // java.util.function.IntConsumer
                public final void accept(int i2) {
                    TouchControls.this.m29lambda$showEditorOptions$1$comptportappTouchControls(control, i2);
                }
            });
            CheckBox checkBox = new CheckBox(getContext());
            checkBox.setText("Show this control");
            checkBox.setChecked(control.visible);
            checkBox.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() { // from class: com.ptport.app.TouchControls$$ExternalSyntheticLambda2
                @Override // android.widget.CompoundButton.OnCheckedChangeListener
                public final void onCheckedChanged(CompoundButton compoundButton, boolean z) {
                    TouchControls.this.m30lambda$showEditorOptions$2$comptportappTouchControls(control, compoundButton, z);
                }
            });
            linearLayoutColumn.addView(checkBox);
            if (this.selected < 5) {
                Spinner spinner = new Spinner(getContext());
                spinner.setAdapter((SpinnerAdapter) new ArrayAdapter(getContext(), android.R.layout.simple_spinner_dropdown_item, ACTION_NAMES));
                spinner.setSelection(control.action);
                linearLayoutColumn.addView(spinner);
                spinner.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() { // from class: com.ptport.app.TouchControls.1
                    @Override // android.widget.AdapterView.OnItemSelectedListener
                    public void onNothingSelected(AdapterView<?> adapterView) {
                    }

                    @Override // android.widget.AdapterView.OnItemSelectedListener
                    public void onItemSelected(AdapterView<?> adapterView, View view, int i2, long j) {
                        control.action = i2;
                        TouchControls.this.invalidate();
                    }
                });
            }
        }
        MobileSettings.slider(linearLayoutColumn, "Opacity", Math.round(this.opacity * 100.0f) - 15, 85, new IntConsumer() { // from class: com.ptport.app.TouchControls$$ExternalSyntheticLambda3
            @Override // java.util.function.IntConsumer
            public final void accept(int i2) {
                TouchControls.this.m31lambda$showEditorOptions$3$comptportappTouchControls(i2);
            }
        });
        MobileSettings.slider(linearLayoutColumn, "Look sensitivity", Math.round((this.sensitivity - 0.25f) * 100.0f), 275, new IntConsumer() { // from class: com.ptport.app.TouchControls$$ExternalSyntheticLambda4
            @Override // java.util.function.IntConsumer
            public final void accept(int i2) {
                TouchControls.this.m32lambda$showEditorOptions$4$comptportappTouchControls(i2);
            }
        });
        MobileSettings.slider(linearLayoutColumn, "Stick dead zone", Math.round(this.deadZone * 100.0f), 30, new IntConsumer() { // from class: com.ptport.app.TouchControls$$ExternalSyntheticLambda5
            @Override // java.util.function.IntConsumer
            public final void accept(int i2) {
                TouchControls.this.m33lambda$showEditorOptions$5$comptportappTouchControls(i2);
            }
        });
        CheckBox checkBox2 = new CheckBox(getContext());
        checkBox2.setText("Floating sticks (start where you touch)");
        checkBox2.setChecked(this.floating);
        checkBox2.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() { // from class: com.ptport.app.TouchControls$$ExternalSyntheticLambda6
            @Override // android.widget.CompoundButton.OnCheckedChangeListener
            public final void onCheckedChanged(CompoundButton compoundButton, boolean z) {
                TouchControls.this.m34lambda$showEditorOptions$6$comptportappTouchControls(compoundButton, z);
            }
        });
        linearLayoutColumn.addView(checkBox2);
        new AlertDialog.Builder(getContext()).setTitle(this.selected < 0 ? "Touch preferences" : "Selected control").setView(MobileSettings.scroll(linearLayoutColumn)).setPositiveButton("Done", (DialogInterface.OnClickListener) null).show();
    }

    /* renamed from: lambda$showEditorOptions$1$com-ptport-app-TouchControls, reason: not valid java name */
    /* synthetic */ void m29lambda$showEditorOptions$1$comptportappTouchControls(Control control, int i) {
        control.size = (i / 100.0f) + 0.65f;
        layout();
    }

    /* renamed from: lambda$showEditorOptions$2$com-ptport-app-TouchControls, reason: not valid java name */
    /* synthetic */ void m30lambda$showEditorOptions$2$comptportappTouchControls(Control control, CompoundButton compoundButton, boolean z) {
        control.visible = z;
        invalidate();
    }

    /* renamed from: lambda$showEditorOptions$3$com-ptport-app-TouchControls, reason: not valid java name */
    /* synthetic */ void m31lambda$showEditorOptions$3$comptportappTouchControls(int i) {
        this.opacity = (i + 15) / 100.0f;
        invalidate();
    }

    /* renamed from: lambda$showEditorOptions$4$com-ptport-app-TouchControls, reason: not valid java name */
    /* synthetic */ void m32lambda$showEditorOptions$4$comptportappTouchControls(int i) {
        this.sensitivity = (i / 100.0f) + 0.25f;
    }

    /* renamed from: lambda$showEditorOptions$5$com-ptport-app-TouchControls, reason: not valid java name */
    /* synthetic */ void m33lambda$showEditorOptions$5$comptportappTouchControls(int i) {
        this.deadZone = i / 100.0f;
    }

    /* renamed from: lambda$showEditorOptions$6$com-ptport-app-TouchControls, reason: not valid java name */
    /* synthetic */ void m34lambda$showEditorOptions$6$comptportappTouchControls(CompoundButton compoundButton, boolean z) {
        this.floating = z;
    }

    private static boolean controllerConnected() {
        for (int i : InputDevice.getDeviceIds()) {
            InputDevice device = InputDevice.getDevice(i);
            if (device != null && !device.isVirtual() && isController(device.getSources())) {
                return true;
            }
        }
        return false;
    }

    @Override // android.view.View
    protected void onDraw(Canvas canvas) {
        if (!this.shown) {
            return;
        }
        int iRound = Math.round(this.opacity * 255.0f);
        this.ring.setColor(Color.argb(iRound, 234, 241, 255));
        this.ring.setStrokeWidth(dp(2.0f));
        int i = iRound / 3;
        this.fill.setColor(Color.argb(i, 234, 241, 255));
        this.label.setColor(Color.argb(iRound, 234, 241, 255));
        int i2 = 0;
        int i3 = 0;
        while (true) {
            Control[] controlArr = this.controls;
            if (i3 >= controlArr.length) {
                break;
            }
            Control control = controlArr[i3];
            if ((i3 != 6 || lookStyle == 0 || editing) && (control.visible || this.editing)) {
                int i4 = control.visible ? iRound : i;
                this.ring.setAlpha(i4);
                this.label.setAlpha(i4);
                if (this.editing && i3 == this.selected) {
                    this.ring.setColor(Color.argb(iRound, 255, 190, 70));
                }
                if (i3 >= 5) {
                    drawStick(canvas, i3 == 5 ? this.move : this.look, i3, i3 == 5 ? "MOVE" : "LOOK");
                } else {
                    if (!control.fingers.isEmpty()) {
                        canvas.drawRoundRect(control.area, dp(20.0f), dp(20.0f), this.fill);
                    }
                    canvas.drawRoundRect(control.area, dp(20.0f), dp(20.0f), this.ring);
                    this.label.setTextSize(dp(14.0f));
                    text(canvas, ACTION_NAMES[control.action], control.area.centerX(), control.area.centerY());
                }
                this.ring.setColor(Color.argb(iRound, 234, 241, 255));
                this.label.setAlpha(iRound);
            }
            i3++;
        }
        if (this.editing) {
            String[] strArr = {"Save", "Cancel", "Options", "Reset"};
            this.fill.setColor(Color.argb(230, 20, 24, 30));
            this.label.setAlpha(255);
            this.label.setTextSize(dp(15.0f));
            while (true) {
                RectF[] rectFArr = this.toolbar;
                if (i2 >= rectFArr.length) {
                    this.label.setTextSize(dp(12.0f));
                    text(canvas, "Drag controls. Select one, then Options to resize or remap. Save to keep changes.", this.safe.centerX(), this.safe.bottom - dp(10.0f));
                    return;
                } else {
                    canvas.drawRoundRect(rectFArr[i2], dp(6.0f), dp(6.0f), this.fill);
                    text(canvas, strArr[i2], this.toolbar[i2].centerX(), this.toolbar[i2].centerY());
                    i2++;
                }
            }
        } else {
            this.label.setTextSize(dp(12.0f));
            text(canvas, "SETTINGS", this.settingsHit.centerX(), this.settingsHit.centerY());
        }
    }

    private void text(Canvas canvas, String str, float f, float f2) {
        canvas.drawText(str, f, f2 - ((this.label.descent() + this.label.ascent()) / 2.0f), this.label);
    }

    private void drawStick(Canvas canvas, Stick stick, int i, String str) {
        Control control = this.controls[i];
        float fRadius = radius(i);
        float fCenterX = (stick.pointer < 0 || this.editing) ? control.area.centerX() : stick.originX;
        float fCenterY = (stick.pointer < 0 || this.editing) ? control.area.centerY() : stick.originY;
        canvas.drawCircle(fCenterX, fCenterY, fRadius, this.ring);
        if (stick.pointer >= 0 && !this.editing) {
            float f = stick.x - fCenterX;
            float f2 = stick.y - fCenterY;
            float fHypot = (float) Math.hypot(f, f2);
            if (fHypot > fRadius) {
                float f3 = fRadius / fHypot;
                f *= f3;
                f2 *= f3;
            }
            canvas.drawCircle(fCenterX + f, fCenterY + f2, fRadius * 0.42f, this.fill);
            return;
        }
        this.label.setTextSize(dp(12.0f));
        text(canvas, str, fCenterX, fCenterY);
    }
}
