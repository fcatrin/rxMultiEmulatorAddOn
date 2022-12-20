package com.retroarch.browser.retroactivity;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.SharedPreferences.Editor;
import android.os.Build;
import android.os.Bundle;
import android.util.Log;
import android.view.Window;
import android.view.WindowManager;
import android.widget.Toast;
import retrobox.utils.RetroBoxDialog;
import retrobox.v2.retroarch.R;
import xtvapps.core.Callback;
import xtvapps.core.SimpleCallback;
import xtvapps.core.Utils;

import java.io.File;
import java.io.IOException;

import com.retroarch.browser.preferences.util.UserPreferences;

public class RetroBoxWrapper extends Activity {
	private static final String LOGTAG = RetroBoxWrapper.class.getSimpleName();
	
	private static final String RETROARCH_FUTURE = "retrobox.v2.retroarch.future";
	private static final String RETROARCH_PAST = "retrobox.v2.retroarch.past";
	private static final String KEY_SIGNATURE = "signature";

	private static RetroBoxWrapper instance;
	private boolean started;

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		
		requestWindowFeature(Window.FEATURE_NO_TITLE);
		getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN,
				WindowManager.LayoutParams.FLAG_FULLSCREEN);

		setContentView(R.layout.retrobox_wrapper);
		
		instance = this;
		
		String signature = getIntent().getStringExtra("RETROBOX_SIGNATURE");
		String lastSignature = getLastSignature();
		if (signature.equals(lastSignature)) {
			finish();
			return;
		}
		
		started = false;
		saveLastSignature(signature);
	}
	
	@Override
	public void onStart() {
		super.onStart();
		
		if (started) return;
		started = true;
		
		SimpleCallback onSuccessCallback = new SimpleCallback() {
			@Override
			public void onResult() {
				startTargetActivityUIThread();
			}
		};
		
		Callback<String> onFailureCallback = new Callback<String>(){

			@Override
			public void onResult(String message) {
				displayErrorUIThread(message);
			}
		};
		
		String srcDirPath = getIntent().getStringExtra("RETROBOX_CORES_DIR");
		File srcDir = new File(srcDirPath);
		
		updateLocalCores(srcDir, onSuccessCallback, onFailureCallback);
		
	}
	
	private void startTargetActivityUIThread() {
		runOnUiThread(new Runnable(){
			@Override
			public void run() {
				startTargetActivity();
			}
		});
	}

	private void displayErrorUIThread(final String message) {
		runOnUiThread(new Runnable(){
			@Override
			public void run() {
				displayError(message);
			}
		});
	}

	private void startTargetActivity() {
		setTitle(getIntent().getStringExtra("TITLE"));
		UserPreferences.updateConfigFile(RetroBoxWrapper.this);

		Intent intent = new Intent();
		intent.setAction(Build.VERSION.SDK_INT >= Build.VERSION_CODES.HONEYCOMB ? RETROARCH_FUTURE : RETROARCH_PAST);
		intent.putExtra("CONFIGFILE", UserPreferences.getDefaultConfigPath(RetroBoxWrapper.this));

		intent.fillIn(getIntent(), 0);

		startActivityForResult(intent, 1);
	}
	
	private void updateLocalCores(final File srcDir, final SimpleCallback onSuccessCallback, final Callback<String> onFailureCallback) {
		Thread t = new Thread() {
			@Override
			public void run() {
				File dstDir = new File(getApplicationInfo().dataDir, "cores");
				dstDir.mkdirs();
				
				File srcFiles[] = srcDir.listFiles();
				if (srcFiles == null) return;
				
				for(File srcFile : srcFiles) {
					if (!srcFile.isFile() || srcFile.isHidden() || !srcFile.getName().endsWith(".so")) continue;
					try {
						updateLocalCore(srcFile, dstDir);
					} catch (IOException e) {
						e.printStackTrace();
						
						String msg = "Cannot copy " + srcFile.getName() + " on " + dstDir.getAbsolutePath() + ": " + e.getMessage();
						onFailureCallback.onResult(msg);
						return;
					}
				}
				onSuccessCallback.onResult();
			}
		};
		t.start();
	}
	
	private void updateLocalCore(File srcFile, File dstDir) throws IOException {
		File dstFile = new File(dstDir, srcFile.getName());
		File timestampFile = new File(dstFile.getAbsolutePath() + ".ts");
		
		long dstTimestamp = 0;
		if (timestampFile.exists()) {
			dstTimestamp = Utils.str2l(Utils.loadString(timestampFile));
		}
		
		long srcTimestamp = srcFile.lastModified();
		if (dstFile.exists() && dstTimestamp == srcTimestamp) return;

		// copy core using a temp file in case of failure
		File dstFileTemp = new File(dstFile.getAbsolutePath() + ".tmp");
		try {
			Log.d(LOGTAG, "Updating core " + srcFile.getAbsolutePath() + " -> " + dstFile.getAbsolutePath());
			Utils.copyFile(srcFile, dstFileTemp);
			dstFileTemp.renameTo(dstFile);
			Utils.saveString(timestampFile, String.valueOf(srcTimestamp));
		} catch (IOException e) {
			dstFile.delete();
			timestampFile.delete();
			throw e;
		} finally {
			dstFileTemp.delete();
		}
	
	}

	private void saveLastSignature(String signature) {
		Editor editor = getPreferences().edit();
		editor.putString(KEY_SIGNATURE, signature);
		editor.commit();
	}

	private String getLastSignature() {
		return getPreferences().getString(KEY_SIGNATURE, "");
	}
	
	private SharedPreferences getPreferences() {
		return getSharedPreferences("RetroBoxWrapper", Context.MODE_PRIVATE);
	}

	@Override
	protected void onActivityResult(int requestCode, int resultCode, Intent data) {
		Log.d("RetroArch", "onActivityResult.finish()");
		finish();
	}

	public static void toast(final String message) {
		instance.runOnUiThread(new Runnable(){

			@Override
			public void run() {
				Toast.makeText(instance, message, Toast.LENGTH_LONG).show();
			}
		});
	}
	
	private void displayError(String errorMessage) {
		RetroBoxDialog.showAlert(this, errorMessage, new SimpleCallback(){

			@Override
			public void onFinally() {
				finish();
			}

			@Override
			public void onResult() {}
		});
	}

}
