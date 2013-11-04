package android.pplive.media;

import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

import android.content.Context;
import android.database.Cursor;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.PixelFormat;
import android.media.ThumbnailUtils;
import android.net.Uri;
import android.os.Build;
import android.os.Environment;
import android.pplive.media.config.Config;
import android.pplive.media.player.MediaInfo;
import android.pplive.media.player.MeetPlayerHelper;
import android.pplive.media.util.DiskLruCache;
import android.pplive.media.util.LogUtils;
import android.pplive.media.util.UrlUtil;
import android.provider.MediaStore;
import android.view.Surface;
import android.view.SurfaceHolder;

public final class MeetSDK {

	@SuppressWarnings("unused")
	private static final String TAG = "ppmedia/MeetSDK";

	public static String AppRootDir = null;
	public static String PPBoxLibName = null;
    
    public static int FullScreenWidth = 0;
    public static int FullScreenHeight = 0;

	public static final int COMPATIBILITY_HARDWARE_DECODE = 1;
	public static final int COMPATIBILITY_SOFTWARE_DECODE = 2;

	public static final int LEVEL_HARDWARE = 1;
	public static final int LEVEL_SOFTWARE_LIUCHANG = 2;
	public static final int LEVEL_SOFTWARE_GAOQING = 3;
	public static final int LEVEL_SOFTWARE_CHAOQING = 4;
	public static final int LEVEL_SOFTWARE_LANGUANG = 5;
	
	private static int status = 0;

	public static void init(Context ctx) {
		AppRootDir = "/data/data/" + ctx.getPackageName() + "/";
	}
	
//	public static MediaPlayer createMediaPlayer() {
//		return new MediaPlayer();
//	}

	public static boolean checkCompatibility(Surface surface) {
		return checkCompatibility(COMPATIBILITY_HARDWARE_DECODE, surface);
	}

	public static boolean checkCompatibility(int checkWhat, Surface surface) {
		if (AppRootDir == null) {
		    LogUtils.error("MeetSDK.AppRootDir is null.");
			throw new IllegalArgumentException("MeetSDK.AppRootDir is null.");
		}

		boolean ret = false;

		try {
			if (checkWhat == COMPATIBILITY_HARDWARE_DECODE) {
				if (Build.VERSION.SDK_INT >= 16) {

					ret = MeetPlayerHelper.checkCompatibility(surface);
				}
				else if (Build.VERSION.SDK_INT >= 14) {
					ret = MeetPlayerHelper.checkCompatibility(checkWhat, surface);
				}
				else {

					//Android 2.2/2.3
					ret = false;
				}
			} else {
				ret = MeetPlayerHelper.checkCompatibility(checkWhat, surface);
			}
		} catch (LinkageError e) {
			e.printStackTrace();
		}
		
		return ret;
	}

	public static int checkSoftwareDecodeLevel() {
		if (AppRootDir == null) {
			LogUtils.error("MeetSDK.AppRootDir is null.");
			throw new IllegalArgumentException("MeetSDK.AppRootDir is null.");
		}

		int decodeLevel = LEVEL_HARDWARE;
		try {
			decodeLevel = MeetPlayerHelper.checkSoftwareDecodeLevel();
		} catch (LinkageError e) {
			LogUtils.error("LinkageError", e);
		}
		
		return decodeLevel;
	}
	

	public static int getCpuArchNumber() {
		if (AppRootDir == null) {
		    LogUtils.error("MeetSDK.AppRootDir is null.");
			throw new IllegalArgumentException("MeetSDK.AppRootDir is null.");
		}

		int archNum = 0;
		try {
			archNum = MeetPlayerHelper.getCpuArchNumber();
		} catch (LinkageError e) {
			LogUtils.error("LinkageError", e);
		}
		return archNum;
	}

	public static String getVersion() {
		return Config.getVersion();
	}

	public static String getBestCodec(String appPath) {
		String codec = null;
		try {
			codec = MeetPlayerHelper.getBestCodec(appPath);
		} catch (LinkageError e) {
			LogUtils.error("LinkageError", e);
		}
		
		return codec;
	}
	
	public static MediaInfo getMediaInfo(String filePath) {
		return getMediaInfo(new File(filePath));
	}
	
	public static MediaInfo getMediaInfo(File mediaFile) {
		MediaInfo info = null;

		try {
			if (mediaFile != null && mediaFile.exists()) {
				info = MeetPlayerHelper.getMediaInfo(mediaFile.getAbsolutePath());
			}
		} catch (LinkageError e) {
            LogUtils.error("LinkageError", e);
        }

		return info;
	}

