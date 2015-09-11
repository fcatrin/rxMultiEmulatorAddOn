package com.retroarch.browser.retroactivity;

import android.os.Handler;
import android.view.Menu;

import com.retroarch.browser.preferences.util.UserPreferences;

/**
 * Class which provides common methods for RetroActivity related classes.
 */
public class RetroActivityCommon extends RetroActivityLocation
{
	
	@Override
	public void onLowMemory()
	{
	}

	@Override
	public void onTrimMemory(int level)
	{
	}

	// Exiting cleanly from NDK seems to be nearly impossible.
	// Have to use exit(0) to avoid weird things happening, even with runOnUiThread() approaches.
	// Use a separate JNI function to explicitly trigger the readback.
	public void onRetroArchExit()
	{
		UserPreferences.readbackConfigFile(this);
	}
	
	static final private int CANCEL_ID = Menu.FIRST;
    static final private int LOAD_ID = Menu.FIRST +1;
    static final private int SAVE_ID = Menu.FIRST +2;
    static final private int QUIT_ID = Menu.FIRST +3;
    
    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        super.onCreateOptionsMenu(menu);

        menu.add(0, CANCEL_ID, 0, "Cancel");
        menu.add(0, LOAD_ID, 0, "Load State");
        menu.add(0, SAVE_ID, 0, "Save State");
        menu.add(0, QUIT_ID, 0, "Quit");
        
        return true;
    }
	
    @Override
	public boolean onMenuOpened(int featureId, Menu menu) {
		onPause();
		return super.onMenuOpened(featureId, menu);
	}

	@Override
	public void onOptionsMenuClosed(Menu menu) {
		onResume();
		super.onOptionsMenuClosed(menu);
	}
    
	public void showOptionsMenu() {
		runOnUiThread(new Runnable() {
			@Override
			public void run() {
				openOptionsMenu();
			}
		});
	}
	
}
