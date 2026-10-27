package dev.twilitrealm.dusk;

import android.content.Intent;
import android.content.Context;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.os.SystemClock;
import android.content.res.ColorStateList;
import android.graphics.Color;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.Typeface;
import android.graphics.drawable.GradientDrawable;
import android.util.Log;
import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup;
import android.view.SurfaceHolder;
import android.widget.Button;
import android.widget.FrameLayout;
import android.widget.LinearLayout;
import android.widget.ProgressBar;
import android.widget.TextView;

import dev.encounter.borealis.BorealisActivity;
import org.libsdl.app.SDLSurface;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

public class DuskActivity extends BorealisActivity {
    private static final String TAG = "DuskActivity";
    private static native void nativeWakeRenderer();
    private FrameLayout loadingView;
    private TextView loadingTitle, loadingDetail, loadingTime;
    private ProgressBar loadingProgress;
    private Button loadingCancel;
    private volatile boolean loadingCancelled;
    private boolean discSelectionPending;
    private long loadingStarted;
    private final Handler loadingHandler = new Handler(Looper.getMainLooper());
    private final Runnable loadingClock = new Runnable() {
        @Override public void run() {
            if (loadingView == null) return;
            long seconds = (SystemClock.elapsedRealtime() - loadingStarted) / 1000;
            loadingTime.setText(seconds < 30 ? "Temps écoulé : " + seconds + " s"
                : "Temps écoulé : " + seconds + " s\nChargement plus long que prévu. Si l'attente se prolonge, relancez l'application.");
            loadingHandler.postDelayed(this, 1000);
        }
    };

    private static final class LoadingSpinner extends View {
        private final Paint paint = new Paint(Paint.ANTI_ALIAS_FLAG);
        LoadingSpinner(Context context) {
            super(context);
            paint.setColor(Color.rgb(230, 209, 97));
            paint.setStyle(Paint.Style.STROKE);
            paint.setStrokeCap(Paint.Cap.ROUND);
            paint.setStrokeWidth(4 * getResources().getDisplayMetrics().density);
            setContentDescription("Chargement en cours");
        }
        @Override protected void onDraw(Canvas canvas) {
            super.onDraw(canvas);
            float inset = paint.getStrokeWidth();
            float angle = (SystemClock.uptimeMillis() % 1200) * 360f / 1200;
            canvas.drawArc(inset, inset, getWidth() - inset, getHeight() - inset,
                angle, 260, false, paint);
            // Keep animating even on consoles with Android system animations disabled.
            postInvalidateOnAnimation();
        }
    }