	public static MediaInfo getMediaDetailInfo(File mediaFile) {
		MediaInfo info = null;

		try {
			if (mediaFile != null && mediaFile.exists()) {
				info = MeetPlayerHelper.getMediaDetailInfo(mediaFile.getAbsolutePath());
			}
		} catch (LinkageError e) {
			LogUtils.error("LinkageError", e);
		}

		return info;
	}

	public static String Uri2String(Context ctx, Uri uri) {

		if (null == ctx || null == uri) {
			return null;
		}

		String schema = uri.getScheme();
		String path = null;

		if ("content".equalsIgnoreCase(schema)) {
			String[] proj = { MediaStore.Video.Media.DATA };
			Cursor cursor = ctx.getContentResolver().query(uri, proj, null, null, null);
			if (cursor != null && cursor.moveToFirst()) {
				int column_index = cursor.getColumnIndexOrThrow(MediaStore.Video.Media.DATA);
				path = cursor.getString(column_index);
				cursor.close();
			}
		} else if ("file".equalsIgnoreCase(schema)) {
			path = uri.getPath();
		} else {
			path = uri.toString();
		}
		return path;
	}

	public static boolean isOMXSurface(String url) {

		if (url == null) {
			return false;
		}

		boolean isOMXSurface = false;

		if (url.startsWith("/")) {
			try {
				if(("f16ref".equalsIgnoreCase(android.os.Build.BOARD) ||
					"AML8726-M3".equalsIgnoreCase(android.os.Build.BOARD) ||
					"ppbox".equalsIgnoreCase(android.os.Build.BOARD) ) &&
					(url.endsWith(".mp4") ||
					url.endsWith(".mkv") ||
					url.endsWith(".rmvb") ||
					url.endsWith(".rm") ||
					url.endsWith(".flv") ||
					url.endsWith(".3gp") ||
					url.endsWith(".asf") ||
					url.endsWith(".mpg") ||
					url.endsWith(".ts") ||
					url.endsWith(".m2ts") ||
					url.endsWith(".mov"))) {

					isOMXSurface = true;
				} else if (MeetPlayerHelper.checkSoftwareDecodeLevel() == LEVEL_HARDWARE) {

					isOMXSurface = true;
				} else if (url.endsWith(".mp4") || url.endsWith(".3gp")) {

					MediaInfo info = MeetSDK.getMediaDetailInfo(new File(url));

					if (null != info) {
						LogUtils.info("info.Width:" + info.getWidth());
						LogUtils.info("info.Height:" + info.getHeight());
						LogUtils.info("info.FormatName:" + info.getFormatName());
						LogUtils.info("info.AudioName:" + info.getAudioName());
						LogUtils.info("info.VideoName:" + info.getVideoName());

						isOMXSurface = "aac".equals(info.getAudioName()) && "h264".equals(info.getVideoName());
					}
				}
			} catch (LinkageError e) {
	            LogUtils.error("LinkageError", e);
        	}
			
		} else {
			isOMXSurface = UrlUtil.isPPTVPlayUrl(url);
		}

		return isOMXSurface;
	}

	public static boolean isOMXSurface(Context ctx, Uri uri) {
		if (null == ctx || null == uri) {
			return false;
		}

		String path = Uri2String(ctx, uri);

		return isOMXSurface(path);
	}

	public static boolean setSurfaceType(Context ctx, SurfaceHolder holder, Uri uri) {
		if (holder == null || uri == null) {

			return false;
		}

		String schema = uri.getScheme();
		String path = null;

		if ("content".equalsIgnoreCase(schema)) {
			String[] proj = { MediaStore.Video.Media.DATA };
			Cursor cursor = ctx.getContentResolver().query(uri, proj, null, null, null);
			
			if (cursor != null && cursor.moveToFirst()) {
				int column_index = cursor.getColumnIndexOrThrow(MediaStore.Video.Media.DATA);
				path = cursor.getString(column_index);
				cursor.close();
			}
			
		} else if ("file".equalsIgnoreCase(schema)) {
			path = uri.getPath();
		} else {
			path = uri.toString();
		}

		// Log.d(TAG, "url: " + url);
		boolean isOMXSurface = isOMXSurface(path);
		setSurfaceType(holder, isOMXSurface);
		return isOMXSurface;
	}

	public interface SurfaceTypeDecider {
		public boolean isOMXSurface();
	}

