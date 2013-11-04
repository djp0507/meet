/**
 * Copyright (C) 2013 PPTV
 *
 */
package android.pplive.media.provider;

import android.content.ContentValues;
import android.content.Context;
import android.database.Cursor;
import android.database.sqlite.SQLiteDatabase;
import android.net.Uri;

/**
 *
 * @author leoxie
 * @version 2013-3-21
 */
public abstract class BaseTable {
	
	public final String COLUMN_ID = "_id";
	public final String COLUMN_DATA = "_data";
	public final String COLUMN_SIZE = "_size";
	public final String COLUMN_DATE_ADDED = "date_added";
	public final String COLUMN_DATE_MODIFIED = "date_modified"; 
	
	abstract int handle(Uri uri);
	
	abstract void createTableIfNotExists(SQLiteDatabase db);
	abstract void dropTableIfExists(SQLiteDatabase db);
	
	abstract String getType(Uri uri);
	
	abstract Cursor query(Context ctx, Uri uri, String[] projection, String selection, String[] selectionArgs, String sortOrder);
	abstract Uri insert(Context ctx, Uri uri, ContentValues values);
	abstract int bulkInsert(Context ctx, Uri uri, ContentValues[] values);
	abstract int update(Context ctx, Uri uri, ContentValues values, String selection, String[] selectionArgs);
	abstract int delete(Context ctx, Uri uri, String where, String[] whereArgs);
}