    @Override
    protected SDLSurface createSDLSurface(Context context) {
        SDLSurface surface = super.createSDLSurface(context);
        surface.getHolder().addCallback(new SurfaceHolder.Callback() {
            @Override public void surfaceCreated(SurfaceHolder holder) {}
            @Override public void surfaceDestroyed(SurfaceHolder holder) {}
            @Override public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
                loadingHandler.post(DuskActivity::nativeWakeRenderer);
            }
        });
        return surface;
    }

    @Override
    public void onWindowFocusChanged(boolean hasFocus) {
        super.onWindowFocusChanged(hasFocus);
        if (hasFocus) loadingHandler.post(DuskActivity::nativeWakeRenderer);
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        extractBundledMods();
        super.onCreate(savedInstanceState);
        showLoading("Démarrage de Dusklight", "Préparation de l'interface…", -1, false);
    }

    private int dp(int value) {
        return Math.round(value * getResources().getDisplayMetrics().density);
    }

    public void prepareDiscSelection() {
        runOnUiThread(() -> discSelectionPending = true);
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        if (discSelectionPending) {
            discSelectionPending = false;
            if (resultCode == RESULT_OK && data != null
                    && (data.getData() != null || data.getClipData() != null)) {
                showLoading("Ouverture de la ROM", "Reprise de la vérification du disque…", -1, true);
            }
        }
        super.onActivityResult(requestCode, resultCode, data);
    }

    private TextView loadingText(int size, int color) {
        TextView text = new TextView(this);
        text.setTextSize(size);
        text.setTextColor(color);
        text.setGravity(Gravity.CENTER);
        text.setPadding(0, dp(8), 0, dp(8));
        return text;
    }

    // Called from the native game thread through JNI. Android keeps the spinner animated
    // even while the game thread is opening the disc or preparing its resources.
    public void showLoading(String stage, String detail, int percent, boolean cancellable) {
        runOnUiThread(() -> {
            if (isFinishing() || isDestroyed()) return;
            if (loadingView == null) {
                loadingCancelled = false;
                loadingStarted = SystemClock.elapsedRealtime();
                loadingView = new FrameLayout(this);
                loadingView.setBackgroundColor(Color.rgb(20, 21, 16));
                loadingView.setClickable(true);
                loadingView.setImportantForAccessibility(View.IMPORTANT_FOR_ACCESSIBILITY_YES);

                LinearLayout panel = new LinearLayout(this);
                panel.setOrientation(LinearLayout.VERTICAL);
                panel.setGravity(Gravity.CENTER);
                panel.setPadding(dp(24), dp(20), dp(24), dp(20));
                GradientDrawable background = new GradientDrawable();
                background.setColor(Color.rgb(42, 43, 33));
                background.setCornerRadius(dp(20));
                background.setStroke(dp(1), Color.rgb(147, 138, 88));
                panel.setBackground(background);
                int available = getResources().getDisplayMetrics().widthPixels - dp(48);
                FrameLayout.LayoutParams params = new FrameLayout.LayoutParams(
                    Math.min(dp(580), available), ViewGroup.LayoutParams.WRAP_CONTENT, Gravity.CENTER);
                loadingView.addView(panel, params);

                TextView brand = loadingText(16, Color.rgb(206, 191, 109));
                brand.setText("DUSKLIGHT");
                brand.setTypeface(null, Typeface.BOLD);
                panel.addView(brand);
                View spinner = new LoadingSpinner(this);
                panel.addView(spinner, new LinearLayout.LayoutParams(dp(44), dp(44)));
                loadingTitle = loadingText(22, Color.WHITE);
                loadingTitle.setTypeface(null, Typeface.BOLD);
                loadingTitle.setAccessibilityLiveRegion(View.ACCESSIBILITY_LIVE_REGION_POLITE);
                panel.addView(loadingTitle);
                loadingDetail = loadingText(16, Color.rgb(223, 221, 207));
                panel.addView(loadingDetail);
                loadingProgress = new ProgressBar(this, null, android.R.attr.progressBarStyleHorizontal);
                loadingProgress.setMax(100);
                loadingProgress.setProgressTintList(ColorStateList.valueOf(Color.rgb(230, 209, 97)));
                panel.addView(loadingProgress, new LinearLayout.LayoutParams(
                    ViewGroup.LayoutParams.MATCH_PARENT, dp(8)));
                loadingTime = loadingText(14, Color.rgb(187, 185, 167));
                panel.addView(loadingTime);
                loadingCancel = new Button(this);
                loadingCancel.setText("Annuler");
                loadingCancel.setAllCaps(false);
                loadingCancel.setTextColor(Color.WHITE);
                GradientDrawable buttonBackground = new GradientDrawable();
                buttonBackground.setColor(Color.rgb(87, 81, 52));
                buttonBackground.setCornerRadius(dp(12));
                buttonBackground.setStroke(dp(1), Color.rgb(206, 191, 109));
                loadingCancel.setBackground(buttonBackground);
                loadingCancel.setOnClickListener(view -> {
                    loadingCancelled = true;
                    loadingCancel.setEnabled(false);
                    loadingCancel.setAlpha(0.6f);
                    loadingCancel.setText("Annulation…");
                });
                panel.addView(loadingCancel);
                addContentView(loadingView, new ViewGroup.LayoutParams(
                    ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT));
                loadingHandler.post(loadingClock);
            }
            if (!stage.contentEquals(loadingTitle.getText())) loadingTitle.setText(stage);
            loadingDetail.setText(detail);
            loadingProgress.setVisibility(percent < 0 ? View.GONE : View.VISIBLE);
            if (percent >= 0) loadingProgress.setProgress(Math.min(100, percent));
            loadingCancel.setVisibility(cancellable ? View.VISIBLE : View.GONE);
        });
    }

    public boolean isLoadingCancelled() {
        return loadingCancelled;
    }

    public void hideLoading() {
        runOnUiThread(() -> {
            discSelectionPending = false;
            loadingHandler.removeCallbacks(loadingClock);
            if (loadingView != null) {
                ((ViewGroup) loadingView.getParent()).removeView(loadingView);
                loadingView = null;
            }
            loadingCancelled = false;
        });
    }

    @Override
    protected void onDestroy() {
        loadingHandler.removeCallbacks(loadingClock);
        super.onDestroy();
    }

    // Bundled mod packages ship as APK assets, which the native loader cannot read directly;
    // mirror them into internal storage (the loader's CachePath/bundled_mods search dir)
    // before SDL_main starts.
    private void extractBundledMods() {
        File outDir = new File(getFilesDir(), "bundled_mods");
        try {
            deleteRecursively(outDir); // drop packages removed by an app update
            String[] names = getAssets().list("mods");
            if (names == null || names.length == 0) {
                return;
            }
            if (!outDir.mkdirs()) {
                Log.w(TAG, "Unable to create " + outDir);
                return;
            }
            byte[] buffer = new byte[65536];
            for (String name : names) {
                if (!name.endsWith(".dusk")) {
                    continue;
                }
                try (InputStream in = getAssets().open("mods/" + name);
                     OutputStream out = new FileOutputStream(new File(outDir, name)))
                {
                    int count;
                    while ((count = in.read(buffer)) > 0) {
                        out.write(buffer, 0, count);
                    }
                }
            }
        } catch (IOException e) {
            Log.w(TAG, "Failed to extract bundled mods", e);
        }
    }

    private static void deleteRecursively(File file) {
        File[] children = file.listFiles();
        if (children != null) {
            for (File child : children) {
                deleteRecursively(child);
            }
        }
        file.delete();
    }

    @Override
    protected String[] getArguments() {
        String[] arguments = super.getArguments();
        if (arguments.length > 0) {
            return arguments;
        }

        Intent intent = getIntent();
        if (intent == null) {
            return arguments;
        }
        String[] argv = intent.getStringArrayExtra("dusk_argv");
        if (argv != null && argv.length > 0) {
            return argv;
        }
        String rawArgs = intent.getStringExtra("dusk_args");
        return rawArgs == null ? arguments : splitArguments(rawArgs.trim());
    }

}
