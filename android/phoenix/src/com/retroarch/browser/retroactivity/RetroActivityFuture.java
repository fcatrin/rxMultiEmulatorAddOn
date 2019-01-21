package com.retroarch.browser.retroactivity;

import java.util.List;

import retrobox.utils.ImmersiveModeSetter;
import xtvapps.prg.retroarch.R;
import retrobox.utils.RetroBoxUtils;
import retrobox.vinput.Mapper;
import android.app.ActivityManager;
import android.app.ActivityManager.RunningAppProcessInfo;
import android.app.NativeActivity;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.SharedPreferences.Editor;
import android.os.Bundle;
import android.os.Handler;
import android.util.Log;
import android.view.Menu;
import android.widget.Toast;

public final class RetroActivityFuture extends NativeActivity {
	private static final int REQUEST_CODE_OPTIONS = 0x9292;
	
	static int saveSlot = 0;

	private boolean menuRunning = false;
	
	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		
		String userName = getIntent().getStringExtra("username");
		RetroBoxUtils.initExceptionHandler(this, "rxMultiEmulator", userName);
	}

	@Override
	public void onResume() {
		SharedPreferences preferences = getPreferences();
		int optionId = RESULT_CANCEL_ID;
		if (preferences.contains("optionId")) {
			optionId = preferences.getInt("optionId", RESULT_CANCEL_ID);
			int param = preferences.getInt("param", 0);
			
			Editor editor = preferences.edit();
			editor.remove("optionId");
			editor.remove("param");
			editor.commit();
			
			handleOption(optionId, param);
		}
		
		Log.d("MENU", "RetroActivityFuture onResume start threadId:" + Thread.currentThread().getName());

		ImmersiveModeSetter.postImmersiveMode(new Handler(), getWindow(), isStableLayout());
		
		super.onResume();
		
		if (optionId == EventCommand.SAVE_STATE.ordinal()) {
			eventCommand(EventCommand.SCREENSHOT.ordinal());
			toast(this, getString(R.string.emu_slot_saved).replace("{n}", String.valueOf(saveSlot+1)));
		} else if (optionId == EventCommand.LOAD_STATE.ordinal()) {
			toast(this, getString(R.string.emu_slot_loaded).replace("{n}", String.valueOf(saveSlot+1)));
		}

		Log.d("MENU", "RetroActivityFuture onResume end threadId:" + Thread.currentThread().getName());
		menuRunning = false;
	}
	
	private SharedPreferences getPreferences() {
		return getSharedPreferences("RetroBoxMenu", Context.MODE_PRIVATE);
	}
	
    private void setImmersiveMode() {
    	ImmersiveModeSetter.get().setImmersiveMode(getWindow(), isStableLayout());
	}

	private boolean isStableLayout() {
		return Mapper.hasGamepads();
	}
	
	public enum EventCommand {
		QUIT,
		RESET,
		LOAD_STATE,
		SAVE_STATE,
		SWAP_DISK,
		SCREENSHOT,
		DISK_EJECT,
		DISK_INSERT,
		OPEN_MAME_MENU,
		OPEN_MAME_SERVICE
	}
	
	public static native void eventCommand(int command, int command_number);
	public static native void setSaveSlot(int slot);

	public static native void cheatsInit(String path);
	public static native boolean[] cheatsGetStatus();
	public static native String[]  cheatsGetNames();
	public static native void cheatsEnable(int index, boolean enable);
	
	static final public int RESULT_CANCEL_ID = Menu.FIRST;
    static final public int RESULT_LOAD_ID   = Menu.FIRST + 1;
    static final public int RESULT_SAVE_ID   = Menu.FIRST + 2;
    static final public int RESULT_QUIT_ID   = Menu.FIRST + 3;
    static final public int RESULT_RESET_ID  = Menu.FIRST + 4;
    static final public int RESULT_SWAP_ID   = Menu.FIRST + 5;
    static final public int RESULT_HELP_ID   = Menu.FIRST + 6;
    static final public int RESULT_DISK_INSERT_ID    = Menu.FIRST + 7;
    static final public int RESULT_OPEN_MAME_MENU    = Menu.FIRST + 8;
    static final public int RESULT_OPEN_MAME_SERVICE = Menu.FIRST + 9;

	
    public static void eventCommand(int command) {
    	eventCommand(command, 0);
    }
    
    private void uiQuit() {
    	Log.d("MENU", "RetroActivityFuture UI QUIT send threadId:" + Thread.currentThread().getName());
    	try {
    		// eventCommand(EventCommand.QUIT.ordinal());
    	} catch (UnsatisfiedLinkError ue) {
    		// ignore this error. Sometimes the native lib isn't up yet
    	}
		finish();
    	Log.d("MENU", "RetroActivityFuture UI QUIT sent threadId:" + Thread.currentThread().getName());
	}

	private void uiReset() {
		Log.d("MENU", "RetroActivityFuture UI RESET send threadId:" + Thread.currentThread().getName());
		eventCommand(EventCommand.RESET.ordinal());
		Log.d("MENU", "RetroActivityFuture UI RESET sent threadId:" + Thread.currentThread().getName());
	}

	private void uiSaveState() {
		setSaveSlot(saveSlot);
		eventCommand(EventCommand.SAVE_STATE.ordinal());
	}

	private void uiLoadState() {
		setSaveSlot(saveSlot);
		eventCommand(EventCommand.LOAD_STATE.ordinal());
	}
	
	private void uiSwapDisk() {
		eventCommand(EventCommand.SWAP_DISK.ordinal());
	}

	private void uiOpenMAMEMenu() {
		eventCommand(EventCommand.OPEN_MAME_MENU.ordinal());
	}

	private void uiOpenMAMEService() {
		eventCommand(EventCommand.OPEN_MAME_SERVICE.ordinal());
	}

	private void uiInsertDisk(final int diskNumber) {
		final int delay = 500;
		final EventCommand commands[] = {EventCommand.DISK_EJECT, EventCommand.DISK_INSERT, EventCommand.DISK_EJECT};
		final int commandsParam[] = {0, diskNumber, 0};
		
		final Handler handler = new Handler();
		Runnable task = new Runnable() {
			int commandIndex = 0;
			@Override
			public void run() {
				EventCommand command = commands[commandIndex];
				int param = commandsParam[commandIndex];
				eventCommand(command.ordinal(), param);
				
				commandIndex++;
				if (commandIndex<commands.length) {
					handler.postDelayed(this, delay);
				} else {
					handler.post(new Runnable(){
						@Override
						public void run() {
							String msg = getString(R.string.emu_disk_inserted_n).replace("{n}", String.valueOf(diskNumber+1));
							toast(RetroActivityFuture.this, msg);
						}
					});
				}
			}
		};
			
		handler.postDelayed(task, delay);
	}

	public void showOptionsMenu() {
		if (menuRunning) return;
		menuRunning  = true;
		Log.d("MENU", "RetroActivityFuture showOptionsMenu start");
		Intent intent = new Intent(RetroActivityFuture.this, RetroBoxMenu.class);
		intent.fillIn(getIntent(), 0);
		startActivity(intent);
		Log.d("MENU", "RetroActivityFuture showOptionsMenu end");
	}

	private boolean isMenuRunning() {
		ActivityManager activityManager = (ActivityManager) getSystemService( ACTIVITY_SERVICE );
        List<RunningAppProcessInfo> procInfos = activityManager.getRunningAppProcesses();
        for(RunningAppProcessInfo procInfo : procInfos) {
            if (procInfo.processName.equals(RetroBoxMenu.class.getName())) return true; 
        }
        return false;
	}
	
	protected void handleOption(int optionId, int param) {
		
		Log.e("MENU", "handleOption " + optionId);
	    switch (optionId) {
        case RESULT_LOAD_ID   : uiLoadState(); break;
        case RESULT_SAVE_ID   : uiSaveState(); break;
        case RESULT_SWAP_ID   : uiSwapDisk(); break;
        case RESULT_RESET_ID  : uiReset(); break;
        case RESULT_QUIT_ID   : uiQuit(); break;
        case RESULT_DISK_INSERT_ID : uiInsertDisk(param); break;
        case RESULT_OPEN_MAME_MENU : uiOpenMAMEMenu(); break;
        case RESULT_OPEN_MAME_SERVICE : uiOpenMAMEService(); break;
        case RESULT_CANCEL_ID : break;
        }

	}
	
	public static void toast(Context context, String message) {
		Toast.makeText(context, message, Toast.LENGTH_LONG).show();
	}

}
