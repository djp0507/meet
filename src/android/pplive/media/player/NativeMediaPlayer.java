/**
 * Copyright (C) 2012 PPTV
 *
 */
package android.pplive.media.player;

import java.io.FileDescriptor;
import java.io.IOException;
import java.lang.ref.WeakReference;
import java.util.Map;

import android.content.Context;
import android.graphics.Bitmap;
import android.media.AudioManager;
import android.net.Uri;
import android.os.Message;
import android.os.Parcel;
import android.pplive.media.config.Config;
import android.pplive.media.util.LogUtils;
import android.util.Log;
import android.view.Surface;
import android.view.SurfaceHolder;

abstract class NativeMediaPlayer extends MeetMediaPlayer {
	
	// TV BOX does not need to call this check.
	static boolean checkCompatibility(int checkWhat, Surface surface) {
		boolean ret = false;
		try {
			native_init(false);
			ret = native_checkCompatibility(checkWhat, surface);
		} catch(Exception ex) {
		    LogUtils.error("Exception", ex);
		} 
		
		return ret;
	}
	static int checkSoftwareDecodeLevel() {
		return native_checkSoftwareDecodeLevel();
	}
	static int getCpuArchNumber() {
		return native_getCpuArchNumber();
	}
	
	/**
	 * Do decoding compatibility check.
	 */
	private static native boolean native_checkCompatibility(int checkWhat, Surface surface) throws IllegalStateException;
    
	static native String getBestCodec(String appPath);

	private static native int native_checkSoftwareDecodeLevel();

	private static native int native_getCpuArchNumber();
	
	/**
	 * Native getMediaInfo requires a reference to a MediaInfo instance. It's easier to
	 * create it here in native code.
	 */
	static MediaInfo getMediaInfo(String mediaFilePath) {
		try {
			native_init(false);
		} catch (RuntimeException e) {
            LogUtils.error("RuntimeException", e);
			return null;
        }
		
		MediaInfo info = new MediaInfo();
		
		return native_getMediaInfo(mediaFilePath, info) ? info : null;
	}
	
	static MediaInfo getMediaDetailInfo(String mediaFilePath) {
		try {
			native_init(false);
		} catch (RuntimeException e) {
            LogUtils.error("RuntimeException", e);
			return null;
        }
		
		MediaInfo info = new MediaInfo();
		
		return native_getMediaDetailInfo(mediaFilePath, info) ? info : null;
	}
	
	private static native boolean native_getMediaInfo(String mediaFilePath, MediaInfo info);
	private static native boolean native_getMediaDetailInfo(String mediaFilePath, MediaInfo info);
	
	static Bitmap createVideoThumbnail(String mediaFilePath, int kind) {
		Bitmap bitmap = null;
		
		MediaInfo info = getThumbnail(mediaFilePath);
		if (null != info) {
			int[] colors = info.getThumbnail();
			int width = info.getThumbnailWidth();
			int height = info.getThumbnailHeight();
			Log.d(TAG, "colors.length: " + colors.length);
			Log.d(TAG, "width: " + width + "; height: " + height);
			bitmap = Bitmap.createBitmap(colors, width, height, Bitmap.Config.ARGB_8888);
		}
		
		return bitmap;
	}
	
	private static MediaInfo getThumbnail(String mediaFilePath) {
		try {
			native_init(false);
		} catch (RuntimeException e) {
            LogUtils.error("RuntimeException", e);
			return null;
        }
		
		MediaInfo info = new MediaInfo();
		
		return native_getThumbnail(mediaFilePath, info) ? info : null;
	}
	
	private static native boolean native_getThumbnail(String mediaFilePath, MediaInfo info);
	
	NativeMediaPlayer(boolean isSoftwareDecode) {
		init();
		
		if (!isSoftwareDecode) {
			LogUtils.info("go meetplayer hardware");
		} else {
			LogUtils.info("go meetplayer software");
		}
		
		/*
		 * Native setup requires a weak reference to our object. It's easier to
		 * create it here than in C++.
		 */
		native_setup(new WeakReference<NativeMediaPlayer>(this), isSoftwareDecode);
	}
	
	/**
	 * Sets the data source (file-path or http/rtsp URL) to use.
	 * 
	 * @param path
	 *            the path of the file, or the http/rtsp URL of the stream you
	 *            want to play
	 * @throws IllegalStateException
	 *             if it is called in an invalid state
	 */
	public void setDataSource(String path) throws IOException,
			IllegalArgumentException, IllegalStateException {
	    LogUtils.info(path);
		setDataSource_(path);
	}
	
