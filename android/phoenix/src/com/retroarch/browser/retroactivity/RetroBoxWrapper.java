package com.retroarch.browser.retroactivity;
import android.annotation.TargetApi;
import android.app.Activity;
import android.content.Intent;
import android.os.Build;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.view.Window;
import android.view.WindowManager;

import com.retroarch.browser.preferences.util.UserPreferences;

public class RetroBoxWrapper extends Activity {
        private static final String RETROARCH_FUTURE = "retrobox.retroarch.future";
        private static final String RETROARCH_PAST = "retrobox.retroarch.past";
        
        @Override
        protected void onCreate(Bundle savedInstanceState) {
                super.onCreate(savedInstanceState);
                requestWindowFeature(Window.FEATURE_NO_TITLE);
        getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN,
            WindowManager.LayoutParams.FLAG_FULLSCREEN);
        
        Log.d("RetroArch", "RetroBoxWrapper");
        
                setTitle(getIntent().getStringExtra("TITLE"));
                UserPreferences.updateConfigFile(this);
                
                Intent intent = new Intent();
                intent.setAction(Build.VERSION.SDK_INT >= Build.VERSION_CODES.HONEYCOMB?RETROARCH_FUTURE:RETROARCH_PAST);
                intent.putExtra("CONFIGFILE", UserPreferences.getDefaultConfigPath(this));

                intent.fillIn(getIntent(), 0);

                startActivityForResult(intent,1);
                
        }

        @Override
        protected void onActivityResult(int requestCode, int resultCode, Intent data) {
                finish();
        }

        @TargetApi(Build.VERSION_CODES.KITKAT)
        public static void setImmersiveMode(View decorView) {
                if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.KITKAT) {
                        Log.d("RetroArch", "setImmersiveMode");
                        decorView.setSystemUiVisibility(
                            View.SYSTEM_UI_FLAG_LAYOUT_STABLE
                            | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
                            | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
                            | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION // hide nav bar
                            | View.SYSTEM_UI_FLAG_FULLSCREEN // hide status bar
                            | View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY);
                } else {
                        
                }
        }
}
