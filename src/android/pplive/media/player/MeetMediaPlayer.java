/**
 * Copyright (C) 2012 PPTV
 * 
 */
package android.pplive.media.player;

import java.io.IOException;

import android.content.Context;
import android.media.AudioManager;
import android.net.Uri;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.os.PowerManager;
import android.pplive.media.player.MeetVideoView.OnBufferingUpdateListener;
import android.pplive.media.player.MeetVideoView.OnCompletionListener;
import android.pplive.media.player.MeetVideoView.OnErrorListener;
import android.pplive.media.player.MeetVideoView.OnInfoListener;
import android.pplive.media.player.MeetVideoView.OnPreparedListener;
import android.pplive.media.player.MeetVideoView.OnSeekCompleteListener;
import android.pplive.media.player.MeetVideoView.OnVideoSizeChangedListener;
import android.pplive.media.util.LogUtils;
import android.view.SurfaceHolder;

abstract class MeetMediaPlayer implements MediaPlayerInterface {
	
	@SuppressWarnings("unused")
	private final static String TAG = "ppmedia/MeetMediaPlayer";
	
	protected final static long TIMEOUT = 5000L /* microseconds */;
	
	protected MeetMediaPlayer() {
		Looper looper;
		
		if ((looper = Looper.myLooper()) != null) {
			mEventHandler = new EventHandler(looper);
		} else if ((looper = Looper.getMainLooper()) != null) {
			mEventHandler = new EventHandler(looper);
		} else {
			mEventHandler = null;
		}
	}
	
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
	public abstract void setDataSource(Context ctx, Uri uri) throws IOException,
		IllegalArgumentException, SecurityException, IllegalStateException;	
	/**
	 * Sets the data source (file-path or http/rtsp URL) to use.
	 * 
	 * @param path
	 *            the path of the file, or the http/rtsp URL of the stream you
	 *            want to play
	 * @throws IllegalStateException
	 *             if it is called in an invalid state
	 */
	public abstract void setDataSource(String uri) throws IOException, IllegalArgumentException, IllegalStateException;
	
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
		mSurfaceHolder = sh;
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
	public abstract void prepare() throws IOException, IllegalStateException;
	
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
	public abstract void prepareAsync() throws IllegalStateException;
	
	/**
	 * Starts or resumes playback. If playback had previously been paused,
	 * playback will continue from where it was paused. If playback had been
	 * stopped, or never started before, playback will start at the beginning.
	 * 
	 * @throws IllegalStateException
	 *             if it is called in an invalid state
	 */
	public abstract void start() throws IllegalStateException;
	
	/**
	 * Stops playback after playback has been stopped or paused.
	 * 
	 * @throws IllegalStateException
	 *             if the internal player engine has not been initialized.
	 */
	public abstract void stop();
	
	/**
	 * Pauses playback. Call start() to resume.
	 * 
	 * @throws IllegalStateException
	 *             if the internal player engine has not been initialized.
	 */
	@Override
	public abstract void pause();
	
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
	public abstract boolean suspend();
	
	/**
	 * Resumes the MediaPlayer. Only to be called after a previous (successful)
	 * call to {@link #suspend()}. MediaPlayer will return to a state close to
	 * what it was in before suspension.
	 * 
	 * @hide
	 */
	@Deprecated
	public abstract boolean resume();
	
	/**
	 *  Seeks to specified time position in milliseconds
	 */
	public abstract void seekTo(int seekingTimeUs) throws IllegalStateException;
	
	/**
	 * Releases resources associated with this MediaPlayer object. It is
	 * considered good practice to call this method when you're done using the
	 * MediaPlayer.
	 */
	@Override
	public abstract void release();
	
