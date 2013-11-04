package android.pplive.media.scan;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.pplive.media.util.LogUtils;

public class MediaScannerReceiver extends BroadcastReceiver {
	
	@SuppressWarnings("unused")
	private static final String TAG = "ppmedia/MediaScannerReceiver";

	@Override
	public void onReceive(Context context, Intent intent) {
		String action = intent.getAction();
		
		if (MediaScannerService.ACTION_MEDIA_MOUNTED.equals(action) ||
			MediaScannerService.ACTION_MEDIA_SCANNER_SCAN_FILE.equals(action)) {
			
			LogUtils.debug("ACTION_MEDIA_MOUNTED");
			context.startService(new Intent(context, MediaScannerService.class));
		}
	}
}
