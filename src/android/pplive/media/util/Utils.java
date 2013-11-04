/**
 * Copyright (C) 2012 PPTV
 * 
 */
package android.pplive.media.util;

import java.util.concurrent.TimeUnit;


public class Utils { 

	@SuppressWarnings("unused")
	private final static String TAG = "ppmedia/Utils";

	public static void sleep(long timeout, TimeUnit unit) {
		try {
			unit.sleep(timeout);
		} catch (InterruptedException e) {
			LogUtils.error("InterruptedException", e);
		}
	}
	
	/**
	 * Convert the given time duration in the given unit to the destination unit.
	 * 
	 * @param srcDuration
	 * 			the given time duration.
	 * @param srcTimeUnit
	 * 			the given unit.
	 * @param dstTimeUnit
	 * 			the destination unit.
	 * @return
	 * 			the converted duration in the destination unit.
	 */
	public static long convertTime(long srcDuration, TimeUnit srcTimeUnit, TimeUnit dstTimeUnit) {
		return dstTimeUnit.convert(srcDuration, srcTimeUnit);
	}
	
	public static String[] toStringArray(String... args) {
		
		return args;
	}
}