	private native void setDataSource_(String path) throws IOException,
			IllegalArgumentException, IllegalStateException;


	/**
	 * Sets the SurfaceHolder to use for displaying the video portion of the
	 * media. This call is optional. Not calling it when playing back a video
	 * will result in only the audio track being played.
	 * 
	 * @param sh
	 *            the SurfaceHolder to use for video display
	 */
	@Override
	public void setDisplay(SurfaceHolder sh) {
		super.setDisplay(sh);
		
		if (sh != null) {
			mSurface = sh.getSurface();
		} else {
			mSurface = null;
		}
		_setVideoSurface();
		updateSurfaceScreenOn();
	}

	/**
	 * Prepares the player for playback, synchronously.
	 * 
	 * After setting the datasource and the display surface, you need to either
	 * call prepare() or prepareAsync(). For files, it is OK to call prepare(),
	 * which blocks until MediaPlayer is ready for playback.
	 * 
	 * @throws IllegalStateException
	 *             if it is called in an invalid state
	 */
	@Override
	public native void prepare() throws IOException, IllegalStateException;

	/**
	 * Prepares the player for playback, asynchronously.
	 * 
	 * After setting the datasource and the display surface, you need to either
	 * call prepare() or prepareAsync(). For streams, you should call
	 * prepareAsync(), which returns immediately, rather than blocking until
	 * enough data has been buffered.
	 * 
	 * @throws IllegalStateException
	 *             if it is called in an invalid state
	 */
	@Override
	public native void prepareAsync() throws IllegalStateException;

	/**
	 * Starts or resumes playback. If playback had previously been paused,
	 * playback will continue from where it was paused. If playback had been
	 * stopped, or never started before, playback will start at the beginning.
	 * 
	 * @throws IllegalStateException
	 *             if it is called in an invalid state
	 */
	@Override
	public void start() throws IllegalStateException {
		stayAwake(true);
		_start();
	}
	
	/**
	 * Pauses playback. Call start() to resume.
	 * 
	 * @throws IllegalStateException
	 *             if the internal player engine has not been initialized.
	 */
	@Override
	public void pause() throws IllegalStateException {
		stayAwake(false);
		_pause();
	}
	
	/**
	 * Stops playback after playback has been stopped or paused.
	 * 
	 * @throws IllegalStateException
	 *             if the internal player engine has not been initialized.
	 */
	@Override
	public void stop() throws IllegalStateException {
		stayAwake(false);
		_stop();
	}
	
	/**
	 * Releases resources associated with this MediaPlayer object. It is
	 * considered good practice to call this method when you're done using the
	 * MediaPlayer.
	 */
	@Override
	public void release() {
		stayAwake(false);
		updateSurfaceScreenOn();
		mOnPreparedListener = null;
		mOnBufferingUpdateListener = null;
		mOnCompletionListener = null;
		mOnSeekCompleteListener = null;
		mOnErrorListener = null;
		mOnInfoListener = null;
		mOnVideoSizeChangedListener = null;
		_release();
	}

	//Returns the width of the video.
	@Override
	public native int getVideoWidth();

	//Returns the height of the video.
	@Override
	public native int getVideoHeight();

	//Checks whether the MediaPlayer is playing.
	//return true if currently playing, false otherwise
	@Override
	public native boolean isPlaying();

	//Seeks to specified time position in milliseconds
	@Override
	public native void seekTo(int msec) throws IllegalStateException;

	//return the current position in milliseconds
	@Override
	public native int getCurrentPosition();

	//return the duration in milliseconds
	@Override
	public native int getDuration();
	
	@Override
	public native int flags() throws IllegalStateException;
	
	/**
	 * Constant to retrieve only the new metadata since the last call. // FIXME:
	 * unhide. // FIXME: add link to getMetadata(boolean, boolean) {@hide
	 * }
	 */
	public static final boolean METADATA_UPDATE_ONLY = true;

	/**
	 * Constant to retrieve all the metadata. 
	 * // FIXME: unhide. 
	 * // FIXME: add
	 * link to getMetadata(boolean, boolean) {@hide}
	 */
	public static final boolean METADATA_ALL = false;

