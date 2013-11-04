/**
 * Copyright (C) 2012 PPTV
 * 
 */
package android.pplive.media.player;

import java.nio.ByteBuffer;

import android.media.MediaFormat;

/**
 * @author leoxie
 * 
 * MediaExtractor facilitates extraction of demuxed, 
 * typically encoded, media data from a data source.
 */
interface MediaExtractable {
	
	boolean advance();
	
	long getCachedDuration();
	
//	boolean getSampleCryptoInfo(MediaCodec.CryptoInfo info);
	
	int getSampleFlags();
	
	long getSampleTime();
	
	int getSampleTrackIndex();
	
	int getTrackCount();
	
	MediaFormat getTrackFormat(int index);
	
	boolean hasCachedReachedEndOfStream();
	
	int readSampleData(ByteBuffer byteBuf, int offset);
	
	void release();
	
	void seekTo(long timeUs, int mode);
	
	void selectTrack(int index);
	
	void setDataSource(String path);
	
//	void setDataSource(String path, Map<String, String> headers);
	
//	void setDataSource(FileDescriptor fd);
	
//	void setDataSource(FileDescriptor fd, long offset, long length);
	
//	void setDataSource(Context context, Uri uri, Map<String, String> headers);
	
	void unselectTrack(int index);

}
