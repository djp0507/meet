/**
 * Copyright (C) 2012 PPTV
 * 
 */
package android.pplive.media.player;

import static android.pplive.media.player.MeetVideoView.CAN_PAUSE;
import static android.pplive.media.player.MeetVideoView.CAN_SEEK;
import static android.pplive.media.player.MeetVideoView.CAN_SEEK_BACKWARD;
import static android.pplive.media.player.MeetVideoView.CAN_SEEK_FORWARD;

import java.io.IOException;
import java.nio.ByteBuffer;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.locks.Condition;
import java.util.concurrent.locks.Lock;
import java.util.concurrent.locks.ReentrantLock;

import android.content.Context;
import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioTrack;
import android.media.MediaCodec;
import android.media.MediaExtractor;
import android.media.MediaFormat;
import android.net.Uri;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Message;
import android.pplive.media.player.MeetVideoView.DecodeMode;
import android.pplive.media.util.LogUtils;
import android.pplive.media.util.UrlUtil;
import android.pplive.media.util.Utils;
import android.util.Log;
import android.view.Surface;
import android.view.SurfaceHolder;

class NuMediaPlayer extends MeetMediaPlayer {
	
	private static final String TAG = "ppmedia/NuMediaPlayer2";
	
	private static final int EVENT_READ_SAMPLE = 1000;
	private static final int EVENT_DECODE_AUDIO = 1001;
	private static final int EVENT_DECODE_VIDEO = 1002;
	private static final int EVENT_SEEKTO = 1003;
	private static final int EVENT_BUFFERING_UPDATE_CHECK = 1004;
	private static final int EVENT_BUFFERING_START_CHECK = 1005;
	private static final int EVENT_BUFFERING_END_CHECK = 1006;
	
	private String mUri = null;
	
	private boolean mIsOnline = true; // default is online mode
	private boolean mIsBuffering = false;
	
	private Surface mSurface = null;
	private MediaExtractable mExtractor = null;
	
	private int mVideoWidth = 0;
	private int mVideoHeight = 0;

	private AudioTrack mAudioTrack = null;
	
	private MediaFormat mAudioFormat = null;
	private MediaFormat mVideoFormat = null;
	
	private MediaFormat mOutputAudioFormat = null;
	private MediaFormat mOutputVideoFormat = null;
	
	private int mAudioTrackIndex = -1;
	private int mVideoTrackIndex = -1;
	private int mLastTrackIndex = -1;
	
	private boolean mHaveAudio = false;
	private boolean mHaveVideo = false;
	
	private MediaCodec mAudioCodec = null;
	private MediaCodec mVideoCodec = null;
	
	private MediaCodec.BufferInfo mAudioBufferInfo = null;
	private MediaCodec.BufferInfo mVideoBufferInfo = null;
	
	private byte[] mAudioData = null;
	
	private boolean mSawInputEOS = false;
	private boolean mSawOutputEOS = false;
	
	private long mDuration = 0L;
	private long mSeekingTimeUs = 0L;
	private long mCurrentTimeUs = 0L;
	
	private Handler mMediaEventHandler = null;
	
	private boolean mSeeking = false;
	
	private Lock mLock = new ReentrantLock();
	private Condition notStopped = mLock.newCondition();
	
	NuMediaPlayer() {
		
		HandlerThread ht = new HandlerThread("MediaEventHandler");
		ht.start();
		
		mMediaEventHandler = new Handler(ht.getLooper()) {
			
			@Override
			public void handleMessage(Message msg) {
				switch (msg.what) {
					case EVENT_READ_SAMPLE: {
//						Log.d(TAG, "EVENT_READ_SAMPLE");
						onReadSampleEvent();
						return;
					} case EVENT_DECODE_AUDIO: {
//						Log.d(TAG, "EVENT_DECODE_AUDIO");
						onDecodeAudioEvent();
						return;
					} case EVENT_DECODE_VIDEO: {
//						Log.d(TAG, "EVENT_DECODE_VIDEO");
						onDecodeVideoEvent();
						return;
					} case EVENT_SEEKTO: {
//						Log.d(TAG, "EVENT_SEEKTO");
						onSeekToEvent();
						return;
					} case EVENT_BUFFERING_UPDATE_CHECK: {
//						Log.d(TAG, "EVENT_BUFFERING_UPDATE_CHECK");
						onBufferingUpdateCheckEvent();
						return;
					} case EVENT_BUFFERING_START_CHECK: {
//						Log.d(TAG, "EVENT_BUFFERING_START_CHECK");
						onBufferingStartCheckEvent();
						return;
					} case EVENT_BUFFERING_END_CHECK: {
//						Log.d(TAG, "EVENT_BUFFERING_END_CHECK");
						onBufferingEndCheckEvent();
						return;
					} default: {
						LogUtils.error("Unknown message type " + msg.what);
						return;
					}
				}
			}
		};
	}
	
