/**
 * Copyright (C) 2013 PPTV
 *
 */
package android.pplive.media.provider;

import java.util.ArrayList;
import java.util.List;

import android.content.UriMatcher;
import android.net.Uri;
import android.pplive.media.util.LogUtils;

/**
 *
 * @author leoxie
 * @version 2013-3-19
 */
public final class MediaMetadata {
	
	public static final VideoTable Video = new VideoTable();
	
	private static final List<BaseTable> Tables = new ArrayList<BaseTable>();
	
	static {
		Tables.add(Video);
	}
	
	public static BaseTable dispatchTable(Uri uri) {
		
		for (BaseTable table : Tables) {
			if (table.handle(uri) != UriMatcher.NO_MATCH) {
				return table;
			}
		}
		
		LogUtils.error("Unknown URI " + uri);
		throw new IllegalArgumentException("Unknown URI " + uri);
	}
}