	/**
	 * Constant to enable the metadata filter during retrieval. 
	 * // FIXME: unhide. 
	 * // FIXME: add link to getMetadata(boolean, boolean) {@hide}
	 */
	public static final boolean APPLY_METADATA_FILTER = true;

	/**
	 * Constant to disable the metadata filter during retrieval. 
	 * // FIXME: unhide. 
	 * // FIXME: add link to getMetadata(boolean, boolean) {@hide}
	 */
	public static final boolean BYPASS_METADATA_FILTER = false;
	
	private final static String TAG = "DefaultMediaPlayer";

	static {
		System.loadLibrary("meet");
	}

	// Name of the remote interface for the media player. Must be kept
	// in sync with the 2nd parameter of the IMPLEMENT_META_INTERFACE
	// macro invocation in IMediaPlayer.cpp
	private final static String IMEDIA_PLAYER = "android.media.IMediaPlayer";

	private int mNativeContext; // accessed by native methods
	private int mListenerContext; // accessed by native methods
	private Surface mSurface; // accessed by native methods
//	private SurfaceHolder mSurfaceHolder;
//	private PowerManager.WakeLock mWakeLock = null;
//	private boolean mScreenOnWhilePlaying;
//	private boolean mStayAwake;

	private void init() {
		
		native_init(Config.needStartP2P());
	}

	/*
	 * Update the MediaPlayer ISurface. Call after updating mSurface.
	 */
	private native void _setVideoSurface();

	/**
	 * Create a request parcel which can be routed to the native media player
	 * using {@link #invoke(Parcel, Parcel)}. The Parcel returned has the proper
	 * InterfaceToken set. The caller should not overwrite that token, i.e it
	 * can only append data to the Parcel.
	 * 
	 * @return A parcel suitable to hold a request for the native player.
	 *         {@hide}
	 */
	public Parcel newRequest() {
		Parcel parcel = Parcel.obtain();
		parcel.writeInterfaceToken(IMEDIA_PLAYER);
		return parcel;
	}

	/**
	 * Invoke a generic method on the native player using opaque parcels for the
	 * request and reply. Both payloads' format is a convention between the java
	 * caller and the native player. Must be called after setDataSource to make
	 * sure a native player exists.
	 * 
	 * @param request
	 *            Parcel with the data for the extension. The caller must use
	 *            {@link #newRequest()} to get one.
	 * 
	 * @param reply
	 *            Output parcel with the data returned by the native player.
	 * 
	 * @return The status code see utils/Errors.h {@hide}
	 */
	public int invoke(Parcel request, Parcel reply) {
		int retcode = native_invoke(request, reply);
		reply.setDataPosition(0);
		return retcode;
	}

	/**
	 * Convenience method to create a MediaPlayer for a given Uri. On success,
	 * {@link #prepare()} will already have been called and must not be called
	 * again.
	 * <p>
	 * When done with the MediaPlayer, you should call {@link #release()}, to
	 * free the resources. If not released, too many MediaPlayer instances will
	 * result in an exception.
	 * </p>
	 * 
	 * @param context
	 *            the Context to use
	 * @param uri
	 *            the Uri from which to get the datasource
	 * @return a MediaPlayer object, or null if creation failed
	 */
//	public static DefaultMediaPlayer create(Context context, Uri uri, String appPath) {
//		return create(context, uri, appPath, null);
//	}

	/**
	 * Convenience method to create a MediaPlayer for a given Uri. On success,
	 * {@link #prepare()} will already have been called and must not be called
	 * again.
	 * <p>
	 * When done with the MediaPlayer, you should call {@link #release()}, to
	 * free the resources. If not released, too many MediaPlayer instances will
	 * result in an exception.
	 * </p>
	 * 
	 * @param context
	 *            the Context to use
	 * @param uri
	 *            the Uri from which to get the datasource
	 * @param holder
	 *            the SurfaceHolder to use for displaying the video
	 * @return a MediaPlayer object, or null if creation failed
	 */
//	public static DefaultMediaPlayer create(Context context, Uri uri, String appPath,
//			SurfaceHolder holder) {
//
//		try {
//			DefaultMediaPlayer mp = new DefaultMediaPlayer(appPath);
//			mp.setDataSource(context, uri);
//			if (holder != null) {
//				mp.setDisplay(holder);
//			}
//			mp.prepare();
//			return mp;
//		} catch (IOException ex) {
//			Log.e(TAG, "create failed:", ex);
//			// fall through
//		} catch (IllegalArgumentException ex) {
//			Log.e(TAG, "create failed:", ex);
//			// fall through
//		} catch (SecurityException ex) {
//			Log.e(TAG, "create failed:", ex);
//			// fall through
//		}
//
//		return null;
//	}

