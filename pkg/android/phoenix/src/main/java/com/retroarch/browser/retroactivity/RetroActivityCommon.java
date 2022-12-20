package com.retroarch.browser.retroactivity;

import android.app.NativeActivity;

import com.retroarch.browser.preferences.util.UserPreferences;

/**
 * Class which provides common methods for RetroActivity related classes.
 */
public class RetroActivityCommon extends NativeActivity
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
		finish();
	}

}
