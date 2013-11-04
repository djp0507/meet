package com.pplive.meetplayer.ui;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.content.DialogInterface;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.pplive.media.player.MeetVideoView;
import android.util.Log;
import android.view.GestureDetector;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;
import android.widget.ProgressBar;
import android.widget.TextView;
import android.widget.Toast;

import com.pplive.meetplayer.R;
import com.pplive.meetplayer.util.DownloadAsyncTask;

public class VideoPlayerActivity extends Activity {

	private static final String TAG = "VideoPlayerActivity";

	private Uri mUri = null;
	private MeetVideoView mVideoView = null;
	private ProgressBar mBufferingProgressBar = null;
	
	private boolean mIsBuffering = false;

	private ProgressBar mDownloadProgressBar = null;
	private TextView mProgressTextView = null;
	private Dialog mUpdateDialog = null;
	
	private boolean needUpdate = false;
	
	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		Log.d(TAG, "onCreate()");
		
		Intent intent = getIntent();
		mUri = intent.getData();

		Log.d(TAG,"Step: onCreate 1");
		setContentView(R.layout.activity_video_player);
		Log.d(TAG,"Step: onCreate 2");

		setupUpdater();
		setupPlayer();
		Log.d(TAG,"Step: onCreate 3");
	}

	@Override
	protected void onStart() {
		super.onStart();

		Log.d(TAG, "Step: onStart 1");
		
		if (!needUpdate) {
			playVideo();
		} else {
			mUpdateDialog.show();
		}
		Log.d(TAG, "Step: onStart 2");
	}
	
	@Override
	protected void onResume() {
		super.onResume();

		Log.d(TAG, "Step: onResume 1");

		mVideoView.resume();
		Log.d(TAG, "Step: onResume 2");
	}

	@Override
	protected void onPause() {
		super.onPause();

		Log.d(TAG, "onPause()");

		mVideoView.pause();
	}

	@Override
	protected void onStop() {
		super.onStop();

		Log.d(TAG, "onStop()");

		mVideoView.stopPlayback();
	}
	
	@Override
	public boolean onTouchEvent(MotionEvent event) {
//		Log.d(TAG, "onTouchEvent()");
		
		return mDoubleTapListener.onTouchEvent(event);
	}
	
	@Override
	public boolean onKeyDown(int keyCode, KeyEvent event) {
		
		Log.d(TAG, "keyCode: " + keyCode);
		
		switch (keyCode) {
			case KeyEvent.KEYCODE_BACK:
				
				Log.d(TAG, "Back Down.");
				onBackPressed();
				return true;
			default:
				return super.onKeyDown(keyCode, event);
			}
	}

	private void setupUpdater() {
//		final String codec = MediaPlayer.getBestCodec("/data/data/com.pplive.meetplayer/");
		final String codec = "";
		Log.d(TAG, "codec: " + codec);
		if (null != codec && codec.length() > 0) {
			Log.d(TAG, "codec: " + codec);
			
			mDownloadProgressBar = (ProgressBar) findViewById(R.id.progressbar_download);
			mProgressTextView = (TextView) findViewById(R.id.textview_progress);
			
			needUpdate = true;
			
			AlertDialog.Builder builder = 
				new AlertDialog.Builder(this); 
			
			builder.setMessage("Download");

			builder.setPositiveButton("Yes",
					new DialogInterface.OnClickListener() {

						@Override
						public void onClick(DialogInterface dialog, int which) {
							downloadCodec(codec);
						}
					});

			builder.setNeutralButton("No",
					new DialogInterface.OnClickListener() {

						@Override
						public void onClick(DialogInterface dialog, int which) {
							playVideo();
						}
					});

			mUpdateDialog = builder.create();
		}
	}

	private void setupPlayer() {
		Log.d(TAG,"Step: setupPlayer 1");

		mVideoView = (MeetVideoView) findViewById(R.id.surface_view);
		Log.d(TAG,"Step: setupPlayer 2");
		mVideoView.setVideoURI(mUri);
		Log.d(TAG,"Step: setupPlayer 3");
		mVideoView.setOnCompletionListener(mCompletionListener);
		mVideoView.setOnErrorListener(mErrorListener);
		mVideoView.setOnInfoListener(mInfoListener);
		Log.d(TAG,"Step: setupPlayer 4");
		
		mBufferingProgressBar = (ProgressBar) findViewById(R.id.progressbar_buffering);
	}
	
	private void downloadCodec(String codec) {
		DownloadAsyncTask downloadTask = new DownloadAsyncTask() {

			final String format = getResources().getString(R.string.format_progress);

			@Override
			protected void onPreExecute() {
				mDownloadProgressBar.setVisibility(View.VISIBLE);
				mProgressTextView.setVisibility(View.VISIBLE);
			}

			@Override
			protected void onPostExecute(Boolean result) {
				String msg = null;
				if (result) {
					msg = MSG_DOWNLOAD_SUCCESS;
				} else {
					msg = MSG_DOWNLOAD_FAILED;
				}

				mProgressTextView.setText("");
				mProgressTextView.setVisibility(View.GONE);
				mDownloadProgressBar.setVisibility(View.GONE);

				Toast toast = 
					Toast.makeText(getApplicationContext(), msg, Toast.LENGTH_SHORT);
				toast.show();
				
				needUpdate = false;
				playVideo();
			}

			@Override
			protected void onProgressUpdate(Integer... values) {
				int progress = values[0];
				mProgressTextView.setText(String.format(format, progress));
			}
		};

		String url = "http://172.16.10.126:8080/player/" + codec;
		String path = "/data/data/com.pplive.meetplayer/player/lib/" + codec;
		downloadTask.execute(url, path);
	}

	private void playVideo() {
		Log.d(TAG, "Step: playVideo 1 setVisibility");
		mVideoView.setVisibility(View.VISIBLE);
		Log.d(TAG, "Step: playVideo 2 mVideoView.start");
		mVideoView.start();
		Log.d(TAG, "Step: playVideo 3");
	}

	private void switchDisplayMode() {
		mVideoView.switchDisplayMode();
	}

	private MeetVideoView.OnCompletionListener mCompletionListener = new MeetVideoView.OnCompletionListener() {
		public void onCompletion() {
			Log.d(TAG, "MEDIA_PLAYBACK_COMPLETE");
			mVideoView.stopPlayback();
			finish();
		}
	};

	private MeetVideoView.OnErrorListener mErrorListener = new MeetVideoView.OnErrorListener() {
		public boolean onError(int framework_err, int impl_err) {
			Log.d(TAG, "Error: " + framework_err + "," + impl_err);
			mVideoView.stopPlayback();
			finish();
			return true;
		}
	};
	
	private MeetVideoView.OnInfoListener mInfoListener = new MeetVideoView.OnInfoListener() {
		
		@Override
		public boolean onInfo(int what, int extra) {
			Log.d(TAG, "onInfo: " + what + " " + extra);
			
			if ((what == MeetVideoView.MEDIA_INFO_BUFFERING_START) && !mIsBuffering) {
				mBufferingProgressBar.setVisibility(View.VISIBLE);
				mIsBuffering = true;
			} else if ((what == MeetVideoView.MEDIA_INFO_BUFFERING_END) && mIsBuffering) {
				mBufferingProgressBar.setVisibility(View.GONE);
				mIsBuffering = false;
			}
			
			return true;
		}
	};
	
	private GestureDetector mDoubleTapListener = 
			new GestureDetector(getApplication(), new GestureDetector.SimpleOnGestureListener() {
				
		public boolean onSingleTapConfirmed(MotionEvent e) {
			
			Log.d(TAG, "onSingleTapConfirmed!!!");
			
			return false;
		};
		
		@Override
		public boolean onDoubleTap(MotionEvent event) {
			
			Log.d(TAG, "onDoubleTap!!!");
			switchDisplayMode();
			
			return true;
		}
	});
}
