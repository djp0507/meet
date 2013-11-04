package com.pplive.meetplayer.util;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

import org.apache.http.HttpEntity;
import org.apache.http.HttpResponse;
import org.apache.http.HttpStatus;
import org.apache.http.client.ClientProtocolException;
import org.apache.http.client.HttpClient;
import org.apache.http.client.methods.HttpGet;
import org.apache.http.impl.client.DefaultHttpClient;

import android.util.Log;

/**
 * @deprecated Use {@link DownloadAsyncTask}. 
 */
@Deprecated
public class DownloadHelper {
	private static final String TAG = "DownloadHelper";

	/**
	 * @deprecated Use {@link DownloadAsyncTask#execute(String...)}.
	 */
	@Deprecated
	public static boolean download(String url, String path) {
		Log.d(TAG, url);
		Log.d(TAG, path);

		boolean ret = false;

		OutputStream os = null;
		InputStream is = null;

		HttpClient client = null;
		HttpGet httpGet = null;

		File file = new File(path);
		if (file.getParentFile().exists() || 
				!file.getParentFile().exists() && 
				file.getParentFile().mkdirs()) {
			try {
				// File Stream
				os = new FileOutputStream(file);

				// Http
				client = new DefaultHttpClient();

				httpGet = new HttpGet(url);
				HttpResponse rep = client.execute(httpGet);

				int status = rep.getStatusLine().getStatusCode();

				if (status >= HttpStatus.SC_OK
						&& status <= HttpStatus.SC_MULTIPLE_CHOICES) {
					HttpEntity entity = rep.getEntity();
					is = entity.getContent();

					byte[] buf = new byte[1024 * 1024]; // buffer size: 1MB;
					int readSize = 0;
					while ((readSize = is.read(buf)) > 0) {
						os.write(buf, 0, readSize);
					}

					ret = true;
				} else {
					Log.e(TAG, rep.getStatusLine().toString());
				}
			} catch (FileNotFoundException e) {
				Log.e(TAG, e.toString());
			} catch (ClientProtocolException e) {
				Log.e(TAG, e.toString());
			} catch (IOException e) {
				Log.e(TAG, e.toString());
			} finally {
				try {
					if (os != null) {
						os.close();
						os = null;
					}

					if (is != null) {
						is.close();
						os = null;
					}
				} catch (IOException e) {
					Log.w(TAG, e.toString());
				}

				if (httpGet != null) {
					httpGet.abort();
					httpGet = null;
				}
				
				// Clean
				if (!ret && file != null) {
					file.delete();
				}
				
				if (ret) {
					Log.d(TAG, "Download file success!!!");
				} else {
					Log.d(TAG, "Download file failed!!!");
				}
				
			}
		}

		return ret;
	}
}
