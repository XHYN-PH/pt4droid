package com.ptport.app;

import android.app.Application;

public final class PTApplication extends Application {
    static {System.loadLibrary("ptsetup");}
    static native void nativeConfigureLogging(boolean enabled);
    @Override public void onCreate(){super.onCreate();AppLog.initialize(this);}
}
