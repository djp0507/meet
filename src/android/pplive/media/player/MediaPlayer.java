package android.pplive.media.player;

import java.io.IOException;

import android.content.Context;
import android.net.Uri;
import android.os.Build;
import android.pplive.media.MeetSDK;
import android.pplive.media.player.MeetVideoView.DecodeMode;
import android.pplive.media.player.MeetVideoView.OnBufferingUpdateListener;
import android.pplive.media.player.MeetVideoView.OnCompletionListener;
import android.pplive.media.player.MeetVideoView.OnErrorListener;
import android.pplive.media.player.MeetVideoView.OnInfoListener;
import android.pplive.media.player.MeetVideoView.OnPreparedListener;
import android.pplive.media.player.MeetVideoView.OnSeekCompleteListener;
import android.pplive.media.player.MeetVideoView.OnVideoSizeChangedListener;
import android.pplive.media.util.LogUtils;
import android.view.SurfaceHolder;

class MediaPlayer {

	@SuppressWarnings("unused")
	private final static String TAG = "ppmedia/MediaPlayer";
	
	private static MediaPlayerInterface newInstance(MediaPlayer mp) {
		int sdk = Build.VERSION.SDK_INT;

		if (null != mp && mp.mPath.startsWith("/") && mp.mIsOMXSurface) {
			LogUtils.info("go SystemMediaPlayer");
			
			return new SystemMediaPlayer();
		} else if ((sdk >= 16) && mp.mIsOMXSurface) {
			LogUtils.info("go NuMediaPlayer");
			
			return new NuMediaPlayer();
		} else {
			LogUtils.info("DefaultMediaPlayer");

			MediaPlayerInterface player = null;
			try {
				if (mp.isOMXSurface()) {
					LogUtils.info("go OMXMediaPlayer");
					
					
					player = new OMXMediaPlayer();
				} else {
					LogUtils.info("go FFMediaPlayer");
					
					player = new FFMediaPlayer();
				}
				
				return player;
			} catch (RuntimeException e) {
				LogUtils.error("RuntimeException", e);
				
				return null;
			}
		}
	}
	
	private boolean mIsOMXSurface = false;
	private MediaPlayerInterface mPlayer = null;
	
	MediaPlayer() {
		LogUtils.info("MeetPlayer_VERSION: " + MeetSDK.getVersion());
	}

	private String mPath = null;
	void setDataSource(Context ctx, Uri uri) throws IOException, IllegalStateException,
			IllegalArgumentException, SecurityException {
		String path = MeetSDK.Uri2String(ctx, uri);
		setDataSource(path);
	}

	void setDataSource(String path) throws IOException, IllegalArgumentException, IllegalStateException {
		mPath = path;
		setDataSource();
	}
	
	private void setDataSource() throws IllegalArgumentException, IllegalStateException, IOException {
		LogUtils.info(mPath);
		if (hasPlayer()) {
			mPlayer.setDataSource(mPath);
		}
	}

	private SurfaceHolder mSurfaceHolder = null;	
	void setDisplay(SurfaceHolder sh) {
		setDisplay(sh, true);
	}
	
	
	void setDisplay(SurfaceHolder sh, boolean isOMXSurface) {
		mSurfaceHolder = sh;
		mIsOMXSurface = isOMXSurface;
		setDisplay();
	}
	
	private void setDisplay() {
		if (hasPlayer()) {
			mPlayer.setDisplay(mSurfaceHolder);
		}
	}

	void prepare() throws IOException, IllegalStateException {
		setupMediaPlayer();

		if (hasPlayer()) {
			mPlayer.prepare();
		}
	}

	void prepareAsync() throws IllegalStateException {
		setupMediaPlayer();

		if (hasPlayer()) {
			mPlayer.prepareAsync();
		}
	}
	
	private void setupMediaPlayer() throws IllegalStateException {
		if (hasPlayer()) {
			LogUtils.error("MediaPlayer is prepared.");
			throw new IllegalStateException("MediaPlayer is prepared.");
		}
		if (mPath == null) {
		    LogUtils.error("Media Path is not set.");
		    throw new IllegalStateException("Media Path is not set.");
		}
		
		mPlayer = newInstance(this);
		
		try {
			setDataSource();
		} catch (IllegalArgumentException e) {
			LogUtils.error("IllegalArgumentException", e);
			throw new IllegalStateException(e);
		} catch (IOException e) {
			LogUtils.error("IOException", e);
			throw new IllegalStateException(e);
		}
		
		setDisplay();
		setAudioStreamType();
		setOnBufferingUpdateListener();
		setOnCompletionListener();
		setOnErrorListener();
		setOnInfoListener();
		setOnPreparedListener();
		setOnSeekCompleteListener();
		setOnVideoSizeChangedListener();
		setScreenOnWhilePlaying();
		setWakeMode();
	}

	void start() throws IllegalStateException {
		if (hasPlayer()) {
			mPlayer.start();
		} else {
			LogUtils.error("MediaPlayer has't initialized!!!");
			throw new IllegalStateException("MediaPlayer has't initialized!!!");
		}
	}

	void stop() {
		if (hasPlayer()) {
			mPlayer.stop();
		}
	}

	void pause() {
		if (hasPlayer()) {
			mPlayer.pause();
		}
	}

	void seekTo(int seekingTime) throws IllegalStateException {
		if (hasPlayer()) {
			LogUtils.info("seekTo: " + seekingTime);
			mPlayer.seekTo(seekingTime);
		} else {
			LogUtils.error("mMeetPlayer is null");
			throw new IllegalStateException("mMeetPlayer is null");
		}
	}

