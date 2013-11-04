package android.pplive.media.player;

import android.pplive.media.player.MeetVideoView.DecodeMode;

class OMXMediaPlayer extends NativeMediaPlayer {

	OMXMediaPlayer() {
		
		super(false);
	}
	
	@Override
	public DecodeMode getDecodeMode() {
		return DecodeMode.HW_OMX;
	}
}
