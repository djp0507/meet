package android.pplive.media.player;

import java.lang.reflect.Constructor;
import java.util.Map;

import android.media.MediaFormat;
import android.pplive.media.util.LogUtils;

class MediaFormatHelper {
	
	static MediaFormat createMediaFormatFromMap(Map<String, Object> map) {
		MediaFormat format = null;
		
		try {
			Class<?> clazz = Class.forName("android.media.MediaFormat");
			Class<?>[] params = new Class[]{Map.class};
			Constructor<?> constructor = clazz.getDeclaredConstructor(params);
			constructor.setAccessible(true);
			
			format = (MediaFormat) constructor.newInstance(new Object[]{map});
		} catch (Exception e) {
		    LogUtils.error("Exception", e);
		}
		
		return format;
	}
}