	void release() {
		if (hasPlayer()) {
			mPlayer.release();
			mPlayer = null;
		}
	}

	private boolean mScreenOn;
	void setScreenOnWhilePlaying(boolean screenOn) {
		mScreenOn = screenOn;
		setScreenOnWhilePlaying();
	}
	
	private void setScreenOnWhilePlaying() {
		if (hasPlayer()) {
			mPlayer.setScreenOnWhilePlaying(mScreenOn);
		}
	}
	

	@Deprecated
	boolean suspend() {
		return hasPlayer() ? mPlayer.suspend() : false;
	}

	@Deprecated
	boolean resume() {
		return hasPlayer() ? mPlayer.resume() : false;
	}

	private Context mContext = null;
	
	private int mWakeMode;
	
	void setWakeMode(Context ctx, int mode) {
		mContext = ctx;
		mWakeMode = mode;
		setWakeMode();
	}
	
	private void setWakeMode() {
		if (hasPlayer() && mContext != null) {
			mPlayer.setWakeMode(mContext, mWakeMode);
		}
	}

	int getVideoWidth() {
		
		int width = 0;
		if (hasPlayer()) {
			width = mPlayer.getVideoWidth();
		}
		return width;
	}

	int getVideoHeight() {
		
		int height = 0;
		if (hasPlayer()) {
			height = mPlayer.getVideoHeight();
		}
		return height;
	}

	boolean isPlaying() {
		if (hasPlayer()) {
			return mPlayer.isPlaying();
		}
		return false;
	}
	
	boolean isOMXSurface() {
		return mIsOMXSurface;
	}

	int getCurrentPosition() {
		
		int position = 0;
		if (hasPlayer()) {
			position = mPlayer.getCurrentPosition();
		}
		return position;
	}

	int getDuration() {
		
		int duration = 0;
		if (hasPlayer()) {
			duration = mPlayer.getDuration();
		}
		return duration;
	}

	int flags() throws IllegalStateException {
		
		int flags = 0;
		if (hasPlayer()) {
			flags = mPlayer.flags();
		}
		return flags;
	}

	private int mStreamType = 0;
	void setAudioStreamType(int streamtype) {
		mStreamType = streamtype;
		setAudioStreamType();
	}
	
	private void setAudioStreamType() {
		if (hasPlayer()) {
			mPlayer.setAudioStreamType(mStreamType);
		}
	}

	protected void reset() {
		if (hasPlayer()) {
			mPlayer.reset();
		}
	}
	
	private OnPreparedListener mOnPreparedListener = null;
	void setOnPreparedListener(OnPreparedListener listener) {
		mOnPreparedListener = listener;
		setOnPreparedListener();
	}
	
	private void setOnPreparedListener() {
		if (hasPlayer()) {
			mPlayer.setOnPreparedListener(mOnPreparedListener);
		}
	}
	
	private OnCompletionListener mOnCompletionListener = null;
	void setOnCompletionListener(OnCompletionListener listener) {
		mOnCompletionListener = listener;
		setOnCompletionListener();
	}
	
	private void setOnCompletionListener() {
		if (hasPlayer()) {
			mPlayer.setOnCompletionListener(mOnCompletionListener);
		}
	}
	
	private OnBufferingUpdateListener mOnBufferingUpdateListener = null;
	public void setOnBufferingUpdateListener(OnBufferingUpdateListener listener) {
		mOnBufferingUpdateListener = listener;
		setOnBufferingUpdateListener();
	}
	
	private void setOnBufferingUpdateListener() {
		if (hasPlayer()) {
			mPlayer.setOnBufferingUpdateListener(mOnBufferingUpdateListener);
		}
	}
	
	private OnSeekCompleteListener mOnSeekCompleteListener = null;
	void setOnSeekCompleteListener(OnSeekCompleteListener listener) {
		mOnSeekCompleteListener = listener;
		setOnSeekCompleteListener();
	}
	
	private void setOnSeekCompleteListener() {
		if (hasPlayer()) {
			mPlayer.setOnSeekCompleteListener(mOnSeekCompleteListener);
		}
	}

	private OnVideoSizeChangedListener mOnVideoSizeChangedListener= null;
	void setOnVideoSizeChangedListener(OnVideoSizeChangedListener listener) {
		mOnVideoSizeChangedListener = listener;
		setOnVideoSizeChangedListener();
	}
	
	private void setOnVideoSizeChangedListener() {
		if (hasPlayer()) {
			mPlayer.setOnVideoSizeChangedListener(mOnVideoSizeChangedListener);
		}
	}

	private OnErrorListener mOnErrorListener = null;
	void setOnErrorListener(OnErrorListener listener) {
		mOnErrorListener = listener;
		setOnErrorListener();
	}
	
	private void setOnErrorListener() {
		if (hasPlayer()) {
			mPlayer.setOnErrorListener(mOnErrorListener);
		}
	}

	private OnInfoListener mOnInfoListener = null;
	void setOnInfoListener(OnInfoListener listerner) {
		mOnInfoListener = listerner;
		setOnInfoListener();
	}
	
	private void setOnInfoListener() {
		if (hasPlayer()) {
			mPlayer.setOnInfoListener(mOnInfoListener);
		}
	}
	
	private boolean hasPlayer() {
		return mPlayer != null;
	}
	
	private DecodeMode mDecodeMode = DecodeMode.HW_SYSTEM;
	void setDecodeMode(DecodeMode mode) {
		mDecodeMode = mode;
	}
	
	DecodeMode getDecodeMode() {
		if (hasPlayer()) {
			return mPlayer.getDecodeMode();
		}
		
		return DecodeMode.UNKNOWN;
	}
}
