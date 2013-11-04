package android.pplive.media.scan;

import java.io.File;
import java.io.FileFilter;
import java.util.Collection;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Iterator;
import java.util.LinkedHashSet;
import java.util.Map;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import android.app.Service;
import android.content.ContentValues;
import android.content.Context;
import android.content.Intent;
import android.database.Cursor;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.os.IBinder;
import android.os.Looper;
import android.os.Message;
import android.os.PowerManager;
import android.os.Process;
import android.pplive.media.MeetSDK;
import android.pplive.media.player.MediaInfo;
import android.pplive.media.provider.MediaMetadata;
import android.pplive.media.util.CursorUtil;
import android.pplive.media.util.IoUtil;
import android.pplive.media.util.LogUtils;
import android.provider.MediaStore;

public class MediaScannerService extends Service implements Runnable {

	private static final String TAG = "ppmedia/MediaScannerService";

	public static final String ACTION_MEDIA_MOUNTED = "com.pplive.action.MEDIA_MOUNTED";
	public static final String ACTION_MEDIA_SCANNER_SCAN_FILE = "com.pplive.action.MEDIA_SCANNER_SCAN_FILE";
	public static final String ACTION_MEDIA_SCANNER_STARTED = "com.pplive.action.MEDIA_SCANNER_STARTED";
	public static final String ACTION_MEDIA_SCANNER_FINISHED = "com.pplive.action.MEDIA_SCANNER_FINISHED";
	
	public static final String INTENT_KEY_SCAN_PATH = "android.ppmedia.service.MediaScannerService.INTENT_KEY_SCAN_PATH";
	public static final String INTENT_KEY_RECURSIVELY = "android.ppmedia.service.MediaScannerService.INTENT_KEY_RECURSIVELY";

	private static final Map<String, String> sMimeTypeMap;
	private static final Pattern sRegMimeType;

	static {
		final String extensions[] = { "264", "3g2", "3gp", "3gp2", "3gpp", "3gpp2", "3p2", "amv", "asf", "avi", "ddat",
				"dir", "divx", "dlx", "dv", "dv4", "dvr", "dvr-ms", "dvx", "dxr", "evo", "f4p", "f4v", "flv", "gvi",
				"hdmov", "ivf", "ivr", "k3g", "m1v", "m21", "m2t", "m2ts", "m2v", "m3u", "m3u8", "m4e", "m4v", "mj2",
				"mjp", "mjpg", "mkv", "mmv", "mnv", "mod", "moov", "mov", "movie", "mp21", "mp2v", "mp4", "mp4v",
				"mpc", "mpe", "mpeg", "mpeg4", "mpg", "mpg2", "mpv", "mpv2", "mts", "mtv", "mve", "mxf", "nsv", "nuv",
				"ogg","ogm", "ogv", "ogx", "pgi", "pva", "qt", "qtm", "r3d", "rm", "rmvb", "roq", "rv", "svi", "trp", "ts",
				"vc1", "vcr", "vfw", "vid", "vivo", "vob", "vp3", "vp6", "vp7", "vro", "webm", "wm", "wmv", "wtv",
				"xvid", "yuv" };

		sMimeTypeMap = new HashMap<String, String>();
		StringBuilder sb = new StringBuilder();
		sb.append("^(.*)[.](");
		for (int index = 0; index < extensions.length; ++index) {
			addMimeType(extensions[index]);
			
			sb.append(String.format(index == 0 ? "%s" : "|%s", extensions[index]));
		}
		sb.append(")$");
		sRegMimeType = Pattern.compile(sb.toString(), Pattern.CASE_INSENSITIVE);
	}

	private static void addMimeType(String extension) {
		sMimeTypeMap.put(extension, "video/" + extension);
	}

	private static String getMimeType(String filename) {
		if (filename == null) {
			return "video/unknown";
		}

		Matcher matcher = sRegMimeType.matcher(filename);
		return matcher.find() ? sMimeTypeMap.get(matcher.group(2).toLowerCase()) : "video/unknown";
	}

	private volatile Looper mServiceLooper;
	private volatile Handler mServiceHandler;
	private PowerManager.WakeLock mWakeLock;

	public MediaScannerService() {
	}

	@Override
	public IBinder onBind(Intent intent) {
		return null;
	}