	@Override
	public void setDataSource(Context ctx, Uri uri) throws IOException,
			IllegalArgumentException, SecurityException, IllegalStateException {
		setDataSource(uri.toString());
	}
	
	@Override
	public void setDataSource(String uri) throws IOException,
			IllegalArgumentException, IllegalStateException {
	    LogUtils.info(uri);
		
		if (uri == null || uri.equals("") || (mUri = uri) == null) {
			throw new IllegalArgumentException("Invalid Uri!!!");
		}
		
		MediaExtractable extractor = 
				UrlUtil.isNuMediaPlayerSupportUrl(mUri) ? 
				new PPMediaExtractor() : new DefaultMediaExtractor();
					
		mIsOnline = UrlUtil.isOnlinePlayUrl(mUri);
		
		setDataSource(extractor);
	}
	
	private void setDataSource(MediaExtractable extractor) throws IllegalStateException {
		
		mExtractor = extractor;
		setState(PlayState.INITIALIZED);
	}
	
	private void postSetVideoSizeEvent(int width, int height) {
	    LogUtils.info("width: " + width + ", height: " + height);
		
		mVideoWidth = width;
		mVideoHeight = height;
		
		Message msg = mEventHandler.obtainMessage(MEDIA_SET_VIDEO_SIZE, width, height);
		msg.sendToTarget();
	}

	@Override
	public void setDisplay(SurfaceHolder sh) {
		super.setDisplay(sh);
		
		Surface surface = null;
		if (sh != null) {
			surface = sh.getSurface();
		}
		
		setSurface(surface);
	}
	
	private void setSurface(Surface surface) {
		mSurface = surface;
	}

	@Override
	public void prepare() throws IOException, IllegalStateException {
		throw new IllegalStateException("Do not support this operation.");
	}

	@Override
	public void prepareAsync() throws IllegalStateException {
		PlayState state = getState();
		if (state != PlayState.INITIALIZED) {
		    LogUtils.error("Error State: " + state);
			throw new IllegalStateException("Error State: " + state);
		}

		setState(PlayState.PREPARING);
		
		mMediaEventHandler.postAtFrontOfQueue(new Runnable() {
			
			@Override
			public void run() {
				mLock.lock();
//				Log.d(TAG, "onPrepareAsyncEvent Start!!!");
				
				if (initMediaExtractor() && 
					initAudioTrack() &&
					initAudioDecoder() &&
					initVideoDecoder()) {
					
					setState(PlayState.PREPARED);
					mEventHandler.sendEmptyMessage(MEDIA_PREPARED);
				} else {
					setState(PlayState.ERROR);
					mEventHandler.sendEmptyMessage(MEDIA_ERROR);
				}
				
				mLock.unlock();
			}
		});
	}
	
	private boolean initMediaExtractor() {
		try {
			mExtractor.setDataSource(mUri);
		} catch (Exception e) {
		    LogUtils.error("Exception", e);
			return false;
		}

		int trackCount = mExtractor.getTrackCount();
		for (int index = 0; index < trackCount; index++) {
			MediaFormat format =
				mExtractor.getTrackFormat(index);
			
			String mime =
				format.getString(MediaFormat.KEY_MIME);
			
			if (mime == null || mime.equals("")) {
				continue;
			} else {
				mime = mime.toLowerCase();
			}
			LogUtils.info("mime:" + mime);
			
			if (!mHaveAudio && mime.startsWith("audio/")) {
				setAudioFormat(format);
				mAudioTrackIndex = index;
				mHaveAudio = true;
			} else if (!mHaveVideo && mime.startsWith("video/")) {
				setVideoFormat(format);
				mVideoTrackIndex = index;
				mHaveVideo = true;
			} else {
				// unknown media type;
			}
			
			if (mHaveAudio && mHaveVideo) {
				break;
			}
		}
		
		if (!mHaveAudio || !mHaveVideo) {
			return false;
		}
		
//		Log.d(TAG, "Init MediaExtractor Success!!!");
		return true;
	}
	
