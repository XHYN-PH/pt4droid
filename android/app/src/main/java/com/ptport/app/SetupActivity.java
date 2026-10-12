package com.ptport.app;

import android.Manifest;
import android.app.Activity;
import android.app.AlertDialog;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.pm.PackageManager;
import android.graphics.Color;
import android.graphics.Typeface;
import android.graphics.drawable.GradientDrawable;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.os.Environment;
import android.os.ParcelFileDescriptor;
import android.os.StatFs;
import android.provider.DocumentsContract;
import android.provider.Settings;
import android.view.Gravity;
import android.view.KeyEvent;
import android.view.View;
import android.view.ViewGroup;
import android.view.WindowManager;
import android.widget.Button;
import android.widget.FrameLayout;
import android.widget.LinearLayout;
import android.widget.ProgressBar;
import android.widget.ScrollView;
import android.widget.TextView;
import android.widget.ImageView;

import java.io.File;

/* The first screen: starts the game when its files are in place, otherwise sets them up. The player picks their P.T. .pkg
   with Android's file picker (no storage permission needed for that), libptsetup.so takes the three game archives out of it
   into the app's own folder (src/pkginstall), and the game starts. Players who already have the three files can put them in
   Download instead, which needs All files access. Everything is reachable with a gamepad: the d-pad moves, A presses. */
public class SetupActivity extends Activity {
    private static final int PICK_PKG = 1;
    private static final int ASK_MICROPHONE = 2;
    private static final int ASK_STORAGE = 3;
    private static final long NEEDED_BYTES = 1_400_000_000L;
    private static final String[] SHARED_GAME_DIRS = {
        "/storage/emulated/0/Download/CUSA01127", "/storage/emulated/0/Download/PT", "/storage/emulated/0/Download",
        "/storage/emulated/0/PT/CUSA01127", "/storage/emulated/0/PT", "/storage/emulated/0/CUSA01127"};

    private LinearLayout panel;
    private volatile boolean cancelled;
    private long lastUiUpdate;
    private boolean waitingForAccess;
    private boolean firstRunAccess;
    private MobileSettings mobileSettings;
    private boolean handoffToGame;

