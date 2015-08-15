package com.retroarch.browser.retroactivity;

import android.view.View;

public final class RetroActivityFuture extends RetroActivityCamera {

	@Override
	public void onResume() {
		super.onResume();

		if (android.os.Build.VERSION.SDK_INT >= 19) {
			View thisView = getWindow().getDecorView();
			RetroBoxWrapper.setImmersiveMode(thisView);
		}
	}

}