	private void setAudioFormat(MediaFormat format) {
//		Log.d(TAG, "setAudioFormat()");
		mAudioFormat = format;
		
		long duration = format.getLong(MediaFormat.KEY_DURATION);
//		Log.d(TAG, "audio duration: " + duration);
		mDuration = mDuration > duration ? mDuration : duration;
	}
	
	private void setVideoFormat(MediaFormat format) {
//		Log.d(TAG, "setVideoFormat()");
		mVideoFormat = format;
		
		long duration = format.getLong(MediaFormat.KEY_DURATION);
//		Log.d(TAG, "video duration: " + duration);
		mDuration = mDuration > duration ? mDuration : duration;
		
		int width = mVideoFormat.getInteger(MediaFormat.KEY_WIDTH);
		int height = mVideoFormat.getInteger(MediaFormat.KEY_HEIGHT);
		
		postSetVideoSizeEvent(width, height);
	}
	
	private boolean initAudioTrack() {
		if (mAudioTrack != null) {
			mAudioTrack.flush();
			mAudioTrack.stop();
			mAudioTrack.release();
			mAudioTrack = null;
		}
		
		int channelCount = mAudioFormat.getInteger(MediaFormat.KEY_CHANNEL_COUNT);
		int channelConfig = (channelCount >= 2) ? AudioFormat.CHANNEL_OUT_STEREO : AudioFormat.CHANNEL_OUT_MONO;
		
		int sampleRateInHz = mAudioFormat.getInteger(MediaFormat.KEY_SAMPLE_RATE);
		
		int minSize = 
			AudioTrack.getMinBufferSize(
				sampleRateInHz,
				channelConfig,
				AudioFormat.ENCODING_PCM_16BIT);
		
		mAudioTrack = 
			new AudioTrack(
				AudioManager.STREAM_MUSIC,
				sampleRateInHz,
				channelConfig, 
				AudioFormat.ENCODING_PCM_16BIT,
				minSize,
				AudioTrack.MODE_STREAM);
		
		if (mAudioTrack.getState() == AudioTrack.STATE_INITIALIZED) {
			mAudioTrack.play();
		}
		
//		Log.d(TAG, "Init Audio Track Success!!!");
		
		return true;
	}
	
	private boolean initAudioDecoder() {
		boolean ret = true;
		
		String mime = mAudioFormat.getString(MediaFormat.KEY_MIME);
		try {
			mAudioCodec = MediaCodec.createDecoderByType(mime);
			
			mAudioCodec.configure(mAudioFormat, null /* surface */, null /* crypto */, 0 /* flags */);
			mAudioCodec.start();
			
			mExtractor.selectTrack(mAudioTrackIndex);
			
			mAudioBufferInfo = new MediaCodec.BufferInfo();
			
			LogUtils.info("Init Audio Decoder Success!!!");
		} catch (Exception e) {
		    LogUtils.error("Exception", e);
			
			mAudioCodec = null;
			ret = false;
		}
		
		return ret;
	}
	
	private boolean initVideoDecoder() {
		boolean ret = true;
		
		String mime = mVideoFormat.getString(MediaFormat.KEY_MIME);
		try {
			mVideoCodec = MediaCodec.createDecoderByType(mime);
			
			mVideoCodec.configure(mVideoFormat, mSurface /* surface */, null /* crypto */, 0 /* flags */);
			mVideoCodec.start();
	
			mExtractor.selectTrack(mVideoTrackIndex);
			
			mVideoBufferInfo = new MediaCodec.BufferInfo();
			
			LogUtils.info("Init Video Decoder Success!!!");
		} catch (Exception e) {
		    LogUtils.error("Exception", e);
			
			mVideoCodec = null;
			ret = false;
		}
		
		return ret;
	}
	
