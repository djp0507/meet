package com.pplive.meetplayer.ui;

import java.io.File;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Locale;
import java.util.Map;
import java.util.regex.Pattern;

import android.app.AlertDialog;
import android.app.Dialog;
import android.app.ListActivity;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.IntentFilter;
import android.database.Cursor;
import android.graphics.Bitmap;
import android.net.Uri;
import android.os.AsyncTask;
import android.os.Bundle;
import android.pplive.media.MeetSDK;
import android.pplive.media.provider.MediaMetadata;
import android.pplive.media.scan.MediaScannerService;
import android.provider.MediaStore;
import android.provider.MediaStore.Video.Thumbnails;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.view.Window;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.BaseAdapter;
import android.widget.EditText;
import android.widget.ImageView;
import android.widget.TextView;
import android.widget.Toast;

import com.pplive.meetplayer.R;

public class MainActivity extends ListActivity {

	private static final String TAG = "ppmedia/MainActivity";
	
	private static final Pattern sRegPrefix;
	
	static {
		sRegPrefix = Pattern.compile("^(http|https|pplive|pplive2|ppvod|ppfile):.*", Pattern.CASE_INSENSITIVE);
	}
	
	private BroadcastReceiver mScannerReceiver = null;
	
	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		Log.d(TAG, "onCreate()");
		
		requestWindowFeature(Window.FEATURE_CUSTOM_TITLE);
		setContentView(R.layout.activity_main);

		getWindow().setFeatureInt(Window.FEATURE_CUSTOM_TITLE, R.layout.activity_main_title_bar);

		findViewById(R.id.text_refresh).setOnClickListener(new OnClickListener() {

			@Override
			public void onClick(View v) {
				sendBroadcast(new Intent(MediaScannerService.ACTION_MEDIA_MOUNTED));
			}
		});
		
		findViewById(R.id.text_search).setOnClickListener(new OnClickListener() {
			
			@Override
			public void onClick(View v) {
				AlertDialog.Builder builder = new AlertDialog.Builder(MainActivity.this);
				builder.setTitle("Search...");
				
				final EditText editView = new EditText(MainActivity.this);
				editView.setSingleLine(true);
				
				builder.setView(editView);
				builder.setPositiveButton("Ok", new DialogInterface.OnClickListener() {
					
					@Override
					public void onClick(DialogInterface dialog, int which) {
						if (which == Dialog.BUTTON_POSITIVE) {
							String keyWord = editView.getText().toString();
							initVideoList(keyWord);
						}
					}
				});
				builder.setNegativeButton("Cancel", null);
				builder.show();
			}
		});
		
