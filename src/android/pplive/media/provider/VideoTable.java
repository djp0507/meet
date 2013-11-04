/**
 * Copyright (C) 2013 PPTV
 *
 */
package android.pplive.media.provider;

import android.content.ContentUris;
import android.content.ContentValues;
import android.content.Context;
import android.content.UriMatcher;
import android.database.Cursor;
import android.database.SQLException;
import android.database.sqlite.SQLiteDatabase;
import android.database.sqlite.SQLiteQueryBuilder;
import android.net.Uri;
import android.pplive.media.util.CursorUtil;
import android.pplive.media.util.IoUtil;
import android.pplive.media.util.LogUtils;

/**
 *
 * @author leoxie
 * @version 2013-3-21
 */
public final class VideoTable extends BaseTable {
	public final String TABLE_NAME = "video";
	public final String COLUMN_TITLE = "title";
	public final String COLUMN_DURATION = "duration";
	public final String COLUMN_MIME_TYPE = "mime_type";
	
	public final Uri CONTENT_URI = MediaProvider.getContentUri(TABLE_NAME);
	
	private final String SQL_CREATE_TABLE = "CREATE TABLE IF NOT EXISTS " + TABLE_NAME +" (" + 
			COLUMN_ID + " INTEGER PRIMARY KEY AUTOINCREMENT, " + 
			COLUMN_DATA + " TEXT, " + 
			COLUMN_TITLE + " TEXT, " +
			COLUMN_DURATION + " INTEGER, " + 
			COLUMN_SIZE + " INTEGER, " + 
			COLUMN_MIME_TYPE + " INTEGER, " + 
			COLUMN_DATE_ADDED + " INTEGER, " +
			COLUMN_DATE_MODIFIED + " INTEGER" + ");";
	
	private final String SQL_DROP_TABLE = "DROP TABLE IF EXISTS " + TABLE_NAME + ";";
	
	private static final int VIDEO_COLLECTION_URI_INDICATOR = 1;
	private static final int VIDEO_SINGLE_URI_INDICATOR = 2;
	
	private UriMatcher sUriMatcher;
	
	VideoTable() {
		sUriMatcher = new UriMatcher(UriMatcher.NO_MATCH);
		
		sUriMatcher.addURI(MediaProvider.AUTHORITY, TABLE_NAME, VIDEO_COLLECTION_URI_INDICATOR);
		sUriMatcher.addURI(MediaProvider.AUTHORITY, TABLE_NAME + "/#", VIDEO_SINGLE_URI_INDICATOR);
	};
	
	/* 
	 * @see android.ppmedia.provider.BaseTable#createTable(android.database.sqlite.SQLiteDatabase, java.lang.String)
	 */
	@Override
	void createTableIfNotExists(SQLiteDatabase db) {
		db.execSQL(SQL_CREATE_TABLE);		
	}
	
	/* 
	 * @see android.ppmedia.provider.BaseTable#dropTableIfExists(android.database.sqlite.SQLiteDatabase)
	 */
	@Override
	void dropTableIfExists(SQLiteDatabase db) {
		db.execSQL(SQL_DROP_TABLE);
	}
	
	/* 
	 * @see android.ppmedia.provider.BaseTable#dispatch(android.net.Uri)
	 */
	@Override
	int handle(Uri uri) {
		return sUriMatcher.match(uri);
	}
	
	private final String MEDIA_TYPE = "video"; 

	/* 
	 * @see android.ppmedia.provider.BaseTable#getType(android.net.Uri)
	 */
	@Override
	String getType(Uri uri) {
		if (sUriMatcher.match(uri) != UriMatcher.NO_MATCH) {
			return MEDIA_TYPE;
		}
		
		throw new IllegalArgumentException("Unknown URI " + uri);
	}
	
	/* 
	 * @see android.ppmedia.provider.BaseTable#query(android.content.Context, android.net.Uri, java.lang.String[], java.lang.String, java.lang.String[], java.lang.String)
	 */
	@Override
	Cursor query(Context ctx, Uri uri, String[] projection, String selection, String[] selectionArgs, String sortOrder) {
		SQLiteQueryBuilder qb = new SQLiteQueryBuilder();
		qb.setTables(TABLE_NAME);
		
		SQLiteDatabase db = MediaDBHelper.getInstance(ctx.getApplicationContext()).getReadableDatabase();
		
		Cursor cursor = qb.query(db, projection, selection, selectionArgs, null, null, sortOrder);
		if (null != cursor) {
			cursor.setNotificationUri(ctx.getContentResolver(), CONTENT_URI);
		}
		
		return cursor;
	}
	