	@Override
	public void start() throws IllegalStateException {
		PlayState state = getState();
		if (isPlaying()) {
			return;
		} else if (state != PlayState.PREPARED && state != PlayState.PAUSED) {
		    LogUtils.error("IllegalStateException - Error State: " + state);
			throw new IllegalStateException("Error State: " + state);
		}
		
		stayAwake(true);
		
		setState(PlayState.STARTED);
		play_l();
	}
	
	private void play_l() {
//		removeAllEvents();
		
		postReadSampleEvent();
		postDecodeAudioEvent();
		postDecodeVideoEvent();
		
		postBufferingUpdateCheckEvent();
	}
	
	private void postReadSampleEvent() {
		postReadSampleEvent(0 /* delayMillis */);
	}
	
	private void postReadSampleEvent(int delayMillis) {
//		Log.d(TAG, "postWriteEvent");
		
		postBufferingStartCheckEvent();
		
		mMediaEventHandler.sendEmptyMessageDelayed(EVENT_READ_SAMPLE, delayMillis);
	}
	
	private void onReadSampleEvent() {
//		Log.d(TAG, "onWriteSampleEvent");
		
		int trackIndex = mExtractor.getSampleTrackIndex();
		MediaCodec codec = getCodec(trackIndex);
		
		int inputBufIndex = codec.dequeueInputBuffer(TIMEOUT);
//		Log.d(TAG, "[onReadSampleEvent] inputBufIndex: " + inputBufIndex);
		
		if (inputBufIndex > 0) {
			ByteBuffer inputBuf = codec.getInputBuffers()[inputBufIndex];
			int sampleSize = mExtractor.readSampleData(inputBuf, 0 /* offset */);
			
			long presentationTimeUs = 0L;
			int flags = 0;
			
			if (sampleSize < 0) {
				Log.v(TAG, "saw Input EOS.");
				mSawInputEOS = true;
				sampleSize = 0;
			} else {
				presentationTimeUs = mExtractor.getSampleTime();
				flags = mExtractor.getSampleFlags();
			}
			
//			Log.d(TAG, "[ReadSample] trackIndex:" + trackIndex + "; presentationTimeUs: " + presentationTimeUs + "; flags: " + flags);
			
			codec.queueInputBuffer(
					inputBufIndex,
					0 /* offset */,
					sampleSize,
					presentationTimeUs,
					mSawInputEOS ? MediaCodec.BUFFER_FLAG_END_OF_STREAM : flags);
			
//			postReadEvent(trackIndex);
			
			if (!mSawInputEOS) {
				mExtractor.advance();
			}
		}
		
		if ((isPlaying() && !mSawInputEOS) || mSeeking) {
			postReadSampleEvent();
		}
	}
	
	private MediaCodec getCodec(int trackIndex) {
		MediaCodec codec = null; 
		
		if (trackIndex == mAudioTrackIndex) {
			codec = mAudioCodec;
		} else if (trackIndex == mVideoTrackIndex) {
			codec = mVideoCodec;
		} else {
//			Log.d(TAG, "trackIndex: " + trackIndex);
			codec = 
				mLastTrackIndex >= 0 ? getCodec(mLastTrackIndex) : null;
		}
		
		if (trackIndex >= 0) {
			mLastTrackIndex = trackIndex;
		}

		return codec;
	}
/*	
	private void postReadSampleEvent(int trackIndex) {
		if (trackIndex == mAudioTrackIndex) {
			postAudioReadEvent( delaymillis );
		} else if (trackIndex == mVideoTrackIndex) {
			postVideoReadEvent( delaymillis );
		} else {
//			Log.v(TAG, "saw output EOS.");
//			mSawOutputEOS = true;
//			postPlaybackCompletionEvent();
		}
	}
*/	
	private void postDecodeAudioEvent() {
		postDecodeAudioEvent(0 /* delaymillis */);
	}
	
	private void postDecodeAudioEvent(int delayMillis) {
//		Log.d(TAG, "postAudioReadEvent");
		mMediaEventHandler.sendEmptyMessageDelayed(EVENT_DECODE_AUDIO, delayMillis);
	}
	
