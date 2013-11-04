/**
 * Copyright (C) 2013 PPTV
 *
 */
package android.pplive.media.util;

import android.database.Cursor;

/**
 *
 * @author leoxie
 * @version 2013-3-21
 */
public class CursorUtil {
	
	public static byte[] getBlob(Cursor cursor, String columnName) {
		int columnIndex = getColumnIndex(cursor, columnName);
		return cursor.getBlob(columnIndex);
	}
	
	public static double getDouble(Cursor cursor, String columnName) {
		int columnIndex = getColumnIndex(cursor, columnName);
		return cursor.getDouble(columnIndex);
	}
	
	public static float getFloat(Cursor cursor, String columnName) {
		int columnIndex = getColumnIndex(cursor, columnName);
		return cursor.getFloat(columnIndex);
	}
	
	public static int getInt(Cursor cursor, String columnName) {
		int columnIndex = getColumnIndex(cursor, columnName);
		return cursor.getInt(columnIndex);
	}
	
	public static long getLong(Cursor cursor, String columnName) {
		int columnIndex = getColumnIndex(cursor, columnName);
		return cursor.getLong(columnIndex);
	}
	
	public static short getShort(Cursor cursor, String columnName) {
		int columnIndex = getColumnIndex(cursor, columnName);
		return cursor.getShort(columnIndex);
	}
	
	public static String getString(Cursor cursor, String columnName) {
		int columnIndex = getColumnIndex(cursor, columnName);
		return cursor.getString(columnIndex);
	}
	
	public static int getColumnIndex(Cursor cursor, String columnName) {
		return cursor.getColumnIndexOrThrow(columnName);
	}
}
