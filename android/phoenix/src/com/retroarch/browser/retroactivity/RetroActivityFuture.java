package com.retroarch.browser.retroactivity;

import retrobox.vinput.GenericGamepad;
import retrobox.vinput.GenericGamepad.Analog;
import retrobox.vinput.Mapper;
import retrobox.vinput.Mapper.ShortCut;
import retrobox.vinput.QuitHandler;
import retrobox.vinput.QuitHandler.QuitHandlerCallback;
import retrobox.vinput.VirtualEvent.MouseButton;
import retrobox.vinput.VirtualEventDispatcher;
import android.app.Instrumentation;
import android.os.Bundle;
import android.os.Handler;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.KeyEvent;
import android.view.Menu;
import android.view.MenuItem;
import android.view.MotionEvent;
import android.widget.Toast;

public final class RetroActivityFuture extends RetroActivityCamera {
    private static final int RETRO_DEVICE_ID_JOYPAD_B      = 1 << 0;
    private static final int RETRO_DEVICE_ID_JOYPAD_Y      = 1 << 1;
    private static final int RETRO_DEVICE_ID_JOYPAD_SELECT = 1 << 2;
    private static final int RETRO_DEVICE_ID_JOYPAD_START  = 1 << 3;
    private static final int RETRO_DEVICE_ID_JOYPAD_UP     = 1 << 4;
    private static final int RETRO_DEVICE_ID_JOYPAD_DOWN   = 1 << 5;
    private static final int RETRO_DEVICE_ID_JOYPAD_LEFT   = 1 << 6;
    private static final int RETRO_DEVICE_ID_JOYPAD_RIGHT  = 1 << 7;
    private static final int RETRO_DEVICE_ID_JOYPAD_A      = 1 << 8;
    private static final int RETRO_DEVICE_ID_JOYPAD_X      = 1 << 9;
    private static final int RETRO_DEVICE_ID_JOYPAD_L      = 1 << 10;
    private static final int RETRO_DEVICE_ID_JOYPAD_R      = 1 << 11;
    private static final int RETRO_DEVICE_ID_JOYPAD_L2     = 1 << 12;
    private static final int RETRO_DEVICE_ID_JOYPAD_R2     = 1 << 13;
    private static final int RETRO_DEVICE_ID_JOYPAD_L3     = 1 << 14;
    private static final int RETRO_DEVICE_ID_JOYPAD_R3     = 1 << 15;
    
    private static final int retroCodes[] = {
            RETRO_DEVICE_ID_JOYPAD_UP, RETRO_DEVICE_ID_JOYPAD_DOWN, RETRO_DEVICE_ID_JOYPAD_LEFT, RETRO_DEVICE_ID_JOYPAD_RIGHT,
            RETRO_DEVICE_ID_JOYPAD_A, RETRO_DEVICE_ID_JOYPAD_B, RETRO_DEVICE_ID_JOYPAD_X, RETRO_DEVICE_ID_JOYPAD_Y,
            RETRO_DEVICE_ID_JOYPAD_L, RETRO_DEVICE_ID_JOYPAD_R, RETRO_DEVICE_ID_JOYPAD_L2, RETRO_DEVICE_ID_JOYPAD_R2,
            RETRO_DEVICE_ID_JOYPAD_L3, RETRO_DEVICE_ID_JOYPAD_R3, RETRO_DEVICE_ID_JOYPAD_SELECT, RETRO_DEVICE_ID_JOYPAD_START
    };
    
    Handler handler = new Handler();
    	
    
    @Override
    public void onCreate(Bundle savedInstanceState) {
            super.onCreate(savedInstanceState);
            RetroBoxWrapper.setImmersiveMode(getWindow().getDecorView());
            setupMapper();
            Log.d("SHORTCUT", "onCreate");
    }
    
    public void onWindowFocusChanged(boolean hasFocus) {
    super.onWindowFocusChanged(hasFocus);
    if (hasFocus) new Handler().postDelayed(new Runnable(){
                    @Override
                    public void run() {
                            RetroBoxWrapper.setImmersiveMode(getWindow().getDecorView());
                    }
            }, 5000);
    }

    private Mapper mapper;    