	/**
	 * Convenience method to create a MediaPlayer for a given resource id. On
	 * success, {@link #prepare()} will already have been called and must not be
	 * called again.
	 * <p>
	 * When done with the MediaPlayer, you should call {@link #release()}, to
	 * free the resources. If not released, too many MediaPlayer instances will
	 * result in an exception.
	 * </p>
	 * 
	 * @param context
	 *            the Context to use
	 * @param resid
	 *            the raw resource id (<var>R.raw.&lt;something></var>) for the
	 *            resource to use as the datasource
	 * @return a MediaPlayer object, or null if creation failed
	 */
//	public static DefaultMediaPlayer create(Context context, String appPath, int resid) {
//		try {
//			AssetFileDescriptor afd = context.getResources().openRawResourceFd(
//					resid);
//			if (afd == null)
//				return null;
//
//			DefaultMediaPlayer mp = new DefaultMediaPlayer(appPath);
//			mp.setDataSource(afd.getFileDescriptor(), afd.getStartOffset(),
//					afd.getLength());
//			afd.close();
//			mp.prepare();
//			return mp;
//		} catch (IOException ex) {
//			Log.d(TAG, "create failed:", ex);
//			// fall through
//		} catch (IllegalArgumentException ex) {
//			Log.d(TAG, "create failed:", ex);
//			// fall through
//		} catch (SecurityException ex) {
//			Log.d(TAG, "create failed:", ex);
//			// fall through
//		}
//		return null;
//	}

	/**
	 * Sets the data source as a content Uri.
	 * 
	 * @param context
	 *            the Context to use when resolving the Uri
	 * @param uri
	 *            the Content URI of the data you want to play
	 * @throws IllegalStateException
	 *             if it is called in an invalid state
	 */
	public void setDataSource(Context context, Uri uri) throws IOException,
			IllegalArgumentException, SecurityException, IllegalStateException {
		setDataSource(context, uri, null);
	}

	/**
	 * Sets the data source as a content Uri.
	 * 
	 * @param context
	 *            the Context to use when resolving the Uri
	 * @param uri
	 *            the Content URI of the data you want to play
	 * @param headers
	 *            the headers to be sent together with the request for the data
	 * @throws IllegalStateException
	 *             if it is called in an invalid state
	 * @hide pending API council
	 */
	public void setDataSource(Context context, Uri uri,
			Map<String, String> headers) throws IOException,
			IllegalArgumentException, SecurityException, IllegalStateException {

		String scheme = uri.getScheme();
		if (scheme == null || scheme.equals("file")) {
			//local file
			setDataSource(uri.getPath());
			return;
		}
		//network path
		setDataSource(uri.toString());
		/*
		AssetFileDescriptor fd = null;
		try {
			ContentResolver resolver = context.getContentResolver();
			fd = resolver.openAssetFileDescriptor(uri, "r");
			if (fd == null) {
				return;
			}
			// Note: using getDeclaredLength so that our behavior is the same
			// as previous versions when the content provider is returning
			// a full file.
			if (fd.getDeclaredLength() < 0) {
				setDataSource(fd.getFileDescriptor());
			} else {
				setDataSource(fd.getFileDescriptor(), fd.getStartOffset(),
						fd.getDeclaredLength());
			}
			return;
		} catch (SecurityException ex) {
		} catch (IOException ex) {
		} finally {
			if (fd != null) {
				fd.close();
			}
		}
		Log.d(TAG, "Couldn't open file on client side, trying server side");
		setDataSource(uri.toString(), headers);
		*/
		return;
	}


	/**
	 * Sets the data source (file-path or http/rtsp URL) to use.
	 * 
	 * @param path
	 *            the path of the file, or the http/rtsp URL of the stream you
	 *            want to play
	 * @param headers
	 *            the headers associated with the http request for the stream
	 *            you want to play
	 * @throws IllegalStateException
	 *             if it is called in an invalid state
	 * @hide pending API council
	 */
	public native void setDataSource(String path, Map<String, String> headers)
			throws IOException, IllegalArgumentException, IllegalStateException;

