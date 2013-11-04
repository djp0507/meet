package android.pplive.media.player;

import android.graphics.Bitmap;
import android.view.Surface;

public class MeetPlayerHelper {
	
	private MeetPlayerHelper() {}
	
	public static boolean checkCompatibility(int checkWhat, Surface surface) {
		return NativeMediaPlayer.checkCompatibility(checkWhat, surface);
	}
	
	public static boolean checkCompatibility(Surface surface) {
		return NuMediaPlayerTest.checkCompatibility(surface);
	}
	
	public static Bitmap createVideoThumbnail(String mediaFilePath, int kind) {
		return NativeMediaPlayer.createVideoThumbnail(mediaFilePath, kind);
	}
	
	public static int checkSoftwareDecodeLevel() {
		return NativeMediaPlayer.checkSoftwareDecodeLevel();
	}
	
	public static String getBestCodec(String appPath) {
		return NativeMediaPlayer.getBestCodec(appPath);
	}
	
	public static int getCpuArchNumber() {
		return NativeMediaPlayer.getCpuArchNumber();
	}
	
	public static MediaInfo getMediaDetailInfo(String mediaFilePath) {
		return NativeMediaPlayer.getMediaDetailInfo(mediaFilePath);
	}
	
	public static MediaInfo getMediaInfo(String mediaFilePath) {
		return NativeMediaPlayer.getMediaInfo(mediaFilePath);
	}
	
	
}