	private void onDecodeAudioEvent() {
//		Log.d(TAG, "onAudioReadEvent");
		
		MediaCodec.BufferInfo info = mAudioBufferInfo;
		int res = mAudioCodec.dequeueOutputBuffer(info, TIMEOUT);
//		Log.d(TAG, "[OnAudioReadEvent] res: " + res);
		
		if (res > 0) {
//			Log.d(TAG, "[DecodeAudioBuffer] presentationTimeUs: " + info.presentationTimeUs + "; flags: " + info.flags);

			int outputBufIndex = res;
			ByteBuffer outputBuf = mAudioCodec.getOutputBuffers()[outputBufIndex];
			
			if (!mSeeking) {
				int bufSize = info.size;
				if (mAudioData == null || mAudioData.length < bufSize) {
					// Allocate a new buffer.
					mAudioData = new byte[bufSize];
				}
				
				outputBuf.get(mAudioData);
				mAudioTrack.write(mAudioData, 0 /* offsetInBytes */, bufSize);
			}
			
			setCurrentTimeUs(info.presentationTimeUs);
			
			mAudioCodec.releaseOutputBuffer(outputBufIndex, false /* render */);
			
			if ((info.flags & MediaCodec.BUFFER_FLAG_END_OF_STREAM) != 0) {
//				Log.v(TAG, "saw output EOS.");
				mSawOutputEOS = true;
				postPlaybackCompletionEvent();
			}
		} else if (res == MediaCodec.INFO_OUTPUT_BUFFERS_CHANGED) {
//			Log.d(TAG, "Audio: INFO_OUTPUT_BUFFERS_CHANGED");
			
		} else if (res == MediaCodec.INFO_OUTPUT_FORMAT_CHANGED) {
//			Log.d(TAG, "Audio: INFO_OUTPUT_FORMAT_CHANGED");
			
			// init AudioTrack again.
			mAudioFormat = mAudioCodec.getOutputFormat();
			initAudioTrack();
		} else if (res == MediaCodec.INFO_TRY_AGAIN_LATER) {
//			Log.d(TAG, "Audio: INFO_TRY_AGAIN_LATER");
//			
//			if (getState() == PlayState.STARTED) {
//				postAudioReadEvent(0 /* milliseconds */);
//			}
		}
		
		if (isPlaying() || mSeeking) {
			postDecodeAudioEvent();
		}
	}
	
	private void postDecodeVideoEvent() {
//		Log.d(TAG, "postVideoReadEvent");
		postDecodeVideoEvent(0 /* delayMillis */);
	}
	
	private void postDecodeVideoEvent(int delayMillis) {
		mMediaEventHandler.sendEmptyMessageDelayed(EVENT_DECODE_VIDEO, delayMillis);
	}
	
