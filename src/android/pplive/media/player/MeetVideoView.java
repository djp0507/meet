package android.pplive.media.player;

import java.io.IOException;
import java.util.Map;

import android.content.Context;
import android.content.Intent;
import android.media.AudioManager;
import android.net.Uri;
import android.os.Build;
import android.pplive.media.MeetSDK;
import android.pplive.media.player.MediaController.MediaPlayerControl;
import android.pplive.media.util.LogUtils;
import android.util.AttributeSet;
import android.util.Log;
import android.view.KeyEvent;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.view.accessibility.AccessibilityEvent;
import android.view.accessibility.AccessibilityNodeInfo;

/**
 * Displays a video file. The VideoView class can load images from various sources (such as resources or content
 * providers), takes care of computing its measurement from the video so that it can be used in any layout manager, and
 * provides various display options such as scaling and tinting.
 */
public class MeetVideoView extends SurfaceView implements MediaPlayerControl {
    private static final String TAG = "MeetVideoView";
    // settable by the client
    private Uri mUri;

    // all possible internal states
    private static final int STATE_ERROR = -1;
    private static final int STATE_IDLE = 0;
    private static final int STATE_PREPARING = 1;
    private static final int STATE_PREPARED = 2;
    private static final int STATE_PLAYING = 3;
    private static final int STATE_PAUSED = 4;
    private static final int STATE_PLAYBACK_COMPLETED = 5;

    public static final int SCREEN_FIT = 0;
    public static final int SCREEN_STRETCH = 1;
    public static final int SCREEN_FILL = 2;
    public static final int SCREEN_CENTER = 3;

    private int mDisplayMode = SCREEN_FIT;

    // mCurrentState is a VideoView object's current state.
    // mTargetState is the state that a method caller intends to reach.
    // For instance, regardless the VideoView object's current state,
    // calling pause() intends to bring the object to a target state
    // of STATE_PAUSED.
    private int mCurrentState = STATE_IDLE;
    private int mTargetState = STATE_IDLE;

    // All the stuff we need for playing and showing a video
    private SurfaceHolder mSurfaceHolder = null;
    private MediaPlayer mMediaPlayer = null;
    private int mVideoWidth;
    private int mVideoHeight;
    private int mSurfaceWidth;
    private int mSurfaceHeight;
    private OnCompletionListener mOnCompletionListener;
    private OnPreparedListener mOnPreparedListener;
    private OnInfoListener mOnInfoListener;
    private OnErrorListener mOnErrorListener;
    private int mSeekWhenPrepared; // recording the seek position while preparing
    private int mCurrentBufferPercentage;

    private boolean mCanPause;
    private boolean mCanSeekBack;
    private boolean mCanSeekForward;
    private boolean mIsOMXSurface;
    private boolean mSwitchingSurface;

    private Context mContext;

    private DecodeMode mDecodeMode = DecodeMode.HW_SYSTEM;

    public MeetVideoView(Context context) {
        this(context, null);
    }

    public MeetVideoView(Context context, AttributeSet attrs) {
        this(context, attrs, 0);
    }

    public MeetVideoView(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
        this.mContext = context;
        initVideoView();
    }

    @Override
    public void onInitializeAccessibilityEvent(AccessibilityEvent event) {
        super.onInitializeAccessibilityEvent(event);
        event.setClassName(MeetVideoView.class.getName());
    }

    @Override
    public void onInitializeAccessibilityNodeInfo(AccessibilityNodeInfo info) {
        super.onInitializeAccessibilityNodeInfo(info);
        info.setClassName(MeetVideoView.class.getName());
    }

    public int resolveAdjustedSize(int desiredSize, int measureSpec) {
        return getDefaultSize(desiredSize, measureSpec);
    }

    @Override
    protected void onFinishInflate() {
        super.onFinishInflate();
        MeetSDK.init(getContext());
        MeetSDK.PPBoxLibName = "libppbox-armandroid-r4-gcc44-mt-1.1.0.so";
    }