 // custom swipe handling
    private static final int MAX_POINTER = 15;
    private static int pointerX[] = new int[MAX_POINTER];
    private static int pointerY[] = new int[MAX_POINTER];
    private static long downTime[] = new long[MAX_POINTER];
    private static DisplayMetrics displayMetrics = null;
    private static int screenWidth = 0;
    private static int screenHeight = 0;
    
    public void handleMotion(int pointer, int action, int x, int y) {
            if (displayMetrics == null) {
                    displayMetrics = new DisplayMetrics();
                    getWindowManager().getDefaultDisplay().getMetrics(displayMetrics);
                    screenWidth = displayMetrics.widthPixels;
                    screenHeight = displayMetrics.heightPixels;
            }
            if (pointer>=MAX_POINTER) return;
            
            boolean isDown = action == MotionEvent.ACTION_DOWN || action == MotionEvent.ACTION_POINTER_DOWN || action == MotionEvent.ACTION_CANCEL;
            boolean isUp = action == MotionEvent.ACTION_UP || action == MotionEvent.ACTION_POINTER_UP;
            if (isDown) {
                    downTime[pointer] = System.currentTimeMillis();
                    pointerX[pointer] = x;
                    pointerY[pointer] = y;
                    return;
            }
            if (!isUp || downTime[pointer] == 0) return;
            
            long dt =  System.currentTimeMillis() - downTime[pointer];
            downTime[pointer] = 0;
            
            if (dt == 0) return;
            
            float dx = (float)(x - pointerX[pointer]) / screenWidth;
            float dy = (float)(y - pointerY[pointer]) / screenHeight;
            
            float vx = dx*100.0f / dt;
            float vy = dy*100.0f / dt;
            
            // only left swipe for now
            if (dx < -0.25 && vx < -0.09) {
                    onMenuPressedAsync();
            }
            
    }
    
    public int handleSpecialKey(int keyCode, int down) {
            if (keyCode == KeyEvent.KEYCODE_BACK || keyCode == KeyEvent.KEYCODE_MENU) {
                    if (down == 0) {
                            if (keyCode == KeyEvent.KEYCODE_BACK) onBackPressedAsync();
                            else onMenuPressedAsync();
                    }
                    return 1;
            }
            return 0;
    }

    public int handleShortcut(String descriptor, int port, int keyCode, int down) {
            GenericGamepad gamepad = mapper.resolveGamepad(descriptor, port);
            if (gamepad!=null) {
                    boolean handled = mapper.handleShortcut(gamepad, keyCode, down!=0);
                    return handled?1:0;
            }
            return 0;
    }
    
    private void onBackPressedAsync() {
            Log.d("SHORTCUT", "onBackPressedAsync");
            handler.post(new Runnable(){
                    @Override
                    public void run() {
                            onBackPressed();
                    }
            });
    }
    
    private void onMenuPressedAsync() {
            Log.d("SHORTCUT", "onMenuPressedAsync");
            handler.post(new Runnable(){
                    @Override
                    public void run() {
                            openOptionsMenu();
                    }
            });
    }

    public void setupMapper() {
            
            VirtualEventDispatcher vinputDispatcher = new VirtualInputDispatcher();
            mapper = new Mapper(getIntent(), vinputDispatcher) {

                    @Override
                    protected boolean isStartButton(GenericGamepad gamepad, int keyCode) {
                            return (keyCode & RETRO_DEVICE_ID_JOYPAD_START) != 0;
                    }

                    @Override
                    protected int getOriginCode(GenericGamepad gamepad, int keyCode) {
                            int translated = -1;
                            for(int i=0; translated<0 && i<retroCodes.length; i++) {
                                    int retroCode = retroCodes[i];
                                    if (retroCode == RETRO_DEVICE_ID_JOYPAD_START) continue; // ignore start status. this is for shortcuts only
                                    if ((keyCode & retroCode) != 0) {
                                            return gamepad.originCodes[i];
                                    }
                            }
                            return 0;
                    }

                    @Override
                    protected void sendStartKeyPress(final GenericGamepad gamepad) {
                            final int translated = getTranslatedVirtualEvent(gamepad, KeyEvent.KEYCODE_BUTTON_START);
                            if (translated!=0) {
                                    Thread t = new Thread(){
                                            @Override
                                            public void run() {
                                                    int keyCode = (gamepad.getDeviceId() << 16) | 0x0C000 | translated; // magic number to avoid reentrant handling
                                                    
                                                    Instrumentation inst = new Instrumentation();
                                                    
                                                    KeyEvent k1 = new KeyEvent(KeyEvent.ACTION_DOWN, keyCode);
                                                    inst.sendKeySync(k1);
                                                    
                                                    try {Thread.sleep(500);} catch (Exception e) {}
                                                    
                                                    KeyEvent k2 = new KeyEvent(KeyEvent.ACTION_UP, keyCode);
                                                    inst.sendKeySync(k2);
                                            }
                                    };
                                    t.start();
                            } else {
                            }
                    }
                    
            };
            
            Mapper.initGestureDetector(this);
    for(int i=0; i<2; i++) {
            String prefix = "j" + (i+1);
            String deviceDescriptor = getIntent().getStringExtra(prefix + "DESCRIPTOR");
            Mapper.registerGamepad(i, deviceDescriptor);
    }
    }
    
protected void toastMessage(String message) {
    Toast.makeText(this, message, Toast.LENGTH_SHORT).show();
}

