package com.ptport.app;

import android.app.Application;
import android.content.Context;
import android.content.pm.PackageInfo;
import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.nio.charset.StandardCharsets;
import java.time.Instant;

/** Read identity from the installed APK, including before SDL or Vulkan starts. */
final class CrashMetadata {
    private CrashMetadata() {}
    static String identity(Context context) {
        StringBuilder text=new StringBuilder("package ").append(context.getPackageName()).append('\n');
        try {
            PackageInfo info=context.getPackageManager().getPackageInfo(context.getPackageName(),0);
            text.append("pt4droid ").append(info.versionName).append(" (code ").append(info.getLongVersionCode()).append(")\n");
        } catch(Exception ignored) {text.append("version unavailable\n");}
        try(BufferedReader in=new BufferedReader(new InputStreamReader(context.getAssets().open("pt_assets.txt"),StandardCharsets.UTF_8))) {
            String stamp=in.readLine();
            if(stamp!=null&&stamp.startsWith("# "))text.append("assets ").append(stamp.substring(2,Math.min(stamp.length(),194))).append('\n');
        } catch(Exception ignored) {text.append("assets stamp unavailable\n");}
        return text.toString();
    }
    static String report(Context context) {
        String timestamp=Instant.now().toString();
        return identity(context)+"timestamp_utc "+timestamp+"\nprocess "+Application.getProcessName()+"\n";
    }
}
