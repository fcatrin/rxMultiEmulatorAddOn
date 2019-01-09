package com.retroarch.browser;

/**
 * Helper class which calls into JNI for various tasks.
 */
public final class NativeInterface
{
	static
	{
		System.loadLibrary("retroarch-jni");
		System.loadLibrary("retroarch-activity");
	}

	// Disallow explicit instantiation.
	private NativeInterface()
	{
	}

	public static native boolean extractArchiveTo(String archive,
			String subDirectory, String destinationFolder);
	
	public static native void cheatsInit(String path);
	public static native boolean cheatsGetStatus();
	public static native String[] cheatsGetNames();
	public static native void cheatsEnable(int index, boolean enable);
	
}
