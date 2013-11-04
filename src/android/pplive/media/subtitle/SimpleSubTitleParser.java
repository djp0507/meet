package android.pplive.media.subtitle;

import android.os.Bundle;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Message;
import android.pplive.media.util.LogUtils;

public class SimpleSubTitleParser implements Handler.Callback, SubTitleParser {
	
	private static boolean sLoadLibSuccess = false;
	
	private static boolean isLoadLibSuccess() {
		return sLoadLibSuccess;
	}
	static {
		try {
			System.loadLibrary("subtitle-jni");
			native_init();
			
			sLoadLibSuccess = true;
		} catch (SecurityException e) {
			
		} catch (UnsatisfiedLinkError e) {
			
		} catch (IllegalStateException e) {
		
		}
	}

	private static native void native_init();
	
	public static final int WHAT_LOAD_SUBTILTE = 9001;
	
	public static final int WHAT_SEEKTO = 9002;
	
	public static final int WHAT_CLOSE = 9003;
	
	private SubTitleSegment mSegment;
	
	private int mNativeContext;
	private Handler mInnerHandler;
	
	public SimpleSubTitleParser() {
		if (!isLoadLibSuccess()) {
			throw new IllegalStateException("Load Lib failed");
		}
		
		native_setup();
		
		HandlerThread ht = new HandlerThread("SampleSubTitleParser");
		ht.start();
		mInnerHandler = new Handler(this);
		mSegment = new SubTitleSegment();
	}
	
	@Override
	public boolean handleMessage(Message msg) {
		boolean ret = true;
		switch (msg.what) {
			case WHAT_LOAD_SUBTILTE: {
				native_loadSubtitle(mFilePath, false /* isMediaFile */);
				break;
			}
			case WHAT_SEEKTO: {
				long msec = msg.getData().getLong("seek_to_msec");
				native_seekTo(msec);
				break;
			}
			case WHAT_CLOSE: {
				native_close();
				break;
			}
			default: {
				ret = false;
				break;
			}
		}
		return ret;
	}
	
	private native void native_setup();

	@Override
	public void close() {
		mInnerHandler.sendEmptyMessage(WHAT_CLOSE);
	}
	
	private native void native_close();

	@Override
	public SubTitleSegment next() {
		boolean ret = native_next(mSegment);
		return ret ? mSegment : null;
	}
	
	private native boolean native_next(SubTitleSegment segment);

	@Override
	public void prepareAsync() {
		mInnerHandler.sendEmptyMessage(WHAT_LOAD_SUBTILTE);
	}
		
	private native void native_loadSubtitle(String filePath, boolean isMediaFile);

	@Override
	public void seekTo(long msec) {
	    LogUtils.info("seekTo: " + msec);
		Message msg = mInnerHandler.obtainMessage(WHAT_SEEKTO);
		Bundle data = new Bundle();
		data.putLong("seek_to_msec", msec);
		msg.setData(data);
		mInnerHandler.sendMessage(msg);
	}
	
	private native void native_seekTo(long msec);

	private String mFilePath = null;
	
	@Override
	public void setDataSource(String filePath) {
	    LogUtils.info(filePath);
		mFilePath = filePath;
	}
	
	private Callback mSubTitleCallback; 
	
	@Override
	public void setOnPreparedListener(Callback callback) {
		mSubTitleCallback = callback;
	}

	private void onPrepared(boolean success, String msg) {
		if (mSubTitleCallback != null) {
			mSubTitleCallback.onPrepared(success, msg);
		} else {
		}
	}
	
	private void onSeekComplete() {
		if (mSubTitleCallback != null) {
			mSubTitleCallback.onSeekComplete();
		}
	}
}