    private void initVideoView() {
        mVideoWidth = 0;
        mVideoHeight = 0;
        getHolder().addCallback(mSHCallback);
        setFocusable(true);
        setFocusableInTouchMode(true);
        requestFocus();
        mCurrentState = STATE_IDLE;
        mTargetState = STATE_IDLE;
    }

    public void setVideoPath(String path) {
        setVideoURI(Uri.parse(path));
    }

    public void setVideoURI(Uri uri) {
        setVideoURI(uri, null);
    }

    private void setVideoURI(Uri uri, Map<String, String> headers) {
        Log.i(TAG, uri.toString());
        mUri = uri;
        mSeekWhenPrepared = 0;
        mIsOMXSurface = MeetSDK.setSurfaceType(getContext(), getHolder(), mUri);
        openVideo();
        requestLayout();
        invalidate();
    }

    public void stopPlayback() {
        if (mMediaPlayer != null) {
            mMediaPlayer.stop();
            mMediaPlayer.release();
            mMediaPlayer = null;
            mCurrentState = STATE_IDLE;
            mTargetState = STATE_IDLE;
        }
    }

    private void openVideo() {
        if (mUri == null || mSurfaceHolder == null) {
            // not ready for playback just yet, will try again later
            return;
        }

        Intent i = new Intent("com.android.music.musicservicecommand");
        i.putExtra("command", "pause");
        mContext.sendBroadcast(i);

        // we shouldn't clear the target state, because somebody might have
        // called start() previously
        release(false);
        try {
            mMediaPlayer = new MediaPlayer();
            mMediaPlayer.setOnPreparedListener(mPreparedListener);
            mMediaPlayer.setOnVideoSizeChangedListener(mSizeChangedListener);
            mMediaPlayer.setOnCompletionListener(mCompletionListener);
            mMediaPlayer.setOnErrorListener(mErrorListener);
            mMediaPlayer.setOnBufferingUpdateListener(mBufferingUpdateListener);
            mMediaPlayer.setOnInfoListener(mInfoListener);
            mCurrentBufferPercentage = 0;
            mMediaPlayer.setDataSource(getContext(), mUri);
            mMediaPlayer.setDisplay(mSurfaceHolder, mIsOMXSurface);
            mMediaPlayer.setAudioStreamType(AudioManager.STREAM_MUSIC);
            mMediaPlayer.setScreenOnWhilePlaying(true);
            mMediaPlayer.setDecodeMode(mDecodeMode);
            mMediaPlayer.prepareAsync();
            // we don't set the target state here either, but preserve the
            // target state that was there before.
            mCurrentState = STATE_PREPARING;
        } catch (IOException ex) {
            LogUtils.warn("Unable to open content: " + mUri);
            mCurrentState = STATE_ERROR;
            mTargetState = STATE_ERROR;
            mErrorListener.onError(MEDIA_ERROR_UNKNOWN, 0);
            return;
        } catch (IllegalArgumentException ex) {
            LogUtils.warn("Unable to open content: " + mUri);
            mCurrentState = STATE_ERROR;
            mTargetState = STATE_ERROR;
            mErrorListener.onError(MEDIA_ERROR_UNKNOWN, 0);
            return;
        }
    }

    OnVideoSizeChangedListener mSizeChangedListener = new OnVideoSizeChangedListener() {
        public void onVideoSizeChanged(int width, int height) {
            mVideoWidth = mMediaPlayer.getVideoWidth();
            mVideoHeight = mMediaPlayer.getVideoHeight();
            if (mVideoWidth != 0 && mVideoHeight != 0) {
                getHolder().setFixedSize(mVideoWidth, mVideoHeight);
                requestLayout();
            }
        }
    };