	private void onDecodeVideoEvent() {
//		Log.d(TAG, "onVideoReadEvent");
		
		int delayTimeUs = 0 /* milliseconds */;
		
		MediaCodec.BufferInfo info = mVideoBufferInfo;
		int res = mVideoCodec.dequeueOutputBuffer(info, TIMEOUT);
//		Log.d(TAG, "[OnVideoReadEvent] res: " + res);
		
		if (res > 0) {
//			Log.d(TAG, "[DecodeVideoBuffer] presentationTimeUs: " + info.presentationTimeUs + "; flags: " + info.flags);
			
			int outputBufIndex = res;
			
			boolean release = true;
			boolean render = true;
			
			long presentationTimeUs = info.presentationTimeUs;
			long currentTimeUs = getCurrentTimeUs();
			
			if (mSeeking) {
				// FIXME: Some first frame's presentationTime is larger than seekingTime 
				if (Math.abs(presentationTimeUs - getSeekingTimeUs()) < 10000000 /* 10 seconds */) {
					mSeeking = false;
					render = true;
				} else {
					render = false;
				}
			}
			
			if (currentTimeUs > 0) {
//				if (presentationTimeUs - currentTimeUs > 10000 /* microseconds */) {
//					Log.d(TAG, "Fast by " + (presentationTimeUs - currentTimeUs) + "(us).");
//					if (!mSeeking) {
//						release = false;
//						delayTimeUs = 1 /* milliseconds */;
//					}
//				} 
//				else if (currentTimeUs - presentationTimeUs > 40000 /* microseconds */) {
//					Log.v(TAG, "Late by " + (currentTimeUs - presentationTimeUs) + "(us).");
//					render = false;
//				}
//				else {
//					if (mSeeking) {
//						mSeeking = false;
//						Log.d(TAG, "Seeking Complete.");
//					}
//				}
			}
			
			if (release) {
				mVideoCodec.releaseOutputBuffer(outputBufIndex, render /* render */);
				if ((info.flags & MediaCodec.BUFFER_FLAG_END_OF_STREAM) != 0) {
//					Log.v(TAG, "saw Output EOS.");
					mSawOutputEOS = true;
					postPlaybackCompletionEvent();
				}
			}
			
		} else if (res == MediaCodec.INFO_OUTPUT_BUFFERS_CHANGED) {
//			Log.d(TAG, "Video: INFO_OUTPUT_BUFFERS_CHANGED");
			
		} else if (res == MediaCodec.INFO_OUTPUT_FORMAT_CHANGED) {
//			Log.d(TAG, "Video: INFO_OUTPUT_FORMAT_CHANGED");

			mVideoFormat = mVideoCodec.getOutputFormat();
			
			int width = mVideoFormat.getInteger(MediaFormat.KEY_WIDTH);
			int height = mVideoFormat.getInteger(MediaFormat.KEY_HEIGHT);
			
			postSetVideoSizeEvent(width, height);
		} else if (res == MediaCodec.INFO_TRY_AGAIN_LATER) {
//			Log.d(TAG, "Video: INFO_OUTPUT_FORMAT_CHANGED");
//			
//			if (getState() == PlayState.STARTED) {
//				postVideoReadEvent(0 /* milliseconds */);
//			}
		}
		
		if (isPlaying() || mSeeking) {
			postDecodeVideoEvent(delayTimeUs /* milliseconds */);
		}
	}
	
	private void postBufferingUpdateCheckEvent() {
//		Log.d(TAG, "postCheckBufferEvent");
		if (getState() == PlayState.PLAYBACK_COMPLETED) {
			return;
		}
		
		Message msg = mMediaEventHandler.obtainMessage(EVENT_BUFFERING_UPDATE_CHECK);
		mMediaEventHandler.sendMessageDelayed(msg, 1000 /* milliseconds */);
	}
	
	private void removeBufferingUpdateCheckEvent() {
		mMediaEventHandler.removeMessages(EVENT_BUFFERING_UPDATE_CHECK);
	}
	
	private void onBufferingUpdateCheckEvent() {
		// Make sure only one CheckBuffer Event in MessageQueue.
		removeBufferingUpdateCheckEvent();
		
		postBufferingUpdateEvent();
		
		postBufferingUpdateCheckEvent();
	}
	
	private void postBufferingUpdateEvent() {
		long currentTimeUs = getCurrentTimeUs();
		long cachedDuration = mExtractor.getCachedDuration();
		
		cachedDuration = cachedDuration > 0 ? cachedDuration : 0;
		
		double percentage = 0;
		
		if (mDuration > 0) {
			percentage = ((double)(currentTimeUs + cachedDuration) * 100) / mDuration;
			percentage = percentage > 100 ? 100 : Math.ceil(percentage);
		} else {
			percentage = 0;
		}
		
		Message msg = mEventHandler.obtainMessage(MEDIA_BUFFERING_UPDATE);
		msg.arg1 = (int)percentage;
		msg.sendToTarget();
	}
	
	private void postBufferingStartCheckEvent() {
		mMediaEventHandler.sendEmptyMessage(EVENT_BUFFERING_START_CHECK);
	}
	
	private void onBufferingStartCheckEvent() {
		if (!mIsOnline) {
			return;
		}
		
		long cachedDuration = mExtractor.getCachedDuration();
		
//		Log.d(TAG, "[BufferingStartCheck] cachedDuration: " + cachedDuration);
		
		if (cachedDuration <= 0 /* no cached duration */  && !mExtractor.hasCachedReachedEndOfStream()) {
			pause_l();
			
			if (!isBuffering()) {
				Message msg = mEventHandler.obtainMessage(MEDIA_INFO);
				msg.arg1 = MEDIA_INFO_BUFFERING_START; // extra
				msg.sendToTarget();				
			}
			
			setBufferingStatus(true);
		}
		
		if (isBuffering()) {
			postBufferingEndCheckEvent();
		}		
	}
	
