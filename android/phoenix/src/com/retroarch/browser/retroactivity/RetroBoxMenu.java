package com.retroarch.browser.retroactivity;

import java.io.File;
import java.util.ArrayList;
import java.util.List;

import retrobox.content.SaveStateInfo;
import retrobox.utils.GamepadInfoDialog;
import retrobox.utils.ListOption;
import retrobox.utils.RetroBoxDialog;
import retrobox.utils.RetroBoxUtils;
import retrobox.utils.SaveStateSelectorAdapter;
import xtvapps.prg.retroarch.R;
import xtvapps.core.AndroidFonts;
import xtvapps.core.Callback;
import xtvapps.core.SimpleCallback;
import xtvapps.core.Utils;
import xtvapps.core.content.KeyValue;
import android.app.Activity;
import android.content.Context;
import android.content.SharedPreferences;
import android.content.SharedPreferences.Editor;
import android.os.Bundle;
import android.util.Log;
import android.view.KeyEvent;
import android.view.View;
import android.view.Window;
import android.view.WindowManager;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.GridView;
import android.widget.TextView;
import android.widget.Toast;

public class RetroBoxMenu extends Activity {
	protected static final String LOGTAG = RetroBoxMenu.class.getSimpleName();
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
		uiMainMenu();
	}
	
	private void uiMainMenu() {
		saveOptionId(RetroActivityFuture.RESULT_CANCEL_ID);
		
		List<ListOption> options = new ArrayList<ListOption>();
        options.add(new ListOption("", getString(R.string.emu_opt_cancel)));
        if (getIntent().getBooleanExtra("CAN_SAVE", false)) {
        	options.add(new ListOption("save", getString(R.string.emu_opt_state_save)));
        	options.add(new ListOption("load", getString(R.string.emu_opt_state_load)));
        }
        //options.add(new ListOption("slot", "Set Save Slot"));
        if (getIntent().hasExtra("MULTIDISK")) {
        	String platform = getIntent().getStringExtra("PLATFORM");
        	if ("psx".equals(platform)) {
        		options.add(new ListOption("disk", getString(R.string.emu_opt_disk_change)));
        	} else {
        		options.add(new ListOption("swap", getString(R.string.emu_opt_disk_swap)));
        	}
        }
        // disable rest (some emulators hang on reset)
        // options.add(new ListOption("reset", "Reset"));
        options.add(new ListOption("help", getString(R.string.emu_opt_help)));
        options.add(new ListOption("quit", getString(R.string.emu_opt_quit)));
		
		RetroBoxDialog.showListDialog(this, getString(R.string.emu_opt_title), options, new Callback<KeyValue>(){
			@Override
			public void onResult(KeyValue result) {
				String key = result.getKey();
				if (key.equals("save")) {
					uiSelectSaveState(RetroActivityFuture.RESULT_SAVE_ID);
					return;
				}
				if (key.equals("load")) {
					uiSelectSaveState(RetroActivityFuture.RESULT_LOAD_ID);
					return;
				}
				
				if (key.equals("disk")) {
					uiSelectDisk();
					return;
				}

				int optionId = RetroActivityFuture.RESULT_CANCEL_ID;
				
				if (key.equals("swap")) optionId = RetroActivityFuture.RESULT_SWAP_ID;
				if (key.equals("reset")) optionId = RetroActivityFuture.RESULT_RESET_ID;
				if (key.equals("quit")) optionId = RetroActivityFuture.RESULT_QUIT_ID;
				if (key.equals("help")) optionId = RetroActivityFuture.RESULT_HELP_ID;

				saveOptionId(optionId);
				
				if (optionId == RetroActivityFuture.RESULT_HELP_ID) {
					uiHelp();
				} else {
					finish();
				}
			}

			@Override
			public void onError() {
				finish();
			}
			
		});
	}
	
	private void saveOptionId(int optionId) {
		saveOptionId(optionId, 0);
	}
	
	private void saveOptionId(int optionId, int param) {
		SharedPreferences preferences = getPreferences();
		Editor editor = preferences.edit();
		editor.putInt("optionId", optionId);
		editor.putInt("param", param);
		editor.commit();
	}
	
	private void uiSelectDisk() {
		int disks = getIntent().getIntExtra("DISKS", 1);
		
		List<ListOption> options = new ArrayList<ListOption>();
		options.add(new ListOption("", "Cancel"));
		for(int i=0; i<disks; i++) {
			int disk = i+1;
			options.add(new ListOption(disk + "", 
					getString(R.string.emu_disk_insert).replace("{n}", String.valueOf(disk))));
		}
		
		RetroBoxDialog.showListDialog(this, getString(R.string.emu_disk_select), options, new Callback<KeyValue>(){

			@Override
			public void onResult(KeyValue result) {
				int disk = Utils.str2i(result.getKey());
				if (disk>0) {
					saveOptionId(RetroActivityFuture.RESULT_DISK_INSERT_ID, disk - 1);
				}
			}

			@Override
			public void onFinally() {
				finish();
			}
		});
	}

	private void uiSelectSaveState(final int optionId) {
		List<SaveStateInfo> list = new ArrayList<SaveStateInfo>();
		String baseName = getIntent().getStringExtra("SAVENAME_BASE");
		File baseDir = new File(baseName).getParentFile();
		File saves[] = baseDir.listFiles();
		
		for(int i=0; i<6; i++) {
			/* ugly workaround before a better solution */
			boolean found = false;
			String ending = ".state" + (i==0?"":i+"");
			if (saves!=null) {
				for(File save : saves) {
					if (save.getName().endsWith(ending)) {
						String fileName = save.getAbsolutePath();
						Log.d(LOGTAG, "Reading filestate discover from " + fileName);
						list.add(new SaveStateInfo(new File(fileName)));
						found = true;
						break;
					}
				}
			}
			if (!found) {
				String fileName = baseName + ".state" + (i==0?"":i+"") ;
				Log.d(LOGTAG, "Reading filestate from " + fileName);
				list.add(new SaveStateInfo(new File(fileName)));
			}
		}
		
		final SaveStateSelectorAdapter adapter = new SaveStateSelectorAdapter(this, list, 
				RetroActivityFuture.saveSlot);
		
		Callback<Integer> callback = new Callback<Integer>() {

			@Override
			public void onResult(Integer index) {
				System.out.println("setting save slot to " + index + " option id " + optionId);
				boolean invalidSlot = optionId == RetroActivityFuture.RESULT_LOAD_ID && 
						!((SaveStateInfo)adapter.getItem(index)).exists();
				
				if (!invalidSlot) {
					RetroActivityFuture.saveSlot = index;
					saveOptionId(optionId);
					RetroBoxDialog.cancelDialog(RetroBoxMenu.this);
				}
			}

			@Override
			public void onFinally() {
				System.out.println("finish menu");
				finish();
			}
			
		};
		
		String title = optionId == RetroActivityFuture.RESULT_SAVE_ID ?
						getString(R.string.emu_slot_save_title) :
						getString(R.string.emu_slot_load_title);
		
		RetroBoxDialog.showSaveStatesDialog(this, title, adapter, callback);
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
	
    private void toastMessage(final String message) {
    	Toast.makeText(this, message, Toast.LENGTH_SHORT).show();
    }

	
}