	/* 
	 * @see android.ppmedia.provider.BaseTable#insert(android.content.Context, android.net.Uri, android.content.ContentValues)
	 */
	@Override
	Uri insert(Context ctx, Uri uri, ContentValues values) {
		if (sUriMatcher.match(uri) != VIDEO_COLLECTION_URI_INDICATOR) {
			throw new IllegalArgumentException("Unknown URI " + uri);
		} else if (null == values) {
			throw new IllegalArgumentException("Invalid values");
		}
		
 		SQLiteDatabase db = MediaDBHelper.getInstance(ctx.getApplicationContext()).getWritableDatabase();
 		db.beginTransaction();
		try {
			long rowId = db.insert(TABLE_NAME, null, values);
			if (rowId > 0) {
				db.setTransactionSuccessful();
				
				Uri notifyUri = ContentUris.withAppendedId(CONTENT_URI, rowId);
				ctx.getApplicationContext().getContentResolver().notifyChange(notifyUri, null);
				
				return notifyUri;
			}
		} finally {
			db.endTransaction();
		}
		
		throw new SQLException("Failed to insert row into " + uri);
	}
	
	/* 
	 * @see android.ppmedia.provider.BaseTable#bulkInsert(android.content.Context, android.net.Uri, android.content.ContentValues[])
	 */
	@Override
	int bulkInsert(Context ctx, Uri uri, ContentValues[] values) {
		if (sUriMatcher.match(uri) != VIDEO_COLLECTION_URI_INDICATOR) {
			throw new IllegalArgumentException("Unknown URI " + uri);
		} else if (null == values) {
			throw new IllegalArgumentException("Invalid values");
		}
		
		SQLiteDatabase db = MediaDBHelper.getInstance(ctx.getApplicationContext()).getWritableDatabase();
 		db.beginTransaction();
 		try {
 			for (int i = 0; i < values.length; ++i) {
 				if (db.insert(TABLE_NAME, null, values[i]) < 0) {
 					return 0;
 				}
 			}
 			
 			db.setTransactionSuccessful();
		} finally {
			db.endTransaction();
		}
 		
 		ctx.getApplicationContext().getContentResolver().notifyChange(uri, null);
 		return values.length;
	}
	
	/* 
	 * @see android.ppmedia.provider.BaseTable#update(android.content.Context, android.net.Uri, android.content.ContentValues, java.lang.String, java.lang.String[])
	 */
	@Override
	int update(Context ctx, Uri uri, ContentValues values, String selection, String[] selectionArgs) {
		// TODO: Don't support update.
		return 0;
	}
	
	/* 
	 * @see android.ppmedia.provider.BaseTable#delete(android.net.Uri, java.lang.String, java.lang.String)
	 */
	@Override
	int delete(Context ctx, Uri uri, String where, String[] whereArgs) {
		int count = 0;
		
		switch(sUriMatcher.match(uri)) {
		case VIDEO_COLLECTION_URI_INDICATOR:
			count = this.delete(ctx, TABLE_NAME, where, whereArgs);
			break;
		case VIDEO_SINGLE_URI_INDICATOR:
			long rowId = ContentUris.parseId(uri);
			count = this.delete(ctx, TABLE_NAME, COLUMN_ID + " = " + rowId, null);
			break;
		default:
		    LogUtils.error("Unknown URI " + uri);
			throw new IllegalArgumentException("Unknown URI " + uri);
		}
		
		if (count > 0) {
			ctx.getContentResolver().notifyChange(CONTENT_URI, null);
		}
		
		return count;
	}

	private int delete(Context ctx, String table, String where, String[] whereArgs) {
		int count = 0;
		
		SQLiteDatabase db = MediaDBHelper.getInstance(ctx.getApplicationContext()).getWritableDatabase();
		db.beginTransaction();
		try {
			Cursor cursor = db.query(table, null, where, whereArgs, null, null, null);
			
			if (null != cursor) {
				
				for (cursor.moveToFirst(); !cursor.isAfterLast(); cursor.moveToNext()) {
					String filePath = CursorUtil.getString(cursor, MediaMetadata.Video.COLUMN_DATA);
					IoUtil.delete(filePath);
				}
				
				cursor.close();
				count = db.delete(table, where, whereArgs);
			}
			
			db.setTransactionSuccessful();
		} finally {
			db.endTransaction();
		}
		
		return count;
	}
}
