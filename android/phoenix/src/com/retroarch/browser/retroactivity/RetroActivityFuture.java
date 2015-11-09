package com.retroarch.browser.retroactivity;

import retrobox.utils.ImmersiveModeSetter;
import retrobox.vinput.Mapper;
import android.app.Activity;
import android.content.Intent;
import android.os.Handler;
import android.util.Log;
import android.view.Menu;

public final class RetroActivityFuture extends RetroActivityCamera {
	private static final int REQUEST_CODE_OPTIONS = 0x9292;

	@Override
	public void onResume() {
		super.onResume();

		ImmersiveModeSetter.postImmersiveMode(new Handler(), getWindow(), isStableLayout());
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
		SAVE_SLOT_PLUS,
		SAVE_SLOT_MINUS,
		SWAP_DISK
	}
	
	public static native void eventCommand(int command);

	
	static final public int RESULT_CANCEL_ID = Menu.FIRST;
    static final public int RESULT_LOAD_ID = Menu.FIRST +1;
    static final public int RESULT_SAVE_ID = Menu.FIRST +2;
    static final public int RESULT_QUIT_ID = Menu.FIRST +3;
    static final public int RESULT_RESET_ID = Menu.FIRST +4;
    static final public int RESULT_SAVE_SLOT_PLUS  = Menu.FIRST +5;
    static final public int RESULT_SAVE_SLOT_MINUS = Menu.FIRST +6;
    static final public int RESULT_SWAP_ID = Menu.FIRST +7;

	
    private void uiQuit() {
    	eventCommand(EventCommand.QUIT.ordinal());
	}

	private void uiReset() {
		eventCommand(EventCommand.RESET.ordinal());
	}

	private void uiSaveState() {
		eventCommand(EventCommand.SAVE_STATE.ordinal());
	}

	private void uiLoadState() {
		eventCommand(EventCommand.LOAD_STATE.ordinal());
	}

	private void uiNextSaveSlot() {
		eventCommand(EventCommand.SAVE_SLOT_PLUS.ordinal());
	}
	
	private void uiPrevSaveSlot() {
		eventCommand(EventCommand.SAVE_SLOT_MINUS.ordinal());
	}
	
	private void uiSwapDisk() {
		eventCommand(EventCommand.SWAP_DISK.ordinal());
	}

	public void showOptionsMenu() {
		runOnUiThread(new Runnable() {
			@Override
			public void run() {
				Intent intent = new Intent(RetroActivityFuture.this, RetroBoxMenu.class);
				if (getIntent().hasExtra("MULTIDISK")) {
					intent.getExtras().putBoolean("MULTIDISK", true);	
				}
				startActivityForResult(intent, REQUEST_CODE_OPTIONS);
			}
		});
	}

	@Override
	protected void onActivityResult(int requestCode, int resultCode, Intent data) {
		super.onActivityResult(requestCode, resultCode, data);
		
		Log.e("MENU", "on activity result " + requestCode + ", " + resultCode + ", intent:" + data);
		
		if (requestCode == REQUEST_CODE_OPTIONS && resultCode == Activity.RESULT_OK) {
			int optionId = data.getIntExtra("optionId", RESULT_CANCEL_ID);
			
	        switch (optionId) {
	        case RESULT_LOAD_ID   : uiLoadState(); break;
	        case RESULT_SAVE_ID   : uiSaveState(); break;
	        case RESULT_SAVE_SLOT_PLUS   : uiNextSaveSlot(); break;
	        case RESULT_SAVE_SLOT_MINUS  : uiPrevSaveSlot(); break;
	        case RESULT_SWAP_ID  : uiSwapDisk(); break;
	        case RESULT_RESET_ID  : uiReset(); break;
	        case RESULT_QUIT_ID   : uiQuit(); break;
	        case RESULT_CANCEL_ID : break;
	        }
		}
	}
	

}
