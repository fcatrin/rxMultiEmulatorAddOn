package com.retroarch.browser.retroactivity;

import java.util.ArrayList;
import java.util.List;

import retrobox.utils.GamepadInfoDialog;
import retrobox.utils.ListOption;
import retrobox.utils.RetroBoxDialog;
import retrobox.utils.RetroBoxUtils;
import retrobox.v2.retroarch.R;
import xtvapps.core.AndroidFonts;
import xtvapps.core.Callback;
import xtvapps.core.SimpleCallback;
import xtvapps.core.content.KeyValue;
import android.app.Activity;
import android.content.Context;
import android.content.SharedPreferences;
import android.content.SharedPreferences.Editor;
import android.os.Bundle;
import android.view.KeyEvent;
import android.view.Window;
import android.view.WindowManager;

public class RetroBoxMenu extends Activity {
	private GamepadInfoDialog gamepadInfoDialog;

	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		requestWindowFeature(Window.FEATURE_NO_TITLE);
		
		getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN,
				WindowManager.LayoutParams.FLAG_FULLSCREEN);

		setContentView(R.layout.retrobox_window);
		
		AndroidFonts.setViewFont(findViewById(R.id.txtDialogActionTitle), RetroBoxUtils.FONT_DEFAULT_M);
		AndroidFonts.setViewFont(findViewById(R.id.txtDialogListTitle), RetroBoxUtils.FONT_DEFAULT_M);
		
        AndroidFonts.setViewFont(findViewById(R.id.txtGamepadInfoTop), RetroBoxUtils.FONT_DEFAULT_M);
        AndroidFonts.setViewFont(findViewById(R.id.txtGamepadInfoBottom), RetroBoxUtils.FONT_DEFAULT_M);

        gamepadInfoDialog = new GamepadInfoDialog(this);
        gamepadInfoDialog.loadFromIntent(getIntent());
	}

	@Override
	protected void onResume() {
		super.onResume();
		List<ListOption> options = new ArrayList<ListOption>();
        options.add(new ListOption("", "Cancel"));
        options.add(new ListOption("save", "Save State"));
        options.add(new ListOption("load", "Load State"));
        //options.add(new ListOption("slot", "Set Save Slot"));
        if (getIntent().hasExtra("MULTIDISK")) {
        	options.add(new ListOption("swap", "Swap Disk"));
        }
        // disable rest (some emulators hang on reset)
        // options.add(new ListOption("reset", "Reset"));
        options.add(new ListOption("help", "Help"));
        options.add(new ListOption("quit", "Quit"));
		
		RetroBoxDialog.showListDialog(this, "RetroBoxTV", options, new Callback<KeyValue>(){
			@Override
			public void onResult(KeyValue result) {
				String key = result.getKey();
				int optionId = RetroActivityFuture.RESULT_CANCEL_ID;
				
				if (key.equals("save")) optionId = RetroActivityFuture.RESULT_SAVE_ID;
				if (key.equals("load")) optionId = RetroActivityFuture.RESULT_LOAD_ID;
				if (key.equals("swap")) optionId = RetroActivityFuture.RESULT_SWAP_ID;
				if (key.equals("reset")) optionId = RetroActivityFuture.RESULT_RESET_ID;
				if (key.equals("quit")) optionId = RetroActivityFuture.RESULT_QUIT_ID;
				if (key.equals("help")) optionId = RetroActivityFuture.RESULT_HELP_ID;
				
				SharedPreferences preferences = getPreferences();
				Editor editor = preferences.edit();
				editor.putInt("optionId", optionId);
				editor.commit();
				
				if (optionId == RetroActivityFuture.RESULT_HELP_ID) {
					uiHelp();
				} else {
					finish();
				}
			}
		});
	}
	
    protected void uiHelp() {
		RetroBoxDialog.showGamepadDialogIngame(this, gamepadInfoDialog, new SimpleCallback() {
			@Override
			public void onResult() {
				finish();
			}
		});
    }

	
	private SharedPreferences getPreferences() {
		return getSharedPreferences("RetroBoxMenu", Context.MODE_PRIVATE);
	}

	@Override
	public boolean onKeyDown(int keyCode, KeyEvent event) {
		if (RetroBoxDialog.onKeyDown(this, keyCode, event)) return true;
		return super.onKeyDown(keyCode, event);
	}

	@Override
	public boolean onKeyUp(int keyCode, KeyEvent event) {
		if (RetroBoxDialog.onKeyUp(this, keyCode, event)) return true;
		return super.onKeyUp(keyCode, event);
	}
	
	
	
}
