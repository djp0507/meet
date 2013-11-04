package android.pplive.media.player;

import java.io.IOException;

import android.content.Context;
import android.net.Uri;
import android.pplive.media.player.MeetVideoView.DecodeMode;
import android.view.SurfaceHolder;

interface MediaPlayerInterface {
	
	int flags() throws IllegalStateException;
	
	int getCurrentPosition();
	
	int getDuration();
	
	int getVideoWidth();
	
	int getVideoHeight();
	
	DecodeMode getDecodeMode();
	
	boolean isPlaying();
	
	void setDataSource(Context ctx, Uri uri) throws IOException, IllegalArgumentException, SecurityException, IllegalStateException;	

	void setDataSource(String uri) throws IOException, IllegalArgumentException, IllegalStateException;
	
	void setDisplay(SurfaceHolder sh);
	
	void prepare() throws IOException, IllegalStateException;
	
	void prepareAsync() throws IllegalStateException;
	
	void setOnBufferingUpdateListener(MeetVideoView.OnBufferingUpdateListener listener);
	
	void setOnCompletionListener(MeetVideoView.OnCompletionListener listener);
	
	void setOnErrorListener(MeetVideoView.OnErrorListener listener);
	
	void setOnInfoListener(MeetVideoView.OnInfoListener listener);
	
	void setOnPreparedListener(MeetVideoView.OnPreparedListener listener);
	
	void setOnSeekCompleteListener(MeetVideoView.OnSeekCompleteListener listener);
	
	void setOnVideoSizeChangedListener(MeetVideoView.OnVideoSizeChangedListener listener);
	
	void setScreenOnWhilePlaying(boolean screenOn);
	
	void setAudioStreamType(int streamType);
	
	void setWakeMode(Context context, int mode);
	
	void start() throws IllegalStateException;
	
	void stop();
	
	void pause();
	
	@Deprecated
	boolean suspend();
	
	@Deprecated
	boolean resume();
	
	void seekTo(int seekingTimeUs) throws IllegalStateException;
	
	void release();
	
	void reset();
}
