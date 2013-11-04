package android.pplive.media.player;

import java.nio.ByteBuffer;
import java.util.Map;

import android.media.MediaFormat;

class PPMediaExtractor implements MediaExtractable {
	
	private final static String TAG = "ppmedia/PPMediaExtractor";
	
	PPMediaExtractor() {
//		Log.d(TAG, "PPMediaExtractor");
		
		setup();
	}

	@Override
	public native boolean advance();

	@Override
	public native long getCachedDuration();

	@Override
	public native int getSampleFlags();

	@Override
	public native long getSampleTime();

	@Override
	public native int getSampleTrackIndex();

	@Override
	public native int getTrackCount();

	@Override
	public MediaFormat getTrackFormat(int index) {
		return MediaFormatHelper.createMediaFormatFromMap(getTrackFormatNative(index));
	}
	
	private native Map<String, Object> getTrackFormatNative(int index);

	@Override
	public native boolean hasCachedReachedEndOfStream();
	
	@Override
	public native int readSampleData(ByteBuffer byteBuf, int offset);

	@Override
	public native void release();

	@Override
	public native void seekTo(long timeUs, int mode);

	@Override
	public native void selectTrack(int index);

	@Override
	public native void setDataSource(String path);

	@Override
	public native void unselectTrack(int index);
	
	private static native void init();
	private native void setup();

	static {
		System.loadLibrary("ppmediaextractor-jni");
		init();
	};
	
	private int mNativeContext;
}
