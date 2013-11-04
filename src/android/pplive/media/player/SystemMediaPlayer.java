package android.pplive.media.player;

import static android.pplive.media.player.MeetVideoView.CAN_PAUSE;
import static android.pplive.media.player.MeetVideoView.CAN_SEEK;
import static android.pplive.media.player.MeetVideoView.CAN_SEEK_BACKWARD;
import static android.pplive.media.player.MeetVideoView.CAN_SEEK_FORWARD;

import java.lang.reflect.Method;

import android.media.MediaPlayer;
import android.pplive.media.player.MeetVideoView.DecodeMode;
import android.pplive.media.util.LogUtils;

class SystemMediaPlayer extends android.media.MediaPlayer implements MediaPlayerInterface {
	
	@SuppressWarnings("unused")
	private static final String TAG = "ppmedia/SystemMediaPlayer";
	
	public SystemMediaPlayer() {}
	
	private static final int PAUSE_AVAILABLE         = 1; // Boolean
	private static final int SEEK_BACKWARD_AVAILABLE = 2; // Boolean
	private static final int SEEK_FORWARD_AVAILABLE  = 3; // Boolean
	private static final int SEEK_AVAILABLE          = 4; // Boolean
	
	private static final int NEW_PAUSE_AVAILABLE         = 29; // Boolean
	private static final int NEW_SEEK_BACKWARD_AVAILABLE = 30; // Boolean
	private static final int NEW_SEEK_FORWARD_AVAILABLE  = 31; // Boolean
	
	@Override
	public int flags() throws IllegalStateException {
		int flags = CAN_PAUSE | CAN_SEEK_BACKWARD | CAN_SEEK_FORWARD  | CAN_SEEK;
		try {
			
			Class<?> clazz = android.media.MediaPlayer.class;

			Method getMetadataMethod = clazz.getDeclaredMethod("getMetadata", boolean.class, boolean.class);
			getMetadataMethod.setAccessible(true);
			Object metadata = getMetadataMethod.invoke(this, false, false);
			
			if (metadata != null) {
				clazz = metadata.getClass();
				Method getBooleanMethod = clazz.getDeclaredMethod("getBoolean", int.class);
				getBooleanMethod.setAccessible(true);
				
				flags = 0;
				
				flags |= (Boolean) getBooleanMethod.invoke(metadata, PAUSE_AVAILABLE) || 
							(Boolean) getBooleanMethod.invoke(metadata, NEW_PAUSE_AVAILABLE) ? CAN_PAUSE : 0;
				flags |= (Boolean) getBooleanMethod.invoke(metadata, SEEK_BACKWARD_AVAILABLE) ||
							(Boolean) getBooleanMethod.invoke(metadata, NEW_SEEK_BACKWARD_AVAILABLE) ? CAN_SEEK_BACKWARD : 0;
				flags |= (Boolean) getBooleanMethod.invoke(metadata, SEEK_FORWARD_AVAILABLE) ||
							(Boolean) getBooleanMethod.invoke(metadata, NEW_SEEK_FORWARD_AVAILABLE) ? CAN_SEEK_FORWARD : 0;
				flags |= (Boolean) getBooleanMethod.invoke(metadata, SEEK_AVAILABLE)? CAN_SEEK : 0;
			}
			
		} catch (Exception e) {
		    LogUtils.error("Exception", e);
		}
		
		return CAN_PAUSE | CAN_SEEK | CAN_SEEK_BACKWARD | CAN_SEEK_FORWARD;
	}

	public void setDataSource(String uri)  {
	    LogUtils.info(uri);
		try {
			super.setDataSource(uri);
		} catch(Exception e) {
			LogUtils.error("Exception", e);
		}
	}

	public void prepareAsync() {
		
		try {
			super.prepareAsync();
		} catch(Exception e) {
			if (null != mOnErrorListener) {
				mOnErrorListener.onError(MeetVideoView.MEDIA_ERROR_SWITCH_SURFACE, 0);
			}
			else
			{
			    LogUtils.error("mOnErrorListener is NULL", e);
			}
		}
	}
	
	private MeetVideoView.OnBufferingUpdateListener mOnBufferingUpdateListener;
	private android.media.MediaPlayer.OnBufferingUpdateListener mSystemOnBufferingUpdateListener = new OnBufferingUpdateListener() {
		
		@Override
		public void onBufferingUpdate(MediaPlayer mp, int percent) {
			if (null != mOnBufferingUpdateListener) {
				mOnBufferingUpdateListener.onBufferingUpdate(percent);
			}
		}
	};
	