	@Override
	public void onCreate() {
		initMeetSDK();

		PowerManager pm = (PowerManager) getSystemService(Context.POWER_SERVICE);
		mWakeLock = pm.newWakeLock(PowerManager.PARTIAL_WAKE_LOCK, TAG);

		Thread t = new Thread(this, TAG);
		t.start();
	}
	
	private void initMeetSDK() {
		MeetSDK.AppRootDir = "/data/data/" + getPackageName() + "/";
		MeetSDK.PPBoxLibName = "";
	}

	@Override
	public int onStartCommand(Intent intent, int flags, int startId) {
	    
		while (mServiceHandler == null) {
			synchronized (this) {
				try {
					wait(100);
				} catch (InterruptedException e) {
				}
			}
		}

		if (!isScanning()) {
			Message msg = mServiceHandler.obtainMessage();
			msg.arg1 = startId;
			if (intent != null){
			    msg.setData(intent.getExtras());
			}
			msg.setData(intent.getExtras());
			mServiceHandler.sendMessage(msg);
		} else {
		    LogUtils.warn("Is Already Scanning");
		}

		return START_REDELIVER_INTENT;
	}

	@Override
	public void onDestroy() {
		while (mServiceLooper == null) {
			synchronized (this) {
				try {
					wait(100);
				} catch (InterruptedException e) {
		            LogUtils.warn("InterruptedException");
				}
			}
		}
	}

	private static boolean sIsScanning = false;

	private static boolean isScanning() {
		return sIsScanning;
	}

	private static void setScanning(boolean value) {
		sIsScanning = value;
	}

	@Override
	public void run() {
		Process.setThreadPriority(Process.THREAD_PRIORITY_BACKGROUND + Process.THREAD_PRIORITY_LESS_FAVORABLE);
		Looper.prepare();

		mServiceLooper = Looper.myLooper();
		mServiceHandler = new Handler() {
			@Override
			public void handleMessage(Message msg) {
				try {
				    Bundle data = msg.getData();
				    boolean recursively = true;
				    File filepath = null;
				    if (data != null){
				        recursively = data.getBoolean(INTENT_KEY_RECURSIVELY, true);
				        filepath = (File) data.getSerializable(INTENT_KEY_SCAN_PATH);
				    }
				    if (filepath == null){
				        filepath = Environment.getExternalStorageDirectory();
				    }
				    scan(filepath, recursively);
				} catch (Exception e) {
					LogUtils.error("Exception in handleMessage", e);
				}

				stopSelf(msg.arg1);
			}
		};

		Looper.loop();
	}

	private void scan(File filepath, boolean recursively) {
		setScanning(true);
		try {
			scanImpl(filepath, recursively);
		} finally {
			setScanning(false);
		}
	}

