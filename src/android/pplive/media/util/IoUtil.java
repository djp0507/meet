/**
 * Copyright (C) 2013 PPTV
 *
 */
package android.pplive.media.util;

import java.io.File;

/**
 *
 * @author leoxie
 * @version 2013-3-20
 */
public class IoUtil {

	public static boolean delete(String filePath) {
		File f = new File(filePath);
		
		return f.exists() ? f.delete() : true;
	}
	
	public static boolean isAccessible(File file) {
		return file != null && file.exists() && file.canRead();
	}

}