	/**
	 * Sets the data source (FileDescriptor) to use. It is the caller's
	 * responsibility to close the file descriptor. It is safe to do so as soon
	 * as this call returns.
	 * 
	 * @param fd
	 *            the FileDescriptor for the file you want to play
	 * @throws IllegalStateException
	 *             if it is called in an invalid state
	 */
	public void setDataSource(FileDescriptor fd) throws IOException,
			IllegalArgumentException, IllegalStateException {
		// intentionally less than LONG_MAX
		setDataSource(fd, 0, 0x7ffffffffffffffL);
	}

	/**
	 * Sets the data source (FileDescriptor) to use. The FileDescriptor must be
	 * seekable (N.B. a LocalSocket is not seekable). It is the caller's
	 * responsibility to close the file descriptor. It is safe to do so as soon
	 * as this call returns.
	 * 
	 * @param fd
	 *            the FileDescriptor for the file you want to play
	 * @param offset
	 *            the offset into the file where the data to be played starts,
	 *            in bytes
	 * @param length
	 *            the length in bytes of the data to be played
	 * @throws IllegalStateException
	 *             if it is called in an invalid state
	 */
	public native void setDataSource(FileDescriptor fd, long offset, long length)
			throws IOException, IllegalArgumentException, IllegalStateException;



	private native void _start() throws IllegalStateException;


	private native void _stop() throws IllegalStateException;


	private native void _pause() throws IllegalStateException;

	/**
	 * Set the low-level power management behavior for this MediaPlayer. This
	 * can be used when the MediaPlayer is not playing through a SurfaceHolder
	 * set with {@link #setDisplay(SurfaceHolder)} and thus can use the
	 * high-level {@link #setScreenOnWhilePlaying(boolean)} feature.
	 * 
	 * <p>
	 * This function has the MediaPlayer access the low-level power manager
	 * service to control the device's power usage while playing is occurring.
	 * The parameter is a combination of {@link android.os.PowerManager} wake
	 * flags. Use of this method requires
	 * {@link android.Manifest.permission#WAKE_LOCK} permission. By default, no
	 * attempt is made to keep the device awake during playback.
	 * 
	 * @param context
	 *            the Context to use
	 * @param mode
	 *            the power/wake mode to set
	 * @see android.os.PowerManager
	 */
	/*
	@Override
	public void setWakeMode(Context context, int mode) {
		boolean washeld = false;
		if (mWakeLock != null) {
			if (mWakeLock.isHeld()) {
				washeld = true;
				mWakeLock.release();
			}
			mWakeLock = null;
		}

		PowerManager pm = (PowerManager) context
				.getSystemService(Context.POWER_SERVICE);
		mWakeLock = pm.newWakeLock(mode | PowerManager.ON_AFTER_RELEASE,
				DefaultMediaPlayer.class.getName());
		mWakeLock.setReferenceCounted(false);
		if (washeld) {
			mWakeLock.acquire();
		}
	}
	 */
	/**
	 * Control whether we should use the attached SurfaceHolder to keep the
	 * screen on while video playback is occurring. This is the preferred method
	 * over {@link #setWakeMode} where possible, since it doesn't require that
	 * the application have permission for low-level wake lock access.
	 * 
	 * @param screenOn
	 *            Supply true to keep the screen on, false to allow it to turn
	 *            off.
	 */
	/*
	@Override
	public void setScreenOnWhilePlaying(boolean screenOn) {
		if (mScreenOnWhilePlaying != screenOn) {
			mScreenOnWhilePlaying = screenOn;
			updateSurfaceScreenOn();
		}
	}
	@Override
	protected void stayAwake(boolean awake) {
		if (mWakeLock != null) {
			if (awake && !mWakeLock.isHeld()) {
				mWakeLock.acquire();
			} else if (!awake && mWakeLock.isHeld()) {
				mWakeLock.release();
			}
		}
		mStayAwake = awake;
		updateSurfaceScreenOn();
	}

	private void updateSurfaceScreenOn() {
		if (mSurfaceHolder != null) {
			mSurfaceHolder.setKeepScreenOn(mScreenOnWhilePlaying && mStayAwake);
		}
	}
	 */

	private native void _release();

