package com.ptport.app;
import android.annotation.TargetApi;
import android.content.Context;
import android.os.Build;
import android.os.PerformanceHintManager;
import android.os.Process;
/** Frame hints follow the native render thread; no affinity, clocks, or FPS cap is changed. */
final class FrameHints {
    private static boolean paused=true,serviceFailed=false;
    private static Api31 api;
    static synchronized void pause(){paused=true;serviceFailed=false;if(api!=null){api.close();api=null;}}
    static synchronized void resume(){paused=false;serviceFailed=false;}
    static synchronized void report(Context context,long work,long target,boolean enabled){
        if(Build.VERSION.SDK_INT<31||paused||!enabled){if(api!=null){api.close();api=null;}if(!enabled)serviceFailed=false;return;}
        if(serviceFailed)return;
        try{if(api==null)api=new Api31(context);api.report(work,target);}catch(RuntimeException error){if(api!=null)api.close();api=null;serviceFailed=true;}
    }
    @TargetApi(31) private static final class Api31 {
        final PerformanceHintManager manager;PerformanceHintManager.Session session;int tid;long target;
        Api31(Context context){manager=context.getSystemService(PerformanceHintManager.class);}
        void report(long work,long requested){
            if(manager==null||work<=0||requested<=0)return;
            int current=Process.myTid();long next=Math.max(4_166_667L,Math.min(100_000_000L,requested));
            if(session==null||tid!=current){close();tid=current;target=next;session=manager.createHintSession(new int[]{tid},target);}
            if(session==null)return;
            if(target!=next){target=next;session.updateTargetWorkDuration(target);}
            session.reportActualWorkDuration(Math.max(1,work));
        }
        void close(){if(session!=null){try{session.close();}catch(RuntimeException ignored){}finally{session=null;}}}
    }
}
