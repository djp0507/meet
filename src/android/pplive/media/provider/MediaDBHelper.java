/**
 * Copyright (C) 2013 PPTV
 *
 */
package android.pplive.media.provider;

import android.content.Context;
import android.database.sqlite.SQLiteDatabase;
import android.database.sqlite.SQLiteOpenHelper;

/**
 *
 * @author leoxie
 * @version 2013-3-18
 */
public class MediaDBHelper extends SQLiteOpenHelper {
	
	private static final String DATABASE_NAME = "media.db";
	private static final int DATABASE_VERSION = 1;
	
	private static MediaDBHelper mInstance;
	
	public static synchronized MediaDBHelper getInstance(Context ctx) {
		if (null == mInstance) {
			mInstance = new MediaDBHelper(ctx.getApplicationContext());
		}
		
		return mInstance;
	}

	/**
	 * @param context to use to open or create the database
	 */
	public MediaDBHelper(Context context) {
		super(context, DATABASE_NAME, null, DATABASE_VERSION);
	}
	
	/* 
	 * @see android.database.sqlite.SQLiteOpenHelper#onCreate(android.database.sqlite.SQLiteDatabase)
	 */
	@Override
	public void onCreate(SQLiteDatabase db) {
		createIfNotExists(db);
	}
	
	private void createIfNotExists(SQLiteDatabase db) {
		MediaMetadata.Video.createTableIfNotExists(db);
	}
	
	/* 
	 * @see android.database.sqlite.SQLiteOpenHelper#onUpgrade(android.database.sqlite.SQLiteDatabase, int, int)
	 */
	@Override
	public void onUpgrade(SQLiteDatabase db, int oldVersion, int newVersion) {
		dropIfExists(db);
		
		onCreate(db);
	}
	
	private void dropIfExists(SQLiteDatabase db) {
		MediaMetadata.Video.dropTableIfExists(db);
	}
}