		initVideoList();
	}
	
	@Override
	protected void onStart() {
		super.onStart();
		
		// Register receivers
		mScannerReceiver = new BroadcastReceiver() {
			@Override
			public void onReceive(Context context, Intent intent) {
				Log.d(TAG, "Action: " + intent.getAction());
				
				initVideoList();
			}
		};
		
		IntentFilter filter = new IntentFilter();
		filter.addAction(MediaScannerService.ACTION_MEDIA_SCANNER_FINISHED);
		
		registerReceiver(mScannerReceiver, filter);
	}
	
	@Override
	protected void onStop() {
		super.onStop();
		
		if (mScannerReceiver != null) {
			unregisterReceiver(mScannerReceiver);
			mScannerReceiver = null;
		}
	}
	
	private void initVideoList() {
		initVideoList(null);
	}
	
	private void initVideoList(final String keyWord) {
		final List<VideoInfo> videoItems = new ArrayList<MainActivity.VideoInfo>();
		
		AsyncTask<Void, VideoInfo, Boolean> initVideoListTask = new AsyncTask<Void, VideoInfo, Boolean>() {
			
			private List<VideoInfo> mVideoItems = null;
			private VideoAdapter mVideoAdapter = null;
			
			@Override
			protected void onPreExecute() {
				mVideoItems = videoItems;
				
//				mVideoItems.add(new VideoInfo("SDK(Http) 1", "http://127.0.0.1:9006/play.mp4?type=ppvod&playlink=73fcc6e8de54533a57647e2390d13ea2.mp4%3Fbwtype%3D0%26type%3Dphone.android%26sv%3D2.2.1%26bighead%3Dtrue"));
//				mVideoItems.add(new VideoInfo("SDK(Http) 2", "http://127.0.0.1:9006/play.mp4?type=ppvod&playlink=%255B400k%255D%25CC%25EC%25D0%25D0%25D5%25DF.mp4%3Fbwtype%3D0%26type%3Dphone.android%26sv%3D2.2.3%26bighead%3Dtrue"));
//				mVideoItems.add(new VideoInfo("CDN(Http)", "http://v.iask.com/v_play_ipad.php?vid=99264895"));
//				mVideoItems.add(new VideoInfo("SDK(m3u8) 流畅", "http://127.0.0.1:9006/record.m3u8?type=ppvod&playlink=fdec02f881b182e619a905a69935bef3.mp4%3Fbwtype%3D0%26type%3Dpad.android%26sv%3D1.2.5&mux.M3U8.segment_duration=5"));
//				mVideoItems.add(new VideoInfo("SDK(m3u8) 高清", "http://127.0.0.1:9006/record.m3u8?type=ppvod&playlink=%255B400k%255D%25D5%25D4%25CA%25CF%25B9%25C2%25B6%25F9%2528%25C0%25B6%25B9%25E2%2529_V2.mp4%3Fbwtype%3D0%26type%3Dpad.android%26sv%3D1.2.5&mux.M3U8.segment_duration=5"));
//				mVideoItems.add(new VideoInfo("SDK(m3u8) 超清", "http://127.0.0.1:9006/record.m3u8?type=ppvod&playlink=5df8e013737c0ed7cd185bc48ebb5f23.mp4%3Fbwtype%3D0%26type%3Dpad.android%26sv%3D1.2.5&mux.M3U8.segment_duration=5"));
//				mVideoItems.add(new VideoInfo("SDK(m3u8) 蓝光", "http://127.0.0.1:9006/record.m3u8?type=ppvod&playlink=ec079c528f2746e3ef4d7c88dbe0193f.mp4%3Fbwtype%3D0%26type%3Dpad.android%26sv%3D1.2.5&mux.M3U8.segment_duration=5"));
//				mVideoItems.add(new VideoInfo("PPVOD(BaseLine)", "ppvod:///%5Bmobile%5D%C8%BC%C9%D5%B5%AF%28%C0%B6%B9%E2%29.mp4"));
//				mVideoItems.add(new VideoInfo("PPVOD(High)", "ppvod:///%C7%D4%CC%FD%B7%E7%D4%C62%28%C0%B6%B9%E2%29.mp4?t=4"));
//				mVideoItems.add(new VideoInfo("PPLive2(BaseLine)", "pplive2:///473af848f27b47cdb401dd193288b24c-5-382?bwtype=0&type=phone.android&sv=2.2.6"));
//				mVideoItems.add(new VideoInfo("PPLive2(High)", "pplive2:///887d4d23ee894ceb8f5f457fa24f3e97-5-400?bwtype=0&type=phone.android&sv=2.2.6"));
				
				mVideoItems.add(new VideoInfo("Youku mp4", "http://59.63.171.47/youku/6771ADDEF6A418116214A45395/03000803005264193E4485061A1866D4F5CC5C-75B4-5894-4664-359C8B77CED9.mp4"));
				
				mVideoAdapter = new VideoAdapter(MainActivity.this, videoItems);
				MainActivity.this.getListView().setAdapter(mVideoAdapter);
				MainActivity.this.getListView().setOnItemClickListener(new OnItemClickListener() {

					@Override
					public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
						VideoAdapter adapter = (VideoAdapter) parent.getAdapter();
						VideoInfo info = (VideoInfo) adapter.getItem(position);
						
						Intent intent = new Intent(MainActivity.this, VideoPlayerActivity.class);
						
						Uri uri = null;
						Log.d(TAG, "info.mPath: " + info.mPath);
						if (isServerSideUri(info.mPath)) {
							uri = Uri.parse(info.mPath);
							Log.d(TAG, "Is ServerSide");
						} else {
							Log.d(TAG, "Not ServerSide");
							uri = Uri.fromFile(new File(info.mPath));
						}
						
						Log.d(TAG, "uri: " + uri.toString());
						
						intent.setData(uri);
						startActivity(intent);
					}
					
					public boolean isServerSideUri(String uri) {
						
						return null == uri ?  false : sRegPrefix.matcher(uri.trim()).matches();
					}
				});
			}
			
			@Override
			protected void onProgressUpdate(VideoInfo... values) {
				VideoInfo info = values[0];
				
				mVideoItems.add(info);
				mVideoAdapter.notifyDataSetChanged();
			}
			
			@Override
			protected Boolean doInBackground(Void... params) {
				String selection = null;
				String[] selectionArgs = null;
				
				if (keyWord != null) {
					selection = MediaStore.Video.Media.DISPLAY_NAME + " LIKE ?";
					selectionArgs = new String[]{ keyWord+"%" };
				}
				
				Cursor cursor = MainActivity.this.managedQuery(MediaMetadata.Video.CONTENT_URI, null, selection, selectionArgs, null);
				if (cursor != null) {
					final int displayNameId = cursor.getColumnIndexOrThrow(MediaMetadata.Video.COLUMN_TITLE);
					final int dataId = cursor.getColumnIndexOrThrow(MediaMetadata.Video.COLUMN_DATA);
					final int durationId = cursor.getColumnIndexOrThrow(MediaMetadata.Video.COLUMN_DURATION);
					final int sizeId = cursor.getColumnIndexOrThrow(MediaMetadata.Video.COLUMN_SIZE);
					
					for (cursor.moveToFirst(); !cursor.isAfterLast(); cursor.moveToNext()) {
						String title = cursor.getString(displayNameId);
						long duration = cursor.getLong(durationId);
						long size = cursor.getLong(sizeId);
						String path = cursor.getString(dataId);
						
						Log.d(TAG, "title: " + title);
						Log.d(TAG, "duration: " + duration);
						
						publishProgress(new VideoInfo(title, duration, size, path));
					}
				}
				
				return true;
			}
			
			@Override
			protected void onPostExecute(Boolean result) {
				Toast.makeText(getApplicationContext(), "Refresh", Toast.LENGTH_SHORT).show();
			}
			
		};
		
		initVideoListTask.execute();
	}
	
	static class VideoInfo {
		private String mTitle;
		private long mDuration; // millisecond  
		private float mSize; // byte
		private String mPath;
		
		VideoInfo() {
			this("", "");
		}
		
		VideoInfo(String title, String filePath) {
			this(title, 0, 0, filePath);
		}
		
		VideoInfo(String title, long duration, float size, String filePath) {
			mTitle = title;
			mDuration = duration;
			mSize = size;
			mPath = filePath;
		}
		
		private static String timeToString(long timeMs) {
	        long totalSeconds = timeMs / 1000;

	        long seconds = totalSeconds % 60;
	        long minutes = (totalSeconds / 60) % 60;
	        long hours   = totalSeconds / 3600;
	        
	        String.format(Locale.US, "%d:%02d:%2d", hours, minutes, seconds);
	        
	        return (hours > 0) ?
	        		String.format(Locale.US, "%d:%02d:%2d", hours, minutes, seconds) :
	        		String.format(Locale.US, "%02d:%02d", minutes, seconds);
	    }
		
		public String getTitle() {
			return mTitle;
		}
		
		public String getDurationString() {
			return timeToString(mDuration);
		}
		
		public float getSizeMB() {
			return mSize / (1024 * 1024);
		}
		
		public String getPath() {
			return mPath;
		}
		
		@Override
		public boolean equals(Object obj) {
			if (obj == null || !(obj instanceof VideoInfo)) {
				return false;
			}
			
			VideoInfo info = (VideoInfo)obj;
			
			return (this.mTitle == null ? info.mTitle == null : this.mTitle.equals(info.mTitle)) && 
					(this.mPath == null ? info.mPath == null : this.mPath.equals(info.mPath)) &&
					(this.mDuration == info.mDuration) && (this.mSize == info.mSize);
		}
		
		@Override
		public int hashCode() {
			int result = 17;
			
			result = 37*result + (this.mTitle == null ? 0 : mTitle.hashCode());
			result = 37*result + (int)mDuration;
			result = 37*result + (this.mPath == null ? 0 : mPath.hashCode());
			result = 37*result + Float.floatToIntBits(mSize);
			
			return super.hashCode();
		}
	}

	static class VideoAdapter extends BaseAdapter {
		
		private Context mContext = null;
		private List<VideoInfo> mVideoItems = null;
		private Map<String, View> mCachedVideoInfos = null;
		
		private final String mVideoDurationFormat;
		private final String mVideoSizeFormat;
		

		public VideoAdapter(Context context, List<VideoInfo> videoItems) {
			mContext = context;
			mVideoItems = videoItems;
			
			mCachedVideoInfos = new HashMap<String, View>(mVideoItems.size());

			mVideoDurationFormat = mContext.getString(R.string.format_video_duration);
			mVideoSizeFormat = mContext.getString(R.string.format_video_size);
		}

		@Override
		public int getCount() {
			return mVideoItems.size();
		}

		@Override
		public Object getItem(int position) {
			return mVideoItems.get(position);
		}

		@Override
		public long getItemId(int position) {
			return position;
		}

		@Override
		public View getView(final int position, View convertView, ViewGroup parent) {
			final VideoInfo info = mVideoItems.get(position);

//			Log.d(TAG, "Position: " + position + "; File path: " + info.mFilePath);
			
			convertView = mCachedVideoInfos.get(info.mPath);
			if (convertView == null) {
				convertView = LayoutInflater.from(mContext).inflate(R.layout.activity_main_list_item, null);
				
				final ImageView videoThumb = (ImageView) convertView.findViewById(R.id.video_thumb);
				AsyncTask<Void, Integer, Bitmap> task = new AsyncTask<Void, Integer, Bitmap> () {

					@Override
					protected Bitmap doInBackground(Void... params) {
						Bitmap bitmap = null;
						
						if (videoThumb != null) {
							MeetSDK.AppRootDir = "/data/data/" + mContext.getPackageName() + "/";
							bitmap = MeetSDK.createVideoThumbnail(info.mPath, Thumbnails.MICRO_KIND);
						}
						
						return bitmap;
					}
					
					@Override
					protected void onPostExecute(Bitmap bitmap) {
						if (videoThumb!= null && bitmap != null) {
							videoThumb.setImageBitmap(bitmap);
						}
					}
				};
				
				task.execute();
				
				TextView videoTitle = (TextView) convertView.findViewById(R.id.video_title);
				videoTitle.setText(info.getTitle());
				
				TextView videoDuration = (TextView) convertView.findViewById(R.id.video_duration);
				videoDuration.setText(String.format(mVideoDurationFormat, info.getDurationString()));
				
				TextView videoSize = (TextView) convertView.findViewById(R.id.video_size);
				videoSize.setText(String.format(mVideoSizeFormat, info.getSizeMB()));
				
				mCachedVideoInfos.put(info.getPath(), convertView);
			}
			
			return convertView;
		}
	}
}