    @Override
    public void onBackPressed() {
            uiQuitConfirm();
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
public boolean onMenuItemSelected(int featureId, MenuItem item) {
    if (item != null) {
            switch (item.getItemId()) {
            case LOAD_ID : uiLoadState(); return true;
            case SAVE_ID : uiSaveState(); return true;
            case QUIT_ID : uiQuit(); return true;
            case CANCEL_ID : return true;
            }
    }
    return super.onMenuItemSelected(featureId, item);
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
    
protected void uiLoadState() {
    sendLoadState(true);
    new Handler().postDelayed(new Runnable(){
                    @Override
                    public void run() {
                            sendLoadState(false);
                            toastMessage("State was restored");
                    }
            }, 50);
}

protected void uiSaveState() {
    sendSaveState(true);
    new Handler().postDelayed(new Runnable(){
                    @Override
                    public void run() {
                            sendSaveState(false);
                            toastMessage("State was saved");
                    }
            }, 50);
}

protected void sendLoadState(boolean pressed) {
    processShortcut(ShortCut.LOAD_STATE.ordinal(), pressed?1:0);
}

protected void sendSaveState(boolean pressed) {
    processShortcut(ShortCut.SAVE_STATE.ordinal(), pressed?1:0);
}

protected void uiQuitConfirm() {
    QuitHandler.askForQuit(this, new QuitHandlerCallback() {
                    @Override
                    public void onQuit() {
                            uiQuit();
                    }
            });
}

protected void uiQuit() {
    new Handler().postDelayed(new Runnable(){
                    @Override
                    public void run() {
                    processShortcut(ShortCut.EXIT.ordinal(), 1);
                    uiQuitUp();
                    }
            }, 50);

}

protected void uiQuitUp() {
    new Handler().postDelayed(new Runnable(){
                    @Override
                    public void run() {
                    processShortcut(ShortCut.EXIT.ordinal(), 0);
                    }
            }, 1250);
    
}
    
    class VirtualInputDispatcher implements VirtualEventDispatcher {

            @Override
            public void sendKey(int keyCode, boolean down) {} // not used for shortcuts

            @Override
            public void sendMouseButton(MouseButton button, boolean down) {} // not used for shortcuts

            @Override
            public void sendAnalog(Analog index, double x, double y) {}

            @Override
            public boolean handleShortcut(ShortCut shortcut, boolean down) {
                    if (shortcut == ShortCut.MENU || shortcut == ShortCut.EXIT) {
                            if (!down) {
                                    if (shortcut == ShortCut.MENU) onMenuPressedAsync();
                                    if (shortcut == ShortCut.EXIT) onBackPressedAsync();
                            }
                            return true;
                    }
                    if (processShortcut(shortcut.ordinal(), down?1:0)) {
                            if (!down) {
                                    String msg = null;
                                    if (shortcut == ShortCut.LOAD_STATE) msg = "State was restored";
                                    else if (shortcut == ShortCut.SAVE_STATE) msg = "State was saved";
                                    if (msg!=null) {
                                            final String message = msg;
                                            handler.post(new Runnable() {
                                                    @Override
                                                    public void run() {
                                                            toastMessage(message);
                                                    }
                                            });
                                    }
                            }
                            
                            return true;
                    }
                    return false;
            }
            
    }
    
    private static native boolean processShortcut(int shortcut, int down);

}