    OnPreparedListener mPreparedListener = new OnPreparedListener() {
        public void onPrepared() {
            mCurrentState = STATE_PREPARED;

            mCanPause = mCanSeekBack = mCanSeekForward = true;

            if (mOnPreparedListener != null) {
                mOnPreparedListener.onPrepared();
            }

            mVideoWidth = mMediaPlayer.getVideoWidth();
            mVideoHeight = mMediaPlayer.getVideoHeight();

            int seekToPosition = mSeekWhenPrepared; // mSeekWhenPrepared may be changed after seekTo() call
            if (seekToPosition != 0) {
                seekTo(seekToPosition);
            }
            if (mVideoWidth != 0 && mVideoHeight != 0) {
                // Log.i("@@@@", "video size: " + mVideoWidth +"/"+
                // mVideoHeight);
                getHolder().setFixedSize(mVideoWidth, mVideoHeight);
                if (mSurfaceWidth == mVideoWidth && mSurfaceHeight == mVideoHeight) {
                    // We didn't actually change the size (it was already at the
                    // size
                    // we need), so we won't get a "surface changed" callback,
                    // so
                    // start the video here instead of in the callback.
                    if (mTargetState == STATE_PLAYING) {
                        start();
                    }
                }
            } else {
                // We don't know the video size yet, but should start anyway.
                // The video size might be reported to us later.
                if (mTargetState == STATE_PLAYING) {
                    start();
                }
            }
        }
    };

    private OnCompletionListener mCompletionListener = new OnCompletionListener() {
        public void onCompletion() {
            mCurrentState = STATE_PLAYBACK_COMPLETED;
            mTargetState = STATE_PLAYBACK_COMPLETED;
            if (mOnCompletionListener != null) {
                mOnCompletionListener.onCompletion();
            }
        }
    };

    private OnErrorListener mErrorListener = new OnErrorListener() {
        public boolean onError(int framework_err, int impl_err) {
            if (framework_err == MEDIA_ERROR_SWITCH_SURFACE) {
                mIsOMXSurface = false;
                mSwitchingSurface = true;
                mVideoWidth = 0;
                mVideoHeight = 0;
                mSurfaceWidth = 0;
                mSurfaceHeight = 0;
                setVisibility(View.INVISIBLE);
                MeetSDK.setSurfaceType(getHolder(), mIsOMXSurface);
                setVisibility(View.VISIBLE);
            } else {
                mCurrentState = STATE_ERROR;
                mTargetState = STATE_ERROR;

                /* If an error handler has been supplied, use it and finish. */
                if (mOnErrorListener != null) {
                    if (mOnErrorListener.onError(framework_err, impl_err)) {
                        return true;
                    }
                }
            }
            return true;
        }
    };

    private OnInfoListener mInfoListener = new OnInfoListener() {
        public boolean onInfo(int what, int extra) {
            if (mOnInfoListener != null) {
                return mOnInfoListener.onInfo(what, extra);
            }
            return false;
        }
    };

    private OnBufferingUpdateListener mBufferingUpdateListener = new OnBufferingUpdateListener() {
        public void onBufferingUpdate(int percent) {
            mCurrentBufferPercentage = percent;
        }
    };

    /**
     * Register a callback to be invoked when the media file is loaded and ready to go.
     * 
     * @param l
     *            The callback that will be run
     */
    public void setOnPreparedListener(OnPreparedListener l) {
        mOnPreparedListener = l;
    }

    /**
     * Register a callback to be invoked when the end of a media file has been reached during playback.
     * 
     * @param l
     *            The callback that will be run
     */
    public void setOnCompletionListener(OnCompletionListener l) {
        mOnCompletionListener = l;
    }

    /**
     * Register a callback to be invoked when an error occurs during playback or setup. If no listener is specified, or
     * if the listener returned false, VideoView will inform the user of any errors.
     * 
     * @param l
     *            The callback that will be run
     */
    public void setOnErrorListener(OnErrorListener l) {
        mOnErrorListener = l;
    }

    /**
     * Register a callback to be invoked when an informational event occurs during playback or setup.
     * 
     * @param l
     *            The callback that will be run
     */
    public void setOnInfoListener(OnInfoListener l) {
        mOnInfoListener = l;
    }