	/**
	 * 1. 删除已不存在的本地视频记录. 
	 * 2. 合并系统媒体库与自有媒体库. 
	 * 3. 扫描SD卡上的本地视频, 并过滤已知的本地视频. 
	 * 4. 更新本地视频列表数据库, 并过滤格式不支持的本地视频. 
	 */
	private void scanImpl(File filepath, boolean recursively) {
	    LogUtils.debug("Before Scan Media Files");

		mWakeLock.acquire();

		sendBroadcast(new Intent(ACTION_MEDIA_SCANNER_STARTED));
		long start = System.currentTimeMillis();
		
		// 1. 删除已不存在的本地视频记录. 
		Cursor cursor = getContentResolver().query(MediaMetadata.Video.CONTENT_URI, null, null, null, null);
		if (cursor != null) {
			StringBuilder where = new StringBuilder();
			for (cursor.moveToFirst(); !cursor.isAfterLast(); cursor.moveToNext()) {
				String filePath = CursorUtil.getString(cursor, MediaMetadata.Video.COLUMN_DATA);
				
				if (!IoUtil.isAccessible(new File(filePath))) {
					where.append(String.format(where.length() > 0 ? ", '%s'" : "'%s'", filePath));
				}
			}
			
			cursor.close();
			
			if (where.length() > 0) {
				where.insert(0, " " + MediaMetadata.Video.COLUMN_DATA + " IN (");
				where.append(")");
				
				getContentResolver().delete(MediaMetadata.Video.CONTENT_URI, where.toString(), null);
			}
		}
		LogUtils.debug("Step 1: " + (System.currentTimeMillis() - start) + "(ms)");
		
		// 2. 合并系统媒体库与自有媒体库.
		FileFilter filter = new FileFilterImpl();
		
		Collection<String> videoFromPPLive = new HashSet<String>();
		cursor = getContentResolver().query(MediaMetadata.Video.CONTENT_URI, null, null, null, null);
		if (cursor != null) {
			for (cursor.moveToFirst(); !cursor.isAfterLast(); cursor.moveToNext()) {
				String filePath = CursorUtil.getString(cursor, MediaMetadata.Video.COLUMN_DATA);
				videoFromPPLive.add(filePath);
			}

			cursor.close();
		}
		
		cursor = getContentResolver().query(MediaStore.Video.Media.EXTERNAL_CONTENT_URI, null, null, null, null);
		if (cursor != null) {
			for (cursor.moveToFirst(); !cursor.isAfterLast(); cursor.moveToNext()) {
				String filePath = CursorUtil.getString(cursor, MediaStore.Video.Media.DATA);
				long duration = CursorUtil.getLong(cursor, MediaStore.Video.Media.DURATION);
				long size = CursorUtil.getLong(cursor, MediaStore.Video.Media.SIZE);
				LogUtils.info(filePath);
				MediaInfo info = new MediaInfo(filePath, duration, size);
				if (!videoFromPPLive.contains(info.getPath()) && filter.accept(info.getData())) {
					save(info);
				}
			}

			cursor.close();
		}
		LogUtils.debug("Step 2: " + (System.currentTimeMillis() - start) + "(ms)");

		// 3. 扫描SD卡上的本地视频, 并过滤已知的本地视频.
		final Collection<String> videos = new LinkedHashSet<String>();
		Scanner scanner = Scanner.getInstance();
		scanner.setOnScannedListener(new OnScannedListener<File>() {
			
			@Override
			public void onScanned(File file) {
				if (file != null) {
					videos.add(file.getAbsolutePath());
				}
			}
		});
		
		scanner.scan(filepath, filter, recursively);

		cursor = getContentResolver().query(MediaMetadata.Video.CONTENT_URI, null, null, null, null);
		if (cursor != null) {
			for (cursor.moveToFirst(); !cursor.isAfterLast(); cursor.moveToNext()) {
				String filePath = CursorUtil.getString(cursor, MediaMetadata.Video.COLUMN_DATA);
				// Log.d(TAG, "filePath: " + cursor.getString(dataId));
				videos.remove(filePath); // filter exists videos.
			}

			cursor.close();
		}
		LogUtils.debug("Step 3: " + (System.currentTimeMillis() - start) + "(ms)");


		// 4. 更新本地视频列表数据库, 并过滤格式不支持的本地视频. 
		for (Iterator<String> iterator = videos.iterator(); iterator.hasNext();) {
			String filePath = iterator.next();
			// Log.d(TAG, "filePath: " + f.getAbsolutePath());
			MediaInfo info = MeetSDK.getMediaInfo(filePath);
			if (info != null) {
				save(info);
			}
		}
		LogUtils.debug("Step 4: " + (System.currentTimeMillis() - start) + "(ms)");

		LogUtils.debug("After Scan Media Files");

		sendBroadcast(new Intent(ACTION_MEDIA_SCANNER_FINISHED));
		mWakeLock.release();
	}

	private void save(MediaInfo info) {
		ContentValues values = new ContentValues();

		values.put(MediaMetadata.Video.COLUMN_TITLE, info.getTitle());
		values.put(MediaMetadata.Video.COLUMN_DATA, info.getPath());
		values.put(MediaMetadata.Video.COLUMN_DURATION, info.getDuration());
		values.put(MediaMetadata.Video.COLUMN_SIZE, info.getSize());
		values.put(MediaMetadata.Video.COLUMN_DATE_ADDED, System.currentTimeMillis());
		values.put(MediaMetadata.Video.COLUMN_DATE_MODIFIED, info.lastModified());
		values.put(MediaMetadata.Video.COLUMN_MIME_TYPE, getMimeType(info.getPath()));

		getContentResolver().insert(MediaMetadata.Video.CONTENT_URI, values);
	}
	
	static class FileFilterImpl implements FileFilter {

		/* 
		 * @see java.io.FileFilter#accept(java.io.File)
		 */
		@Override
		public boolean accept(File file) {
			if (!IoUtil.isAccessible(file)) {
				return false;
			}
			
			String fileName = file.getName();
			if (fileName.startsWith(".") && !fileName.equals(".pps")) {
				return false;
			}
			
			return file.isDirectory() || (file.isFile() && sRegMimeType.matcher(fileName).find());
		}
		
	}
}