    private native String nativeExtract(int fd, String destination);

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        if(GameSession.isActive(this) && !getIntent().getBooleanExtra("returned_from_game",false)){
            launchGame();
            return;
        }
        if(savedInstanceState!=null){waitingForAccess=savedInstanceState.getBoolean("waiting_storage",false);firstRunAccess=savedInstanceState.getBoolean("first_storage",false);}
        getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN, WindowManager.LayoutParams.FLAG_FULLSCREEN);
        mobileSettings = new MobileSettings(this,null);
        FrameLayout root = new FrameLayout(this);
        root.setBackground(MenuStyle.background());
        ScrollView scroll = new ScrollView(this);
        panel = new LinearLayout(this);
        panel.setOrientation(LinearLayout.VERTICAL);
        panel.setGravity(Gravity.START);
        int pad = dp(24);
        panel.setPadding(pad, pad, pad, pad);
        scroll.addView(panel, new ScrollView.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT));
        FrameLayout.LayoutParams lp = new FrameLayout.LayoutParams(Math.min(dp(640), getResources().getDisplayMetrics().widthPixels),
                ViewGroup.LayoutParams.MATCH_PARENT, Gravity.CENTER_HORIZONTAL);
        root.addView(scroll, lp);
        setContentView(root);
        root.post(this::promptFirstRunStorage);
        if (gameInstalled()) {
            showReady();
            return;
        }
        System.loadLibrary("ptsetup");
        showWelcome();
    }

    @Override
    protected void onResume() {
        super.onResume();
        DisplayRefresh.apply(this,null);
        if(mobileSettings!=null)mobileSettings.resumeImport();
    }

    // ---- screens

    @Override protected void onSaveInstanceState(Bundle state){state.putBoolean("waiting_storage",waitingForAccess);state.putBoolean("first_storage",firstRunAccess);super.onSaveInstanceState(state);}
    @Override protected void onDestroy(){cancelled=true;if(mobileSettings!=null)mobileSettings.detach();super.onDestroy();if(handoffToGame)android.os.Process.killProcess(android.os.Process.myPid());}
    private void showReady(){
        panel.removeAllViews();brand();
        button("Continue",v->startGame()).requestFocus();
        button("Settings",v->mobileSettings.show());
        button("Delete saves",v->mobileSettings.deleteSaves());
        button("GitHub • project & updates",v->openGithub());
        button("Quit",v->finish());
        TextView version=text("pt4droid • "+versionName());version.setTextSize(11);version.setPadding(0,dp(18),0,0);
    }
    private String versionName(){try{return getPackageManager().getPackageInfo(getPackageName(),0).versionName;}catch(Exception ignored){return "test";}}
    private void showWelcome() {
        panel.removeAllViews();
        brand();
        text("This app needs your own copy of the game.\n\n"
                + "Choose your P.T. .pkg file (a fake PKG made from your own dump). The setup takes the game files out of it "
                + "once, which takes a minute or two and needs about 1.4 GB of free space. After that you can start the game from the launcher.");
        Button pick = button("Choose your P.T. .pkg", v -> pickPackage());
        button("I already have the extracted files", v -> showExtractedFiles());
        button("GitHub • project & updates", v -> openGithub());
        button("Quit", v -> finish());
        pick.requestFocus();
    }

    private void showExtractedFiles() {
        panel.removeAllViews();
        title("Extracted files");
        text("Copy these three files from your CUSA01127 dump into the Download folder (loose, or in a CUSA01127 folder "
                + "inside it):\n\n    chunk1.psarc\n    texture.qar\n    pathid_list_ps4.bin\n\n"
                + "Then allow pt4droid \"All files access\" so it can read them there.");
        Button allow = button("Allow access", v -> requestAllFilesAccess());
        button("Back", v -> showWelcome());
        allow.requestFocus();
    }

    private void showProgress() {
        panel.removeAllViews();
        title("Setting up");
        TextView status = text("Opening the package...");
        status.setTag("status");
        ProgressBar bar = new ProgressBar(this, null, android.R.attr.progressBarStyleHorizontal);
        bar.setMax(1000);
        bar.setTag("bar");
        LinearLayout.LayoutParams lp = new LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, dp(12));
        lp.setMargins(0, dp(8), 0, dp(20));
        panel.addView(bar, lp);
        Button cancel = button("Cancel", v -> {
            cancelled = true;
            v.setEnabled(false);
        });
        cancel.requestFocus();
    }

    private void showError(String message) {
        panel.removeAllViews();
        title("That did not work");
        text(message);
        Button again = button("Choose another file", v -> pickPackage());
        button("Back", v -> showWelcome());
        again.requestFocus();
    }

    private void showDone(Uri packageUri) {
        panel.removeAllViews();
        title("Ready");
        text("P.T. is set up. The .pkg is not needed any more: deleting it frees about 1.3 GB.");
        Button start = button("Start P.T.", v -> startGame());
        button("Delete the .pkg and start", v -> {
            boolean deleted = false;
            try {
                deleted = DocumentsContract.deleteDocument(getContentResolver(), packageUri);
            } catch (Exception ignored) {
            }
            if (!deleted) {
                panel.removeAllViews();
                title("Ready");
                text("The .pkg could not be deleted from here. You can remove it yourself with a file manager.");
                button("Start P.T.", w -> startGame()).requestFocus();
                return;
            }
            startGame();
        });
        start.requestFocus();
    }

    // asked once, before the first game start: the ending listens for a word. The game works without it (both triggers).
    private void showMicrophone() {
        panel.removeAllViews();
        title("Microphone");
        text("Near the end, P.T. asks you to say a name out loud. The game can listen for it through the microphone.\n\n"
                + "It only listens in that last part, the words are recognized on this device, and nothing is recorded or "
                + "sent anywhere (the app has no internet access). If you'd rather not, holding both triggers says it for you.");
        Button allow = button("Allow the microphone", v -> {
            markMicrophoneAsked();
            requestPermissions(new String[] {Manifest.permission.RECORD_AUDIO}, ASK_MICROPHONE);
        });
        button("Not now", v -> {
            markMicrophoneAsked();
            launchGame();
        });
        allow.requestFocus();
    }

    private boolean shouldAskMicrophone() {
        if (checkSelfPermission(Manifest.permission.RECORD_AUDIO) == PackageManager.PERMISSION_GRANTED) return false;
        return !getSharedPreferences("setup", MODE_PRIVATE).getBoolean("asked_microphone", false);
    }

    private void markMicrophoneAsked() {
        SharedPreferences.Editor editor = getSharedPreferences("setup", MODE_PRIVATE).edit();
        editor.putBoolean("asked_microphone", true);
        editor.apply();
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, String[] permissions, int[] results) {
        super.onRequestPermissionsResult(requestCode, permissions, results);
        if (requestCode == ASK_MICROPHONE) launchGame();
        if (requestCode == ASK_STORAGE) finishStorageRequest();
    }

    // ---- actions

    private void pickPackage() {
        Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT);
        intent.addCategory(Intent.CATEGORY_OPENABLE);
        intent.setType("*/*");
        try {
            startActivityForResult(intent, PICK_PKG);
        } catch (Exception e) {
            showError("This device has no file picker for choosing the package.");
        }
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        if(mobileSettings!=null && mobileSettings.activityResult(requestCode,resultCode,data))return;
        super.onActivityResult(requestCode, resultCode, data);
        if(requestCode==ASK_STORAGE){finishStorageRequest();return;}
        if (requestCode != PICK_PKG || resultCode != RESULT_OK || data == null || data.getData() == null) return;
        extract(data.getData());
    }

    private void extract(Uri uri) {
        File destination = new File(GameStorage.dataRoot(this), "game/CUSA01127");
        long free;
        try{
            if(!destination.isDirectory()&&!destination.mkdirs())throw new java.io.IOException("Cannot create the game folder");
            free=new StatFs(destination.getAbsolutePath()).getAvailableBytes();
        }catch(Exception error){showError("Could not prepare storage for the game: "+error.getMessage());return;}
        if (free < NEEDED_BYTES) {
            showError("There is not enough free space: the game files need about 1.4 GB, and " + (free / 1_000_000) + " MB are free.");
            return;
        }
        final ParcelFileDescriptor pfd;
        try {
            pfd = getContentResolver().openFileDescriptor(uri, "r");
        } catch (Exception e) {
            showError("Could not open the chosen file: " + e.getMessage());
            return;
        }
        if (pfd == null) {
            showError("Could not open the chosen file.");
            return;
        }
        cancelled = false;
        showProgress();
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        new Thread(() -> {
            String error;
            try {
                error = nativeExtract(pfd.getFd(), destination.getAbsolutePath());
            } catch (Throwable t) {
                error = "The setup failed: " + t;
            }
            try {
                pfd.close();
            } catch (Exception ignored) {
            }
            final String result = error;
            runOnUiThread(() -> {
                if(isFinishing()||isDestroyed())return;
                getWindow().clearFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
                if (result == null) {
                    showDone(uri);
                } else if (result.isEmpty()) {
                    showWelcome();  // cancelled
                } else {
                    showError(result);
                }
            });
        }, "pt-setup").start();
    }

    // called from libptsetup.so on the setup thread; false stops the extraction
    @SuppressWarnings("unused")
    private boolean onExtractProgress(long done, long total, String stage) {
        long now = System.currentTimeMillis();
        if (now - lastUiUpdate >= 100 || done == total) {
            lastUiUpdate = now;
            runOnUiThread(() -> {
                TextView status = panel.findViewWithTag("status");
                ProgressBar bar = panel.findViewWithTag("bar");
                if (status == null || bar == null) return;
                int permille = total > 0 ? (int) (done * 1000 / total) : 0;
                bar.setProgress(permille);
                status.setText(stage + "...  " + (permille / 10) + "%");
            });
        }
        return !cancelled;
    }

    private void requestAllFilesAccess() {
        if (storageGranted()) {
            if(firstRunAccess){firstRunAccess=false;if(gameInstalled())showReady();else showWelcome();return;}
            if (gameInstalled()) {
                startGame();
            } else {
                showError("The three files were not found in Download. Check their names and that they finished copying.");
            }
            return;
        }
        waitingForAccess = true;
        if(Build.VERSION.SDK_INT<Build.VERSION_CODES.R){
            requestPermissions(new String[]{Manifest.permission.READ_EXTERNAL_STORAGE},ASK_STORAGE);return;
        }
        try {
            Intent intent = new Intent(Settings.ACTION_MANAGE_APP_ALL_FILES_ACCESS_PERMISSION);
            intent.setData(Uri.parse("package:" + getPackageName()));
            startActivityForResult(intent,ASK_STORAGE);
        } catch (Exception e) {
            try{startActivityForResult(new Intent(Settings.ACTION_MANAGE_ALL_FILES_ACCESS_PERMISSION),ASK_STORAGE);}
            catch(Exception unavailable){waitingForAccess=false;firstRunAccess=false;android.widget.Toast.makeText(this,"Storage settings are unavailable. You can still import with the file picker.",android.widget.Toast.LENGTH_LONG).show();}
        }
    }

    private boolean storageGranted(){return Build.VERSION.SDK_INT>=Build.VERSION_CODES.R?Environment.isExternalStorageManager():checkSelfPermission(Manifest.permission.READ_EXTERNAL_STORAGE)==PackageManager.PERMISSION_GRANTED;}
    private void promptFirstRunStorage(){
        if(isFinishing()||isDestroyed())return;
        SharedPreferences setup=getSharedPreferences("setup",MODE_PRIVATE);
        if(setup.getBoolean("asked_storage",false))return;
        setup.edit().putBoolean("asked_storage",true).apply();
        if(storageGranted())return;
        AlertDialog d=new AlertDialog.Builder(this).setTitle("Storage access")
            .setMessage("Allow storage access to read game files in Download. Android 11 and later opens an All files access screen. You can also import a PKG or driver ZIP with the file picker without this permission.")
            .setPositiveButton("Allow access",(dialog,which)->{firstRunAccess=true;requestAllFilesAccess();})
            .setNegativeButton("Use file picker",null).create();d.show();MenuStyle.styleDialog(d);
    }
    private void finishStorageRequest(){
        if(!waitingForAccess)return;
        waitingForAccess=false;boolean first=firstRunAccess;firstRunAccess=false;
        if(gameInstalled()){if(first)showReady();else startGame();}
        else if(first)showWelcome();
        else showError(storageGranted()?"The game files were not found in Download. Check the file names and wait for copying to finish.":"Storage access was not granted. You can still choose a PKG with the file picker.");
    }
    private void openGithub(){try{startActivity(new Intent(Intent.ACTION_VIEW,Uri.parse("https://github.com/XHYN-PH/pt4droid")));}catch(Exception unavailable){android.widget.Toast.makeText(this,"No browser is available to open GitHub.",android.widget.Toast.LENGTH_LONG).show();}}
    private void brand(){
        ImageView logo=new ImageView(this);logo.setImageResource(R.drawable.pt_logo);logo.setContentDescription("P.T. logo");logo.setScaleType(ImageView.ScaleType.FIT_CENTER);
        LinearLayout.LayoutParams image=new LinearLayout.LayoutParams(-1,dp(120));image.setMargins(0,0,0,dp(6));panel.addView(logo,image);
        title("pt4droid");
        TextView subtitle=text("THE HALLWAY IS WAITING");subtitle.setTextSize(11);subtitle.setLetterSpacing(.18f);subtitle.setGravity(Gravity.CENTER);
    }

    private boolean gameInstalled() {
        File external=getExternalFilesDir(null);
        if(external!=null&&hasGameFiles(new File(external,"game/CUSA01127")))return true;
        if(hasGameFiles(new File(getFilesDir(),"game/CUSA01127")))return true;
        boolean canReadShared = storageGranted();
        if (!canReadShared) return false;
        for (String dir : SHARED_GAME_DIRS) {
            if (hasGameFiles(new File(dir))) return true;
        }
        return false;
    }

    private static boolean hasGameFiles(File dir) {
        return new File(dir, "chunk1.psarc").isFile() && new File(dir, "texture.qar").isFile();
    }

    private void startGame() {
        if(GameSession.isActive(this)){
            android.widget.Toast.makeText(this,"The game is still closing. Please try again in a moment.",android.widget.Toast.LENGTH_SHORT).show();
            return;
        }
        DriverImportJob job=DriverImportJob.get();
        if(job!=null && !job.done){
            android.widget.Toast.makeText(this,"Wait for the driver import to finish or cancel it before starting the game.",android.widget.Toast.LENGTH_SHORT).show();
            return;
        }
        if (shouldAskMicrophone()) {
            showMicrophone();
            return;
        }
        launchGame();
    }

    private void launchGame() {
        startActivity(new Intent(this, PTActivity.class));
        handoffToGame = true;
        finish();
    }

    // ---- gamepad: A presses the focused button, B goes back

    @Override
    public boolean dispatchKeyEvent(KeyEvent event) {
        if (event.getKeyCode() == KeyEvent.KEYCODE_BUTTON_A) {
            KeyEvent center = new KeyEvent(event.getDownTime(), event.getEventTime(), event.getAction(), KeyEvent.KEYCODE_DPAD_CENTER,
                    event.getRepeatCount(), event.getMetaState(), event.getDeviceId(), event.getScanCode(), event.getFlags(), event.getSource());
            return super.dispatchKeyEvent(center);
        }
        if (event.getKeyCode() == KeyEvent.KEYCODE_BUTTON_B) {
            if (event.getAction() == KeyEvent.ACTION_UP) onBackPressed();
            return true;
        }
        return super.dispatchKeyEvent(event);
    }

    // ---- views

    private int dp(int value) {
        return Math.round(value * getResources().getDisplayMetrics().density);
    }

    private void title(String text) {
        TextView view = new TextView(this);
        view.setText(text);
        view.setTextColor(Color.rgb(222, 218, 208));
        view.setTextSize(34);
        view.setTypeface(Typeface.create("serif", Typeface.NORMAL));
        view.setGravity(text.equals("pt4droid")?Gravity.CENTER:Gravity.START);
        LinearLayout.LayoutParams lp = new LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT);
        lp.setMargins(0, dp(12), 0, dp(18));
        panel.addView(view, lp);
    }

    private TextView text(String text) {
        TextView view = new TextView(this);
        view.setText(text);
        view.setTextColor(Color.rgb(176, 172, 164));
        view.setTextSize(16);
        view.setLineSpacing(0, 1.2f);
        LinearLayout.LayoutParams lp = new LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT);
        lp.setMargins(0, 0, 0, dp(20));
        panel.addView(view, lp);
        return view;
    }

    private Button button(String label, View.OnClickListener onClick) {
        Button view = new Button(this);
        view.setText(label);
        MenuStyle.button(view);
        view.setOnClickListener(onClick);
        LinearLayout.LayoutParams lp = new LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, dp(50));
        lp.setMargins(0, 0, 0, dp(12));
        panel.addView(view, lp);
        return view;
    }
}
