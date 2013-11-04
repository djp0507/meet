package android.pplive.media.player;

import java.nio.ByteBuffer;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.locks.Condition;
import java.util.concurrent.locks.Lock;
import java.util.concurrent.locks.ReentrantLock;

import android.media.MediaCodec;
import android.media.MediaFormat;
import android.pplive.media.MeetSDK;
import android.pplive.media.util.LogUtils;
import android.util.Log;
import android.view.Surface;

class NuMediaPlayerTest {

	private static final String TAG = "ppmedia/CompatibilityTestHelper";
	
	private static final String SAMPLE_FILE = "lib/libsample.so";

	private static final long TIMEOUT = 5000l /* microseconds */;

	private String mUri = null;

	private Surface mSurface = null;
	private MediaExtractable mExtractor = null;

	// private int mVideoWidth = 0;
	// private int mVideoHeight = 0;

	// private AudioTrack mAudioTrack = null;

	private MediaFormat mAudioFormat = null;
	private MediaFormat mVideoFormat = null;

	// private MediaFormat mOutputAudioFormat = null;
	// private MediaFormat mOutputVideoFormat = null;

	private int mAudioTrackIndex = -1;
	private int mVideoTrackIndex = -1;
	private int mLastTrackIndex = -1;

	private boolean mHaveAudio = false;
	private boolean mHaveVideo = false;

	private MediaCodec mAudioCodec = null;
	private MediaCodec mVideoCodec = null;

	private MediaCodec.BufferInfo mAudioBufferInfo = null;
	private MediaCodec.BufferInfo mVideoBufferInfo = null;

	// private byte[] mAudioData = null;

	private boolean mSawInputEOS = false;
	private boolean mSawOutputEOS = false;

	private long mDuration = 0L;

	// private boolean mSeeking = false;

	private ExecutorService mExec = null;
	private Lock mLock = null;
	private Condition notEnd = null;

	private void setDataSource(String uri) {
		mUri = uri;

		LogUtils.debug("mUri: " + mUri);

		setDataSource(new DefaultMediaExtractor());
	}

	private void setDataSource(MediaExtractable extractor) {
		mExtractor = extractor;
	}

	private void setDisplay(Surface surface) {
		mSurface = surface;
	}

	private boolean prepare() {

		return initMediaExtractor() && initAudioTrack() && initAudioDecoder() && initVideoDecoder();
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
			MediaFormat format = mExtractor.getTrackFormat(index);

			String mime = format.getString(MediaFormat.KEY_MIME);

			if (mime == null || mime.equals("")) {
				continue;
			} else {
				mime = mime.toLowerCase();
			}

			if (!mHaveAudio && mime.startsWith("audio/")) {
				setAudioFormat(format);
				mAudioTrackIndex = index;
				mHaveAudio = true;
			} else if (!mHaveVideo && mime.startsWith("video/")) {
				setVideoFormat(format);
				mVideoTrackIndex = index;
				mHaveVideo = true;
			} else {
				// unknown mime type;
			    LogUtils.warn("Unknown mime type.");
			}

			if (mHaveAudio && mHaveVideo) {
				break;
			}
		}

		if (!mHaveAudio || !mHaveVideo) {
			return false;
		}

		LogUtils.info("mAudioTrackIndex: " + mAudioTrackIndex);
		LogUtils.info("mVideoTrackIndex: " + mVideoTrackIndex);

