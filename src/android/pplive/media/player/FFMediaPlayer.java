package android.pplive.media.player;

import android.pplive.media.player.MeetVideoView.DecodeMode;

class FFMediaPlayer extends NativeMediaPlayer {

	FFMediaPlayer() {
		
		super(true);
	}
	
	@Override
	public DecodeMode getDecodeMode() {
		return DecodeMode.SW;
	}
}
