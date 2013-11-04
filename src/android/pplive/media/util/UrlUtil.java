/**
 * Copyright (C) 2013 PPTV
 *
 */
package android.pplive.media.util;

import java.io.UnsupportedEncodingException;
import java.net.URLEncoder;
import java.util.regex.Pattern;

/**
 *
 * @author leoxie
 * @version 2013-2-27
 */
public class UrlUtil {
	
	private static final Pattern sRegNuMediaPlaySupportUrl;
	private static final Pattern sRegOnlinePlayUrl;
	private static final Pattern sRegLivePlayUrl;
	
	static {
		sRegNuMediaPlaySupportUrl = Pattern.compile("^(ppvod\\d*|pplive\\d*|ppfile)://.*", Pattern.CASE_INSENSITIVE);
		sRegOnlinePlayUrl = Pattern.compile("^(ppvod\\d*|pplive\\d*|rtsp|http(s)?)://.*", Pattern.CASE_INSENSITIVE);
		sRegLivePlayUrl = Pattern.compile("^(rtsp|pplive\\d*)://.*", Pattern.CASE_INSENSITIVE);
	}
	
	public static boolean isPPTVPlayUrl(String url) {
		return null == url ? false : sRegNuMediaPlaySupportUrl.matcher(url.trim()).matches();
	}
	
	public static boolean isNuMediaPlayerSupportUrl(String url) {
		
		return null == url ? false : sRegNuMediaPlaySupportUrl.matcher(url.trim()).matches();
	}
	
	public static boolean isOnlinePlayUrl(String url) {
		
		return null == url ? false : sRegOnlinePlayUrl.matcher(url.trim()).matches();
	}
	
	public static boolean isLivePlayUrl(String url) {
		
		return null == url ? false : sRegLivePlayUrl.matcher(url.trim()).matches();
	}
	
	@SuppressWarnings("deprecation")
	public static String encode(String s) {
		String out = null;
		
		try {
			out = URLEncoder.encode(s, "UTF-8");
		} catch (UnsupportedEncodingException e) {
			out = URLEncoder.encode(s);
		} finally {
			
		}
		
		return out;
	}
}
