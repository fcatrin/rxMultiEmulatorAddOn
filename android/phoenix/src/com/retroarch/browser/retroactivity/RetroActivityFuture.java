package com.retroarch.browser.retroactivity;

import java.util.List;

import retrobox.utils.ImmersiveModeSetter;
import retrobox.vinput.Mapper;
import android.app.ActivityManager;
import android.app.ActivityManager.RunningAppProcessInfo;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.SharedPreferences.Editor;
import android.os.Handler;
import android.util.Log;
import android.view.Menu;

public final class RetroActivityFuture extends RetroActivityCamera {
	private static final int REQUEST_CODE_OPTIONS = 0x9292;
	
	private static final int saveSlot = 0;

	private boolean menuRunning = false;
	
	@Override
	public void onResume() {
		SharedPreferences preferences = getPreferences();
		if (preferences.contains("optionId")) {
			int optionId = preferences.getInt("optionId", RESULT_CANCEL_ID);
			Editor editor = preferences.edit();
			editor.remove("optionId");
			editor.commit();
			
			handleOption(optionId);
		}
		
		Log.d("MENU", "RetroActivityFuture onResume start threadId:" + Thread.currentThread().getName());

		ImmersiveModeSetter.postImmersiveMode(new Handler(), getWindow(), isStableLayout());
		
		super.onResume();

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
		SWAP_DISK
	}
	
	public static native void eventCommand(int command);
	public static native void setSaveSlot(int slot);

	
	static final public int RESULT_CANCEL_ID = Menu.FIRST;
    static final public int RESULT_LOAD_ID   = Menu.FIRST + 1;
    static final public int RESULT_SAVE_ID   = Menu.FIRST + 2;
    static final public int RESULT_QUIT_ID   = Menu.FIRST + 3;
    static final public int RESULT_RESET_ID  = Menu.FIRST + 4;
    static final public int RESULT_SWAP_ID   = Menu.FIRST + 5;
    static final public int RESULT_HELP_ID   = Menu.FIRST + 6;

	
    private void uiQuit() {
    	Log.d("MENU", "RetroActivityFuture UI QUIT send threadId:" + Thread.currentThread().getName());
    	try {
    		eventCommand(EventCommand.QUIT.ordinal());
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
	
	protected void handleOption(int optionId) {
		
		Log.e("MENU", "handleOption " + optionId);
	    switch (optionId) {
        case RESULT_LOAD_ID   : uiLoadState(); break;
        case RESULT_SAVE_ID   : uiSaveState(); break;
        case RESULT_SWAP_ID   : uiSwapDisk(); break;
        case RESULT_RESET_ID  : uiReset(); break;
        case RESULT_QUIT_ID   : uiQuit(); break;
        case RESULT_CANCEL_ID : break;
        }

	}
	

}
