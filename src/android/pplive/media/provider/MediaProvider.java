/**
 * Copyright (C) 2013 PPTV
 *
 */
package android.pplive.media.provider;

import android.content.ContentProvider;
import android.content.ContentValues;
import android.database.Cursor;
import android.net.Uri;
import android.pplive.media.util.LogUtils;

/**
 *
 * @author leoxie
 * @version 2013-3-18
 */
public class MediaProvider extends ContentProvider {
	
	@SuppressWarnings("unused")
	private static final String TAG = "ppmedia/MediaProvider";
	
	static final String AUTHORITY = "android.pplive.media";
	static final String CONTENT_AUTHORITY_SLASH = "content://" + AUTHORITY + "/";
	
	static Uri getContentUri(String tableName) {
		return Uri.parse(CONTENT_AUTHORITY_SLASH + tableName);
	}
	
	static Uri getContentUri(String tableName, int rowId) {
		return Uri.parse(CONTENT_AUTHORITY_SLASH + tableName + "/" + rowId);
	}
	/* 
	 * @see android.content.ContentProvider#onCreate()
	 */
	@Override
	public boolean onCreate() {
		return true;
	}
	
	/* 
	 * @see android.content.ContentProvider#getType(android.net.Uri)
	 */
	@Override
	public String getType(Uri uri) {
		LogUtils.info("getType" + uri);
		
		BaseTable table = MediaMetadata.dispatchTable(uri);
		return table.getType(uri);
	}
	
	/* 
	 * @see android.content.ContentProvider#query(android.net.Uri, java.lang.String[], java.lang.String, java.lang.String[], java.lang.String)
	 */
	@Override
	public Cursor query(Uri uri, String[] projection, String selection, String[] selectionArgs, String sortOrder) {
	    LogUtils.info("query" + uri);
		
		BaseTable table = MediaMetadata.dispatchTable(uri);
		return table.query(getContext(), uri, projection, selection, selectionArgs, sortOrder);
	}

	/* 
	 * @see android.content.ContentProvider#insert(android.net.Uri, android.content.ContentValues)
	 */
	@Override
	public Uri insert(Uri uri, ContentValues values) {
	    LogUtils.info("insert" + uri);

		BaseTable table = MediaMetadata.dispatchTable(uri);
		return table.insert(getContext(), uri, values);
	}
	
	/* 
	 * @see android.content.ContentProvider#bulkInsert(android.net.Uri, android.content.ContentValues[])
	 */
	@Override
	public int bulkInsert(Uri uri, ContentValues[] values) {
	    LogUtils.info("bulkInsert" + uri);
		BaseTable table = MediaMetadata.dispatchTable(uri);
		return table.bulkInsert(getContext(), uri, values);
	}

	/* 
	 * @see android.content.ContentProvider#delete(android.net.Uri, java.lang.String, java.lang.String[])
	 */
	@Override
	public int delete(Uri uri, String where, String[] whereArgs) {
	    LogUtils.info("delete" + uri);
		
		BaseTable table = MediaMetadata.dispatchTable(uri);
		return table.delete(getContext(), uri, where, whereArgs);
	}
	
	/* 
	 * @see android.content.ContentProvider#update(android.net.Uri, android.content.ContentValues, java.lang.String, java.lang.String[])
	 */
	@Override
	public int update(Uri uri, ContentValues values, String selection, String[] selectionArgs) {
	    LogUtils.info("update" + uri);
		
		BaseTable table = MediaMetadata.dispatchTable(uri);
		return table.update(getContext(), uri, values, selection, selectionArgs);
	}
}
