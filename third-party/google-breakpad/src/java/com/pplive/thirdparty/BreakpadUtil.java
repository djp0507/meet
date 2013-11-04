package com.pplive.thirdparty;

import java.io.File;

public class BreakpadUtil {

	public static boolean registerBreakpad(File dumpDir) {
		if (dumpDir == null || !dumpDir.exists() || !dumpDir.isDirectory()) {
			throw new IllegalArgumentException("dumpDir is illegal.");
		}

		return registerBreakpad(dumpDir.getAbsolutePath());
	}

	private static native boolean registerBreakpad(String dumpDirPath);

	public static native void unregisterBreakpad();

	static {
		System.loadLibrary("breakpad_util_jni");
	}
}