	public static void setSurfaceType(SurfaceHolder holder, SurfaceTypeDecider decider) {
		setSurfaceType(holder, decider.isOMXSurface());
	}

	@SuppressWarnings("deprecation")
	public static void setSurfaceType(SurfaceHolder holder, boolean isOMXSurface) {
		if (isOMXSurface) {
			// Log.d(TAG, "surface push buffer");
			holder.setType(SurfaceHolder.SURFACE_TYPE_PUSH_BUFFERS);
		} else {
			// Log.d(TAG, "surface normal");
			holder.setType(SurfaceHolder.SURFACE_TYPE_NORMAL);
			holder.setFormat(PixelFormat.RGBX_8888);
			if(FullScreenWidth > 0 && FullScreenHeight > 0)
			{
			    holder.setFixedSize(FullScreenWidth, FullScreenHeight);
			}
		}
	}

	/**
	 * @param filePath
	 *            the path of video file
	 * @param kind
	 *            could be MINI_KIND or MICRO_KIND
	 * 
	 * @return Bitmap, or null on failures
	 */
	public synchronized static Bitmap createVideoThumbnail(String filePath, int kind) {
		LogUtils.info("createVideoThumbnail: " + filePath);
		
		Bitmap bitmap = null;

		try {
			bitmap = getThumbnailFromDiskCache(UrlUtil.encode(filePath));
			if (bitmap != null) {
				
				return bitmap;
			} else {
				bitmap = Build.VERSION.SDK_INT >= 8 ? 
						ThumbnailUtils.createVideoThumbnail(filePath, MediaStore.Video.Thumbnails.MICRO_KIND) : null;
				
				bitmap = bitmap == null ?
						MeetPlayerHelper.createVideoThumbnail(filePath, kind) : bitmap;
			}
			
			if (bitmap != null) {
				addThumbnailToDiskCache(UrlUtil.encode(filePath), bitmap);
			}
		} catch (LinkageError e) {
			LogUtils.error("LinkageError", e);
		}

		return bitmap;
	}
	
	private static File sCacheDir = new File(Environment.getExternalStorageDirectory(), "pptv" + File.separator + ".thumbnails");
	private static DiskLruCache sDiskCache = null;

	private synchronized static DiskLruCache getThumbnailDiskLruCache() {

		try {
			if (sDiskCache == null || !sCacheDir.exists()) {
				sDiskCache = DiskLruCache.open(sCacheDir, 1 /* appVersion */, 1 /* valueCount */, 4 * 1024 * 1024 /* maxSize */);
			} 
		} catch (IOException e) {
            LogUtils.error("IOException", e);
		} finally {
			
		}
		
		return sDiskCache;
	}

	private static void addThumbnailToDiskCache(String key, Bitmap bitmap) {
		DiskLruCache cache = getThumbnailDiskLruCache();

		try {
			if (cache != null) {
			    LogUtils.debug("addThumbnailToDiskCache");
				
				DiskLruCache.Editor editor = cache.edit(key);
				
				if (editor != null) {
					OutputStream os = editor.newOutputStream(0);
					editor.commit();
					
					boolean ret = bitmap.compress(Bitmap.CompressFormat.PNG, 100, os);
					LogUtils.debug("bitmap compress: " + ret);
				}
				
			}

		} catch (IOException e) {
            LogUtils.error("IOException", e);
		} finally {
			
		}
	}

	private static Bitmap getThumbnailFromDiskCache(String key) {
		Bitmap bitmap = null;
		DiskLruCache cache = getThumbnailDiskLruCache();

		try {
		    LogUtils.debug("getThumbnailFromDiskCache");
			if (cache != null && cache.get(key) != null) {
				DiskLruCache.Snapshot snapshot = cache.get(key);
				InputStream is = snapshot.getInputStream(0);
				bitmap = BitmapFactory.decodeStream(is);
				
				snapshot.close();
			}
		} catch (IOException e) {
            LogUtils.error("IOException", e);
		} finally {
			if (bitmap != null) {
				LogUtils.debug("bitmap is not null.");
			} else {
			    LogUtils.debug("bitmap is null.");
			}
		}

		return bitmap;
	}
	
    public static boolean setLogPath(String logPath)
    {
        return LogUtils.init(logPath);
    }

    public static void makePlayerlog()
    {
        LogUtils.makePlayerlog();
    }

    public static void setPlayerStatus(int code)
    {
        status = code;
    }

    public static int getPlayerStatus()
    {
        return status;
    }
	private MeetSDK() {}
}