	public void setOnBufferingUpdateListener(MeetVideoView.OnBufferingUpdateListener listener) {
		mOnBufferingUpdateListener = listener;
		super.setOnBufferingUpdateListener(mSystemOnBufferingUpdateListener);
	}
	
	private MeetVideoView.OnCompletionListener mOnCompletionListener;
	private android.media.MediaPlayer.OnCompletionListener mSystemCompletionListener = new OnCompletionListener() {
		
		@Override
		public void onCompletion(MediaPlayer mp) {
			if (null != mOnCompletionListener) {
				mOnCompletionListener.onCompletion();
			}
		}
	};
	
	public void setOnCompletionListener(MeetVideoView.OnCompletionListener listener) {
		mOnCompletionListener = listener;
		super.setOnCompletionListener(mSystemCompletionListener);
	}
	
	private MeetVideoView.OnErrorListener mOnErrorListener;
	private android.media.MediaPlayer.OnErrorListener mSystemErrorListener = new OnErrorListener() {
		
		@Override
		public boolean onError(MediaPlayer mp, int what, int extra) {
			if (null != mOnErrorListener) {
				return mOnErrorListener.onError(MeetVideoView.MEDIA_ERROR_SWITCH_SURFACE, extra);
			}
			return false;
		}
	};
	
	public void setOnErrorListener(MeetVideoView.OnErrorListener listener) {
		mOnErrorListener = listener;
		super.setOnErrorListener(mSystemErrorListener);
	}
	
	private MeetVideoView.OnInfoListener mOnInfoListener;
	private android.media.MediaPlayer.OnInfoListener mSystemOnInfoListener = new OnInfoListener() {
		
		@Override
		public boolean onInfo(MediaPlayer mp, int what, int extra) {
			if (null != mOnInfoListener) {
				return mOnInfoListener.onInfo(what, extra);
			}
			return false;
		}
	};
	
	public void setOnInfoListener(MeetVideoView.OnInfoListener listener) {
		mOnInfoListener = listener;
		super.setOnInfoListener(mSystemOnInfoListener);
	}
	
	private MeetVideoView.OnPreparedListener mOnPreparedListener;
	private android.media.MediaPlayer.OnPreparedListener mSystemOnPreparedListener = new OnPreparedListener() {
		
		@Override
		public void onPrepared(MediaPlayer mp) {
			if (null != mOnPreparedListener) {
				mOnPreparedListener.onPrepared();
			}
		}
	};
	
	public void setOnPreparedListener(MeetVideoView.OnPreparedListener listener) {
		mOnPreparedListener = listener;
		super.setOnPreparedListener(mSystemOnPreparedListener);
	}
	
	private MeetVideoView.OnSeekCompleteListener mOnSeekCompleteListener;
	private android.media.MediaPlayer.OnSeekCompleteListener mSystemOnSeekCompleteListener = new OnSeekCompleteListener() {
		
		@Override
		public void onSeekComplete(MediaPlayer mp) {
			if (null != mOnSeekCompleteListener) {
				mOnSeekCompleteListener.onSeekComplete();
			}
		}
	};
	
	public void setOnSeekCompleteListener(MeetVideoView.OnSeekCompleteListener listener) {
		mOnSeekCompleteListener = listener;
		super.setOnSeekCompleteListener(mSystemOnSeekCompleteListener);
	}
	
	private MeetVideoView.OnVideoSizeChangedListener mOnVideoSizeChangedListener;
	private android.media.MediaPlayer.OnVideoSizeChangedListener mSystemOnVideoSizeChangedListener = new OnVideoSizeChangedListener() {
		
		@Override
		public void onVideoSizeChanged(MediaPlayer mp, int width, int height) {
			if (null != mOnVideoSizeChangedListener) {
				mOnVideoSizeChangedListener.onVideoSizeChanged(width, height);
			}
		}
	};
	
	public void setOnVideoSizeChangedListener(MeetVideoView.OnVideoSizeChangedListener listener) {
		mOnVideoSizeChangedListener = listener;
		super.setOnVideoSizeChangedListener(mSystemOnVideoSizeChangedListener);
	}

	@Override
	@Deprecated
	public boolean suspend() {
		return false;
	}

	@Override
	@Deprecated
	public boolean resume() {
		return false;
	}

	@Override
	public DecodeMode getDecodeMode() {
		return DecodeMode.HW_SYSTEM;
	}
}