	private void postBufferingEndCheckEvent() {
//		Log.d(TAG, "postBufferingEndCheckEvent");
		postBufferingEndCheckEvent(0 /* delayMillis */);
	}
	
	private void postBufferingEndCheckEvent(long delayMillis) {
//		Log.d(TAG, "postBufferingEndCheckEvent");
		mMediaEventHandler.sendEmptyMessageDelayed(EVENT_BUFFERING_END_CHECK, delayMillis);
	}
	
	private void onBufferingEndCheckEvent() {
		if (!mIsOnline) {
			return;
		}
		
		long cachedDuration = mExtractor.getCachedDuration();
		
//		Log.d(TAG, "[BufferingEndCheck] cachedDuration: " + cachedDuration);
		
		if (cachedDuration > 0 /* have cached duration */) {
			play_l();
			
			if (isBuffering()) {
				Message msg = mEventHandler.obtainMessage(MEDIA_INFO);
				msg.arg1 = MEDIA_INFO_BUFFERING_END; // extra
				msg.sendToTarget();
			}
			
			setBufferingStatus(false);
		}
		
		if (isBuffering()) {
			postBufferingEndCheckEvent(100 /* milliseconds */);
		}
	}
	
	private /* synchronized */ boolean isBuffering() {
		return mIsBuffering;
	}
	
	private /* synchronized */ void setBufferingStatus(boolean status) {
		mIsBuffering = status;
	}
	
	private void setSeekingTimeUs(long seekingTimeUs) {
		mSeekingTimeUs = seekingTimeUs;
	}
	
	private long getSeekingTimeUs() {
		return mSeekingTimeUs;
	}
	
	private /* synchronized */ void setCurrentTimeUs(long timeUs) {
		mCurrentTimeUs = timeUs;
	}
	
	private /* synchronized */ long getCurrentTimeUs() {
		return mCurrentTimeUs;
	}
	
	private void postPlaybackCompletionEvent() {
		setState(PlayState.PLAYBACK_COMPLETED);
		mEventHandler.sendEmptyMessage(MEDIA_PLAYBACK_COMPLETE);
	}

	@Override
	public void stop() {
//		Log.d(TAG, "stop() 1");
		
		if (getState() == PlayState.STOPPED) {
//			Log.d(TAG, "Already Stopped.");
			return;
		}
		
		stayAwake(false);
		
		mMediaEventHandler.postAtFrontOfQueue(new Runnable() {
			@Override
			public void run() {
//				Log.d(TAG, "stop() 2");
				
				removeAllEvents();
				
				mLock.lock();
				setState(PlayState.STOPPED);
				notStopped.signalAll();
				mLock.unlock();
			}
		});
		
		mLock.lock();
		try {
			while (getState() != PlayState.STOPPED) {
				notStopped.await(1, TimeUnit.SECONDS);
			}
		} catch (InterruptedException e) {
			LogUtils.error("InterruptedException", e);
		    
		} finally {
			mLock.unlock();
		}
	}

	@Override
	public void pause() {
//		Log.d(TAG, "pause()");
		
		if (getState() == PlayState.PAUSED) {
//			Log.d(TAG, "Already Paused.");
			return;
		}
		
		stayAwake(false);
		
		setState(PlayState.PAUSED);
		pause_l();
	}
	
	private void pause_l() {
		removeAllEvents();
		postBufferingUpdateCheckEvent();
	}
	
	@Deprecated
	@Override
	public boolean suspend() {
//		Log.d(TAG, "suspend()");
		return true;
	}

	@Deprecated
	@Override
	public boolean resume() {
//		Log.d(TAG, "resume()");
		return true;
	}
	
	private void removeAllEvents() {
//		Log.d(TAG, "removeAllEvents() Start!!!");
		
		mMediaEventHandler.removeCallbacksAndMessages(null);
		mEventHandler.removeCallbacksAndMessages(null);
		
//		Log.d(TAG, "removeAllEvents() End!!!");
	}