		LogUtils.info("Init MediaExtractor Success!!!");
		return true;
	}

	private void setAudioFormat(MediaFormat format) {
	    LogUtils.debug("setAudioFormat()");
		mAudioFormat = format;

		long duration = format.getLong(MediaFormat.KEY_DURATION);
		LogUtils.debug("audio duration: " + duration);
		mDuration = mDuration > duration ? mDuration : duration;
	}

	private void setVideoFormat(MediaFormat format) {
	    LogUtils.debug("setVideoFormat()");
		mVideoFormat = format;

		long duration = format.getLong(MediaFormat.KEY_DURATION);
		LogUtils.info("video duration: " + duration);
		mDuration = mDuration > duration ? mDuration : duration;
	}

	private boolean initAudioTrack() {
		// No need to create AudioTrack;
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

			LogUtils.debug("Init Audio Decoder Success!!!");
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

			LogUtils.debug("Init Video Decoder Success!!!");
			
		} catch (Exception e) {
            LogUtils.error("Exception", e);

			mVideoCodec = null;
			ret = false;
		}

		return ret;
	}

	private boolean startCompatibilityTest() {
		mExec = Executors.newFixedThreadPool(6 /* ReadSample, DecodeAudio, DecodeVideo Threads */);
		mLock = new ReentrantLock();
		notEnd = mLock.newCondition();

		startReadSample();
		startDecodeAudioBuffer();
		startDecodeVideoBuffer();

		mLock.lock();
		try {
			// wait for 5 seconds;
			for (int i = 0; !mSawOutputEOS && i < 5; i++) {
				notEnd.await(1, TimeUnit.SECONDS);
			}
		} catch (InterruptedException e) {
		    LogUtils.error("InterruptedException", e);
		} finally {
			mLock.unlock();
		}

		release();

		// Log.d(TAG, "mSawOutputEOS: " + mSawOutputEOS);

		return mSawOutputEOS;
	}

	private void startReadSample() {
		Runnable task = new Runnable() {

			@Override
			public void run() {
				while (!mSawInputEOS) {
					int trackIndex = mExtractor.getSampleTrackIndex();
					MediaCodec codec = getCodec(trackIndex);

					int inputBufIndex = codec.dequeueInputBuffer(TIMEOUT);

					if (inputBufIndex > 0) {
						ByteBuffer inputBuf = codec.getInputBuffers()[inputBufIndex];
						int sampleSize = mExtractor.readSampleData(inputBuf, 0 /* offset */);

						long presentationTimeUs = 0L;
						int flags = 0;

						if (sampleSize < 0) {
						    LogUtils.verbose("saw output EOS.");
							mSawInputEOS = true;
							sampleSize = 0;
						} else {
							presentationTimeUs = mExtractor.getSampleTime();
							flags = mExtractor.getSampleFlags();
						}
						
						// Log.d(TAG, "[ReadSample] presentationTimeUs: " + presentationTimeUs + "; flags: " + flags);
						
						codec.queueInputBuffer(
								inputBufIndex,
								0 /* offset */,
								sampleSize,
								presentationTimeUs,
								mSawInputEOS ? MediaCodec.BUFFER_FLAG_END_OF_STREAM : flags);

						// postReadEvent(trackIndex);

						if (!mSawInputEOS) {
							mExtractor.advance();
						}
					}
				}
			}
		};

		mExec.submit(task);
	}

	private MediaCodec getCodec(int trackIndex) {
		MediaCodec codec = null;

		if (trackIndex == mAudioTrackIndex) {
			codec = mAudioCodec;
		} else if (trackIndex == mVideoTrackIndex) {
			codec = mVideoCodec;
		} else {
			// Log.d(TAG, "trackIndex: " + trackIndex);
			codec = mLastTrackIndex >= 0 ? getCodec(mLastTrackIndex) : null;
		}

		if (trackIndex >= 0) {
			mLastTrackIndex = trackIndex;
		}

		return codec;
	}

	private void startDecodeAudioBuffer() {
		Runnable task = new Runnable() {

			@Override
			public void run() {
				while (!mSawOutputEOS) {
					MediaCodec.BufferInfo info = mAudioBufferInfo;
					int res = mAudioCodec.dequeueOutputBuffer(info, TIMEOUT);

					if (res > 0) {
						// Log.d(TAG, "[DecodeAudioBuffer] presentationTimeUs: " + info.presentationTimeUs + "; flags: " + info.flags);

						int outputBufIndex = res;

						mAudioCodec.releaseOutputBuffer(outputBufIndex, false /* render */);

						if ((info.flags & MediaCodec.BUFFER_FLAG_END_OF_STREAM) != 0) {
						    LogUtils.verbose("saw output EOS.");
							mSawOutputEOS = true;
						}
					} else if (res == MediaCodec.INFO_OUTPUT_BUFFERS_CHANGED) {
						// Log.d(TAG, "[Auido] INFO_OUTPUT_BUFFERS_CHANGED");

					} else if (res == MediaCodec.INFO_OUTPUT_FORMAT_CHANGED) {
						// Log.d(TAG, "[Auido] INFO_OUTPUT_FORMAT_CHANGED");

					} else if (res == MediaCodec.INFO_TRY_AGAIN_LATER) {
						// Log.d(TAG, "[Auido] INFO_TRY_AGAIN_LATER");
					}
				}
			}
		};

		mExec.submit(task);
	}

	private void startDecodeVideoBuffer() {
		Runnable task = new Runnable() {

			@Override
			public void run() {
				while (!mSawOutputEOS) {
					MediaCodec.BufferInfo info = mVideoBufferInfo;
					int res = mVideoCodec.dequeueOutputBuffer(info, TIMEOUT);

					if (res > 0) {
						// Log.d(TAG, "[DecodeVideoBuffer] presentationTimeUs: " + info.presentationTimeUs + "; flags: " + info.flags);
						
						int outputBufIndex = res;

						mVideoCodec.releaseOutputBuffer(outputBufIndex, false /* render */);
						if ((info.flags & MediaCodec.BUFFER_FLAG_END_OF_STREAM) != 0) {
							Log.v(TAG, "saw Output EOS.");
							mSawOutputEOS = true;
						}
						
					} else if (res == MediaCodec.INFO_OUTPUT_BUFFERS_CHANGED) {
						// Log.d(TAG, "[Video] INFO_OUTPUT_BUFFERS_CHANGED");
						
					} else if (res == MediaCodec.INFO_OUTPUT_FORMAT_CHANGED) {
						// Log.d(TAG, "[Video] INFO_OUTPUT_FORMAT_CHANGED");
						 
					} else if (res == MediaCodec.INFO_TRY_AGAIN_LATER) {
						 // Log.d(TAG, "[Video] INFO_TRY_AGAIN_LATER");
						 
					}
				}
			}
		};

		mExec.submit(task);
	}

	private void release() {
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

		// release MediaExtractor
		if (mExtractor != null) {
			mExtractor.release();
			mExtractor = null;
		}
	}

	private static NuMediaPlayerTest helper = null;

	private static synchronized NuMediaPlayerTest getInstance() {
		if (helper == null) {
			helper = new NuMediaPlayerTest();
		}

		return helper;
	}

	public static boolean checkCompatibility(Surface surface) {
		helper = getInstance();
		helper.setDataSource(MeetSDK.AppRootDir + SAMPLE_FILE);
		helper.setDisplay(surface);

		return helper.prepare() && helper.startCompatibilityTest();
	}
}