    SurfaceHolder.Callback mSHCallback = new SurfaceHolder.Callback() {
        public void surfaceChanged(SurfaceHolder holder, int format, int w, int h) {
            mSurfaceWidth = w;
            mSurfaceHeight = h;
            boolean isValidState = (mTargetState == STATE_PLAYING);
            boolean hasValidSize = (mVideoWidth == w && mVideoHeight == h);
            if (mMediaPlayer != null && isValidState && hasValidSize) {
                if (mSeekWhenPrepared != 0) {
                    seekTo(mSeekWhenPrepared);
                }
                start();
            }
        }

        public void surfaceCreated(SurfaceHolder holder) {
            mSurfaceHolder = holder;
            openVideo();
        }

        public void surfaceDestroyed(SurfaceHolder holder) {
            // after we return from this we can't use the surface any more
            mSurfaceHolder = null;
            release(!mSwitchingSurface);
        }
    };

    /*
     * release the media player in any state
     */
    private void release(boolean cleartargetstate) {
        if (mMediaPlayer != null) {
            mMediaPlayer.stop();
            mMediaPlayer.release();
            mMediaPlayer = null;
            mCurrentState = STATE_IDLE;
            if (cleartargetstate) {
                mTargetState = STATE_IDLE;
            }
        }
    }

    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        boolean isKeyCodeSupported = keyCode != KeyEvent.KEYCODE_BACK && keyCode != KeyEvent.KEYCODE_VOLUME_UP && keyCode != KeyEvent.KEYCODE_VOLUME_DOWN
                && keyCode != KeyEvent.KEYCODE_MENU && keyCode != KeyEvent.KEYCODE_CALL && keyCode != KeyEvent.KEYCODE_ENDCALL;
        if (isInPlaybackState() && isKeyCodeSupported) {
            if (keyCode == KeyEvent.KEYCODE_HEADSETHOOK || keyCode == KeyEvent.KEYCODE_MEDIA_PLAY_PAUSE) {
                if (mMediaPlayer.isPlaying()) {
                    pause();
                } else {
                    start();
                }
                return true;
            } else if (keyCode == KeyEvent.KEYCODE_MEDIA_STOP) {
                if (mMediaPlayer.isPlaying()) {
                    pause();
                }
                return true;
            }
        }
        return super.onKeyDown(keyCode, event);
    }

    @Override
    public void start() {
        if (isInPlaybackState()) {
            mMediaPlayer.start();
            mCurrentState = STATE_PLAYING;
        }
        mTargetState = STATE_PLAYING;
    }

    public void pause() {
        if (isInPlaybackState()) {
            if (mMediaPlayer.isPlaying()) {
                mMediaPlayer.pause();
                mCurrentState = STATE_PAUSED;
            }
        }
        mTargetState = STATE_PAUSED;
    }

    public void suspend() {
        release(false);
    }

    public void resume() {
        openVideo();
    }

    @Override
    public int getDuration() {
        if (isInPlaybackState()) {
            return mMediaPlayer.getDuration();
        }
        return -1;
    }

    @Override
    public int getCurrentPosition() {
        if (isInPlaybackState()) {
            return mMediaPlayer.getCurrentPosition();
        }
        return 0;
    }

    @Override
    public void seekTo(int msec) {
        if (isInPlaybackState()) {
            mMediaPlayer.seekTo(msec);
            mSeekWhenPrepared = 0;
        } else {
            mSeekWhenPrepared = msec;
        }
    }

    @Override
    public boolean isPlaying() {
        return isInPlaybackState() && mMediaPlayer.isPlaying();
    }

    @Override
    public int getBufferPercentage() {
        if (mMediaPlayer != null) {
            return mCurrentBufferPercentage;
        }
        return 0;
    }

    private boolean isInPlaybackState() {
        return (mMediaPlayer != null && mCurrentState != STATE_ERROR && mCurrentState != STATE_IDLE && mCurrentState != STATE_PREPARING);
    }

    @Deprecated
    @Override
    public boolean canPause() {
        return mCanPause;
    }

    @Deprecated
    @Override
    public boolean canSeekBackward() {
        return mCanSeekBack;
    }

    @Deprecated
    @Override
    public boolean canSeekForward() {
        return mCanSeekForward;
    }

    @Override
    public void setDisplayMode(int mode) {
        mDisplayMode = mode;
        requestLayout();
    }

    public int getDisplayMode() {
        return mDisplayMode;
    }

    @Override
    public void switchDisplayMode() {
        setDisplayMode((mDisplayMode + 1) % 4);
    }

    @Override
    protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
        int width = getDefaultSize(mVideoWidth, widthMeasureSpec);
        int height = getDefaultSize(mVideoHeight, heightMeasureSpec);

        if (mVideoWidth > 0 && mVideoHeight > 0) {
            switch (mDisplayMode) {
            case SCREEN_CENTER:
                width = mVideoWidth;
                height = mVideoHeight;
                break;
            case SCREEN_FIT:
                if (mVideoWidth * height > width * mVideoHeight) {
                    height = width * mVideoHeight / mVideoWidth;
                } else if (mVideoWidth * height < width * mVideoHeight) {
                    width = height * mVideoWidth / mVideoHeight;
                }
            case SCREEN_FILL:
                if (mVideoWidth * height > width * mVideoHeight) {
                    width = height * mVideoWidth / mVideoHeight;
                } else if (mVideoWidth * height < width * mVideoHeight) {
                    height = width * mVideoHeight / mVideoWidth;
                }
            case SCREEN_STRETCH:
                /* Do nothing */
                break;
            default:
                break;
            }
        }
        setMeasuredDimension(width, height);
    }

    /**
     * Interface definition for a callback to be invoked when the media source is ready for playback.
     */
    public interface OnPreparedListener {
        /**
         * Called when the media file is ready for playback.
         * 
         * @param mp
         *            the MediaPlayer that is ready for playback
         */
        void onPrepared();
    }

    /**
     * Interface definition for a callback to be invoked when playback of a media source has completed.
     */
    public interface OnCompletionListener {
        /**
         * Called when the end of a media source is reached during playback.
         */
        void onCompletion();
    }

    /**
     * Interface definition of a callback to be invoked indicating buffering status of a media resource being streamed
     * over the network.
     */
    public interface OnBufferingUpdateListener {
        /**
         * Called to update status in buffering a media stream.
         * 
         * @param percent
         *            the percentage (0-100) of the buffer that has been filled thus far
         */
        void onBufferingUpdate(int percent);
    }

    /**
     * Interface definition of a callback to be invoked indicating the completion of a seek operation.
     */
    public interface OnSeekCompleteListener {
        /**
         * Called to indicate the completion of a seek operation.
         */
        void onSeekComplete();
    }

    /**
     * Interface definition of a callback to be invoked when the video size is first known or updated
     */
    public interface OnVideoSizeChangedListener {
        /**
         * Called to indicate the video size
         * 
         * @param width
         *            the width of the video
         * @param height
         *            the height of the video
         */
        void onVideoSizeChanged(int width, int height);
    }

    /**
     * Interface definition of a callback to be invoked when there has been an error during an asynchronous operation
     * (other errors will throw exceptions at method call time).
     */
    public interface OnErrorListener {
        /**
         * Called to indicate an error.
         * 
         * @param what
         *            the type of error that has occurred:
         *            <ul>
         *            <li>{@link #MEDIA_ERROR_UNKNOWN}
         *            <li>{@link #MEDIA_ERROR_SERVER_DIED}
         *            </ul>
         * @param extra
         *            an extra code, specific to the error. Typically implementation dependant.
         * @return True if the method handled the error, false if it didn't. Returning false, or not having an
         *         OnErrorListener at all, will cause the OnCompletionListener to be called.
         */
        boolean onError(int what, int extra);
    }

    /**
     * Interface definition of a callback to be invoked to communicate some info and/or warning about the media or its
     * playback.
     */
    public interface OnInfoListener {
        /**
         * Called to indicate an info or a warning.
         * 
         * @param what
         *            the type of info or warning.
         *            <ul>
         *            <li>{@link #MEDIA_INFO_UNKNOWN}
         *            <li>{@link #MEDIA_INFO_VIDEO_TRACK_LAGGING}
         *            <li>{@link #MEDIA_INFO_BAD_INTERLEAVING}
         *            <li>{@link #MEDIA_INFO_NOT_SEEKABLE}
         *            <li>{@link #MEDIA_INFO_METADATA_UPDATE}
         *            </ul>
         * @param extra
         *            an extra code, specific to the info. Typically implementation dependant.
         * @return True if the method handled the info, false if it didn't. Returning false, or not having an
         *         OnErrorListener at all, will cause the info to be discarded.
         */
        boolean onInfo(int what, int extra);
    }

    public static final int CAN_SEEK_BACKWARD = 1;
    public static final int CAN_SEEK_FORWARD = 2;
    public static final int CAN_PAUSE = 4;
    public static final int CAN_SEEK = 8;

    // //////////////////////////////////////////////////////////////////////////////

    /**
     * Constant to retrieve only the new metadata since the last call. // FIXME: unhide. // FIXME: add link to
     * getMetadata(boolean, boolean)
     */
    public static final boolean METADATA_UPDATE_ONLY = true;

    /**
     * Constant to retrieve all the metadata. // FIXME: unhide. // FIXME: add link to getMetadata(boolean, boolean)
     */
    public static final boolean METADATA_ALL = false;

    /**
     * Constant to enable the metadata filter during retrieval. // FIXME: unhide. // FIXME: add link to
     * getMetadata(boolean, boolean)
     */
    public static final boolean APPLY_METADATA_FILTER = true;

    /**
     * Constant to disable the metadata filter during retrieval. // FIXME: unhide. // FIXME: add link to
     * getMetadata(boolean, boolean)
     */
    public static final boolean BYPASS_METADATA_FILTER = false;

    /**
     * Unspecified media player error.
     * 
     * @see android.media.MediaPlayer.OnErrorListener
     */
    public static final int MEDIA_ERROR_UNKNOWN = 1;

    /**
     * Media server died. In this case, the application must release the MediaPlayer object and instantiate a new one.
     * 
     * @see android.media.MediaPlayer.OnErrorListener
     */
    public static final int MEDIA_ERROR_SERVER_DIED = 100;

    /**
     * The video is streamed and its container is not valid for progressive playback i.e the video's index (e.g moov
     * atom) is not at the start of the file.
     * 
     * @see android.media.MediaPlayer.OnErrorListener
     */
    public static final int MEDIA_ERROR_NOT_VALID_FOR_PROGRESSIVE_PLAYBACK = 200;
    public static final int MEDIA_ERROR_SWITCH_SURFACE = -200;

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
     * The video is too complex for the decoder: it can't decode frames fast enough. Possibly only the audio plays fine
     * at this stage.
     * 
     * @see android.media.MediaPlayer.OnInfoListener
     */
    public static final int MEDIA_INFO_VIDEO_TRACK_LAGGING = 700;

    /**
     * MediaPlayer is temporarily pausing playback internally in order to buffer more data.
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
     * Bad interleaving means that a media has been improperly interleaved or not interleaved at all, e.g has all the
     * video samples first then all the audio ones. Video is playing but a lot of disk seeks may be happening.
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

    public enum DecodeMode {
        HW_SYSTEM, HW_OMX {
            @Override
            public MediaPlayerInterface newInstance() {
                return Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN ? new NuMediaPlayer() : new OMXMediaPlayer();
            }
        },
        SW {
            @Override
            public MediaPlayerInterface newInstance() {
                return new FFMediaPlayer();
            }
        },
        AUTO, UNKNOWN;

        public MediaPlayerInterface newInstance() {
            return new SystemMediaPlayer();
        }
    }

    public void setDecodeMode(DecodeMode mode) {
        mDecodeMode = mode;
    }

    public DecodeMode getDecodeMode() {
        if (null != mMediaPlayer) {
            return mMediaPlayer.getDecodeMode();
        }

        return DecodeMode.UNKNOWN;
    }
}