	@Override
	public void seekTo(int seekingTime /* milliseconds */) throws IllegalStateException {
	    LogUtils.info("seekTo:" + seekingTime);
		
		PlayState state = getState();
		if (state != PlayState.STARTED && 
			state != PlayState.PAUSED && 
			state != PlayState.PREPARED) {
			
		    LogUtils.error("SeekTo Exception!!!");
			throw new IllegalStateException("Error State: " + state);
		}
		
		long seekingTimeUs = 
			Utils.convertTime(
					seekingTime /* srcDuration */,
					TimeUnit.MILLISECONDS /* from */, 
					TimeUnit.MICROSECONDS /* to */)
					/* milliseconds to microseconds */;
		
		mSeeking = true;
		setSeekingTimeUs(seekingTimeUs);
		
		Message msg = mMediaEventHandler.obtainMessage(EVENT_SEEKTO);
		mMediaEventHandler.sendMessageAtFrontOfQueue(msg);
	}
	
	private void onSeekToEvent() {
//		Log.d(TAG, "onSeekToEvent()");
		
		pause_l();
		
		mExtractor.seekTo(getSeekingTimeUs(), MediaExtractor.SEEK_TO_CLOSEST_SYNC);
		
		play_l();
		
		postSeekCompletionEvent();
	}
	
	private void postSeekCompletionEvent() {
		mEventHandler.sendEmptyMessage(MEDIA_SEEK_COMPLETE);
	}

	@Override
	public void release() {
		mLock.lock();
//		Log.d(TAG, "release()");
		
		stayAwake(false);
		
		release_l();
		mLock.unlock();
	}
	
	private void release_l() {
//		Log.d(TAG, "release_l()");
		
		mMediaEventHandler.postAtFrontOfQueue(new Runnable() {
			
			@Override
			public void run() {
				removeAllEvents();
				
				// release Audio Codec
				if (mAudioCodec != null) {
					mAudioCodec.flush();
					mAudioCodec.stop();
					mAudioCodec.release();
					mAudioCodec = null;
				}
				
				// release Video Codec
				if (mVideoCodec != null) {
					mVideoCodec.flush();
					mVideoCodec.stop();
					mVideoCodec.release();
					mVideoCodec = null;
				}
				
				// release AudioTrack
				if (mAudioTrack != null) {
					mAudioTrack.flush();
					mAudioTrack.stop();
					mAudioTrack.release();
					mAudioTrack = null;
				}
				
				// release MediaExtractor
				if (mExtractor != null) {
					mExtractor.release();
					mExtractor = null;
				}
				
				setState(PlayState.END);
			}
		});
	}
	
	@Override
	public final void reset() {
		
	}

	@Override
	public int getVideoWidth() {
		return mVideoWidth;
	}

	@Override
	public int getVideoHeight() {
		return mVideoHeight;
	}

	@Override
	public int getCurrentPosition() {
		long currentPosition = 0L;
		if (!mSeeking) {
			currentPosition = getCurrentTimeUs();
		} else {
			currentPosition = getSeekingTimeUs();
		}
		
		return (int)(Utils.convertTime(
				currentPosition /* srcDuration */,
				TimeUnit.MICROSECONDS /* from */,
				TimeUnit.MILLISECONDS /* to */)
				/* microseconds to milliseconds */);
	}

	@Override
	public int getDuration() {
		return (int)(Utils.convertTime(
				mDuration /* srcDuration */,
				TimeUnit.MICROSECONDS /* from */,
				TimeUnit.MILLISECONDS /* to */)
				/* microseconds to milliseconds */);
	}

	@Override
	public int flags() throws IllegalStateException {
		
		if (UrlUtil.isLivePlayUrl(mUri)) { 
			
//			Log.d(TAG, "Can't seek/pause.");
			return 0;
		} else {
			
//			Log.d(TAG, "Can seek/pause.");
			return CAN_PAUSE | CAN_SEEK | CAN_SEEK_BACKWARD | CAN_SEEK_FORWARD;
		}
	}
	
	@Override
	public DecodeMode getDecodeMode() {
		return DecodeMode.HW_OMX;
	}
}
