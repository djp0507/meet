/**
 * Copyright (C) 2012 PPTV
 * 
 */
package android.pplive.media.player;

import java.nio.ByteBuffer;

import android.media.MediaExtractor;
import android.media.MediaFormat;

/**
 * @author leoxie
 * 
 */
class DefaultMediaExtractor implements MediaExtractable {
	
	private MediaExtractor mExtractor;
	
	DefaultMediaExtractor() {
		mExtractor = new MediaExtractor();
	}

	@Override
	public boolean advance() {
		return mExtractor.advance();
	}

	@Override
	public long getCachedDuration() {
		return mExtractor.getCachedDuration();
	}

	@Override
	public int getSampleFlags() {
		return mExtractor.getSampleFlags();
	}

	@Override
	public long getSampleTime() {
		return mExtractor.getSampleTime();
	}

	@Override
	public int getSampleTrackIndex() {
		return mExtractor.getSampleTrackIndex();
	}

	@Override
	public int getTrackCount() {
		return mExtractor.getTrackCount();
	}

	@Override
	public MediaFormat getTrackFormat(int index) {
		return mExtractor.getTrackFormat(index);
	}

	@Override
	public boolean hasCachedReachedEndOfStream() {
		return mExtractor.hasCacheReachedEndOfStream();
	}

	@Override
	public int readSampleData(ByteBuffer byteBuf, int offset) {
		return mExtractor.readSampleData(byteBuf, offset);
	}

	@Override
	public void release() {
		mExtractor.release();
	}

	@Override
	public void seekTo(long timeUs, int mode) {
		mExtractor.seekTo(timeUs, mode);
	}

	@Override
	public void selectTrack(int index) {
		mExtractor.selectTrack(index);
	}

	@Override
	public void setDataSource(String path) {
		mExtractor.setDataSource(path);
	}

	@Override
	public void unselectTrack(int index) {
		mExtractor.unselectTrack(index);
	}

}