	/**
	 * Resets the MediaPlayer to its uninitialized state. After calling this
	 * method, you will have to initialize it again by setting the data source
	 * and calling prepare().
	 */
	public void reset() {
		stayAwake(false);
		_reset();
		// make sure none of the listeners get called anymore
		mEventHandler.removeCallbacksAndMessages(null);
	}

	private native void _reset();

	/**
	 * Suspends the MediaPlayer. The only methods that may be called while
	 * suspended are {@link #reset()}, {@link #release()} and {@link #resume()}.
	 * MediaPlayer will release its hardware resources as far as possible and
	 * reasonable. A successfully suspended MediaPlayer will cease sending
	 * events. If suspension is successful, this method returns true, otherwise
	 * false is returned and the player's state is not affected.
	 * 
	 * @hide
	 */
	@Deprecated
	@Override
	public boolean suspend() {
		if (native_suspend_resume(true) < 0) {
			return false;
		}

		stayAwake(false);

		// make sure none of the listeners get called anymore
		mEventHandler.removeCallbacksAndMessages(null);

		return true;
	}

	/**
	 * Resumes the MediaPlayer. Only to be called after a previous (successful)
	 * call to {@link #suspend()}. MediaPlayer will return to a state close to
	 * what it was in before suspension.
	 * 
	 * @hide
	 */
	@Deprecated
	@Override
	public boolean resume() {
		if (native_suspend_resume(false) < 0) {
			return false;
		}

		if (isPlaying()) {
			stayAwake(true);
		}

		return true;
	}

	/**
	 * @hide
	 */
	private native int native_suspend_resume(boolean isSuspend);

	/**
	 * Sets the audio stream type for this MediaPlayer. See {@link AudioManager}
	 * for a list of stream types. Must call this method before prepare() or
	 * prepareAsync() in order for the target stream type to become effective
	 * thereafter.
	 * 
	 * @param streamtype
	 *            the audio stream type
	 * @see android.media.AudioManager
	 */
	public native void setAudioStreamType(int streamtype);

	/**
	 * Sets the player to be looping or non-looping.
	 * 
	 * @param looping
	 *            whether to loop or not
	 */
	public native void setLooping(boolean looping);

	/**
	 * Checks whether the MediaPlayer is looping or non-looping.
	 * 
	 * @return true if the MediaPlayer is currently looping, false otherwise
	 */
	public native boolean isLooping();

	/**
	 * Sets the volume on this player. This API is recommended for balancing the
	 * output of audio streams within an application. Unless you are writing an
	 * application to control user settings, this API should be used in
	 * preference to {@link AudioManager#setStreamVolume(int, int, int)} which
	 * sets the volume of ALL streams of a particular type. Note that the passed
	 * volume values are raw scalars. UI controls should be scaled
	 * logarithmically.
	 * 
	 * @param leftVolume
	 *            left volume scalar
	 * @param rightVolume
	 *            right volume scalar
	 */
	public native void setVolume(float leftVolume, float rightVolume);

	/**
	 * Currently not implemented, returns null.
	 * 
	 * @deprecated
	 * @hide
	 */
	public native Bitmap getFrameAt(int msec) throws IllegalStateException;

	public static final int CAN_SEEK_BACKWARD = 1;
	public static final int CAN_SEEK_FORWARD = 2;
	public static final int CAN_PAUSE = 4;
	public static final int CAN_SEEK = 8;
	
	/**
	 * @param request
	 *            Parcel destinated to the media player. The Interface token
	 *            must be set to the IMediaPlayer one to be routed correctly
	 *            through the system.
	 * @param reply
	 *            [out] Parcel that will contain the reply.
	 * @return The status code.
	 */
	private native final int native_invoke(Parcel request, Parcel reply);

	/**
	 * @param update_only
	 *            If true fetch only the set of metadata that have changed since
	 *            the last invocation of getMetadata. The set is built using the
	 *            unfiltered notifications the native player sent to the
	 *            MediaPlayerService during that period of time. If false, all
	 *            the metadatas are considered.
	 * @param apply_filter
	 *            If true, once the metadata set has been built based on the
	 *            value update_only, the current filter is applied.
	 * @param reply
	 *            [out] On return contains the serialized metadata. Valid only
	 *            if the call was successful.
	 * @return The status code.
	 */
	private native final boolean native_getMetadata(boolean update_only,
			boolean apply_filter, Parcel reply);