	private PowerManager.WakeLock mWakeLock = null;
	private SurfaceHolder mSurfaceHolder = null;
	private boolean mScreenOnWhilePlaying = false;
	private boolean mStayAwake = false;
	
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
				MediaPlayer.class.getName());
		mWakeLock.setReferenceCounted(false);
		if (washeld) {
			mWakeLock.acquire();
		}
	}
	
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
	public void setScreenOnWhilePlaying(boolean screenOn) {
		if (mScreenOnWhilePlaying != screenOn) {
			mScreenOnWhilePlaying = screenOn;
			updateSurfaceScreenOn();
		}
	}
	
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

	protected void updateSurfaceScreenOn() {
		if (mSurfaceHolder != null) {
			mSurfaceHolder.setKeepScreenOn(mScreenOnWhilePlaying && mStayAwake);
		}
	}
	
	/**
	 * Returns the width of the video.
	 */
	@Override
	public abstract int getVideoWidth();
	
	/**
	 * Returns the height of the video.
	 */
	@Override
	public abstract int getVideoHeight();
	
	/**
	 * Checks whether the MediaPlayer is playing.
	 * Returns true if currently playing, false otherwise.
	 */
	public boolean isPlaying() {
		return (getState() == PlayState.STARTED);
	}
	
	/**
	 * Returns the current position in milliseconds
	 */
	@Override
	public abstract int getCurrentPosition();
	
	/**
	 * Returns the duration in milliseconds
	 */
	@Override
	public abstract int getDuration();
	
	@Override
	public abstract int flags() throws IllegalStateException;
	
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
	public void setAudioStreamType(int streamtype) {
		
	}
	
	/**
	 * Resets the MediaPlayer to its uninitialized state. After calling this
	 * method, you will have to initialize it again by setting the data source
	 * and calling prepare().
	 */
	@Override
	public abstract void reset();
	
	protected enum PlayState {
		IDLE, INITIALIZED, PREPARING, PREPARED, STARTED, STOPPED, PAUSED, PLAYBACK_COMPLETED, END, ERROR
	};
	
	private PlayState mState = PlayState.IDLE;
	
	protected /* synchronized */ void setState(PlayState state) {
//		Log.d(TAG, "State: " + state);
		mState = state;
	}
	
	protected /* synchronized */ PlayState getState() {
		return mState;
	}
	
	protected final static int MEDIA_NOP = 0; // interface test message
	protected final static int MEDIA_PREPARED = 1;
	protected final static int MEDIA_PLAYBACK_COMPLETE = 2;
	protected final static int MEDIA_BUFFERING_UPDATE = 3;
	protected final static int MEDIA_SEEK_COMPLETE = 4;
	protected final static int MEDIA_SET_VIDEO_SIZE = 5;
	
	protected final static int MEDIA_ERROR = 100;
	protected final static int MEDIA_INFO = 200;
	
	protected EventHandler mEventHandler;
	
	protected class EventHandler extends Handler {
		
		public EventHandler(Looper looper) {
			super(looper);
		}
		
		@Override
		public void handleMessage(Message msg) {
			
			switch (msg.what) {
				case MEDIA_PREPARED: {
//					Log.d(TAG, "MEDIA_PREPARED");
					setState(PlayState.PREPARED);
					if (mOnPreparedListener != null) {
						mOnPreparedListener.onPrepared();
					}
					return;
				}
				case MEDIA_PLAYBACK_COMPLETE: {
//					Log.d(TAG, "MEDIA_PLAYBACK_COMPLETE");
//					setState(PlayState.PLAYBACK_COMPLETED);
					if (mOnCompletionListener != null) {
						mOnCompletionListener.onCompletion();
					}
					return;
				}
				case MEDIA_BUFFERING_UPDATE: {
//					Log.d(TAG, "MEDIA_BUFFERING_UPDATE: " + msg.arg1);
					if (mOnBufferingUpdateListener != null) {
						mOnBufferingUpdateListener.onBufferingUpdate(msg.arg1 /* percent */);
					}
					return;
				}
				case MEDIA_SEEK_COMPLETE: {
//					Log.d(TAG, "MEDIA_SEEK_COMPLETE");
					if (mOnSeekCompleteListener != null) {
						mOnSeekCompleteListener.onSeekComplete();
					}
					return;
				}
				case MEDIA_SET_VIDEO_SIZE: {
//					Log.d(TAG, "MEDIA_SET_VIDEO_SIZE");
					if (mOnVideoSizeChangedListener != null) {
						mOnVideoSizeChangedListener.onVideoSizeChanged(msg.arg1 /* width */, msg.arg2 /* height */);
					}
					return;
				}
				case MEDIA_ERROR: {
//					Log.d(TAG, "MEDIA_ERROR");
					if (mOnErrorListener != null) {
						mOnErrorListener.onError(msg.what, msg.arg1 /* extra */);
					}
					return;
				}
				case MEDIA_INFO: {
//					Log.d(TAG, "MEDIA_INFO: " + msg.arg1);
					if (mOnInfoListener != null) {
						mOnInfoListener.onInfo(msg.arg1, msg.what /* extra */);
					}
					return;
				}
				default: {
				    LogUtils.error("Unknown message type " + msg.what);
					return;
				}
			}
		}
	}
	
	protected OnPreparedListener mOnPreparedListener;
	
	/**
	 * Register a callback to be invoked when the media source is ready for
	 * playback.
	 * 
	 * @param listener
	 *            the callback that will be run
	 */
	public void setOnPreparedListener(OnPreparedListener listener) {
		mOnPreparedListener = listener;
	}
	
	protected OnCompletionListener mOnCompletionListener;
	
	/**
	 * Register a callback to be invoked when the end of a media source has been
	 * reached during playback.
	 * 
	 * @param listener
	 *            the callback that will be run
	 */
	public void setOnCompletionListener(OnCompletionListener listener) {
		mOnCompletionListener = listener;
	}
	
	protected OnBufferingUpdateListener mOnBufferingUpdateListener;
	
	/**
	 * Register a callback to be invoked when the status of a network stream's
	 * buffer has changed.
	 * 
	 * @param listener
	 *            the callback that will be run.
	 */
	public void setOnBufferingUpdateListener(OnBufferingUpdateListener listener) {
		mOnBufferingUpdateListener = listener;
	}
	
	protected OnSeekCompleteListener mOnSeekCompleteListener;
	
	/**
	 * Register a callback to be invoked when a seek operation has been
	 * completed.
	 * 
	 * @param listener
	 *            the callback that will be run
	 */
	public void setOnSeekCompleteListener(OnSeekCompleteListener listener) {
		mOnSeekCompleteListener = listener;
	}
	
	protected OnVideoSizeChangedListener mOnVideoSizeChangedListener;
	
	/**
	 * Register a callback to be invoked when the video size is known or
	 * updated.
	 * 
	 * @param listener
	 *            the callback that will be run
	 */
	public void setOnVideoSizeChangedListener(OnVideoSizeChangedListener listener) {
		mOnVideoSizeChangedListener = listener;
	}
	
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
	
	protected OnErrorListener mOnErrorListener;
	
	/**
	 * Register a callback to be invoked when an error has happened during an
	 * asynchronous operation.
	 * 
	 * @param listener
	 *            the callback that will be run
	 */
	public void setOnErrorListener(OnErrorListener listener) {
		mOnErrorListener = listener;
	}
	
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
	 * MediaPlayer is temporarily pausing playback internally in order to
	 * buffer more data.
	 * 
	 * @see android.media.MediaPlayer.OnInfoListener
	 */
	public static final int MEDIA_INFO_BUFFERING_START = 701;

	/** 
	 * MediaPlayer is resuming playback after filling buffers.
	 * 
	 * @see android.media.MediaPlayer.OnInfoListener
	 */
	public static final int MEDIA_INFO_BUFFERING_END = 702;
	
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
	
	protected OnInfoListener mOnInfoListener;
	
	/**
	 * Register a callback to be invoked when an info/warning is available.
	 * 
	 * @param listener
	 *            the callback that will be run
	 */
	public void setOnInfoListener(OnInfoListener listerner) {
		mOnInfoListener = listerner;
	}
	
}
