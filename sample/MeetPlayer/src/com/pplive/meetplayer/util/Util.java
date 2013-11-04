package com.pplive.meetplayer.util;

import android.content.Context;
import android.util.Log;

import com.pplive.sdk.MediaSDK;

public class Util {
	
	public static boolean startP2PEngine(Context context) {
		Log.d("Util", "startP2PEngine()");
		if (context == null) {
			return false;
		}

		String gid = "12";
		String pid = "161";
		String auth = "08ae1acd062ea3ab65924e07717d5994";

		String libPath = "/data/data/com.pplive.meetplayer/lib";
		MediaSDK.libPath = libPath;
		MediaSDK.logPath = "/data/data/com.pplive.meetplayer/cache";
		MediaSDK.logOn = false;
		
		long ret = -1;

		try {
			ret = MediaSDK.startP2PEngine(gid, pid, auth);
		} catch (Throwable e) {
			Log.e("Util", e.toString());
		}

		Log.d("Util", "startP2PEngine: " + ret);
		return (ret != -1);// 端口占用&& ret != 9);
	}
}