	/**
	 * @param request
	 *            Parcel with the 2 serialized lists of allowed metadata types
	 *            followed by the one to be dropped. Each list starts with an
	 *            integer indicating the number of metadata type elements.
	 * @return The status code.
	 */
	private native final int native_setMetadataFilter(Parcel request);

	private static native final void native_init(boolean startP2PEngine);
	
	private native final void native_setup(Object mediaplayer_this, boolean generalPlayer);

	private native final void native_finalize();

	@Override
	protected void finalize() {
		native_finalize();
	}

	/*
	 * Do not change these values without updating their counterparts in
	 * include/media/mediaplayer.h!
	 */
	private static final int MEDIA_NOP = 0; // interface test message
	private static final int MEDIA_PREPARED = 1;
	private static final int MEDIA_PLAYBACK_COMPLETE = 2;
	private static final int MEDIA_BUFFERING_UPDATE = 3;
	private static final int MEDIA_SEEK_COMPLETE = 4;
	private static final int MEDIA_SET_VIDEO_SIZE = 5;
	private static final int MEDIA_ERROR = 100;
	private static final int MEDIA_INFO = 200;

	/**
	 * Called from native code when an interesting event happens. This method
	 * just uses the EventHandler system to post the event back to the main app
	 * thread. We use a weak reference to the original MediaPlayer object so
	 * that the native code is safe from the object disappearing from underneath
	 * it. (This is the cookie passed to native_setup().)
	 */
	private static void postEventFromNative(Object mediaplayer_ref, int what,
			int arg1, int arg2, Object obj) {
		//Log.d(TAG, "Get event from postEventFromNative()");

		NativeMediaPlayer mp = (NativeMediaPlayer) ((WeakReference<?>) mediaplayer_ref).get();
		if (mp == null) {
			return;
		}

		if (mp.mEventHandler != null) {
//			Log.d(TAG, "what: " + what);
			
			Message msg = mp.mEventHandler.obtainMessage(what, arg1, arg2, obj);
			msg.sendToTarget();
		}

	}

	/*
	 * Do not change these values without updating their counterparts in
	 * include/media/mediaplayer.h!
	 */
	/**
	 * Unspecified media player error.
	 * 
	 * @see android.media.MediaPlayer.OnErrorListener
	 */
	public static final int MEDIA_ERROR_UNKNOWN = 1;

	/**
	 * Media server died. In this case, the application must release the
	 * MediaPlayer object and instantiate a new one.
	 * 
	 * @see android.media.MediaPlayer.OnErrorListener
	 */
	public static final int MEDIA_ERROR_SERVER_DIED = 100;

	/**
	 * The video is streamed and its container is not valid for progressive
	 * playback i.e the video's index (e.g moov atom) is not at the start of the
	 * file.
	 * 
	 * @see android.media.MediaPlayer.OnErrorListener
	 */
	public static final int MEDIA_ERROR_NOT_VALID_FOR_PROGRESSIVE_PLAYBACK = 200;

	/*
	 * Do not change these values without updating their counterparts in
	 * include/media/mediaplayer.h!
	 */
	/**
	 * Unspecified media player info.
	 * 
	 * @see android.media.MediaPlayer.OnInfoListener
	 */
	public static final int MEDIA_INFO_UNKNOWN = 1;

	/**
	 * The video is too complex for the decoder: it can't decode frames fast
	 * enough. Possibly only the audio plays fine at this stage.
	 * 
	 * @see android.media.MediaPlayer.OnInfoListener
	 */
	public static final int MEDIA_INFO_VIDEO_TRACK_LAGGING = 700;

	/**
	 * Bad interleaving means that a media has been improperly interleaved or
	 * not interleaved at all, e.g has all the video samples first then all the
	 * audio ones. Video is playing but a lot of disk seeks may be happening.
	 * 
	 * @see android.media.MediaPlayer.OnInfoListener
	 */
	public static final int MEDIA_INFO_BAD_INTERLEAVING = 800;

	/**
	 * The media cannot be seeked (e.g live stream)
	 * 
	 * @see android.media.MediaPlayer.OnInfoListener
	 */
	public static final int MEDIA_INFO_NOT_SEEKABLE = 801;

	/**
	 * A new set of metadata is available.
	 * 
	 * @see android.media.MediaPlayer.OnInfoListener
	 */
	public static final int MEDIA_INFO_METADATA_UPDATE = 802;

	/**
	 * @hide
	 */
	public native static int snoop(short[] outData, int kind);
}
