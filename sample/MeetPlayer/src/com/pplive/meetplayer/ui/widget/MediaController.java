package com.pplive.meetplayer.ui.widget;

import java.util.Formatter;
import java.util.Locale;

import android.content.Context;
import android.media.AudioManager;
import android.os.Handler;
import android.os.Message;
import android.view.GestureDetector;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.View;
import android.widget.ImageButton;
import android.widget.ProgressBar;
import android.widget.SeekBar;
import android.widget.SeekBar.OnSeekBarChangeListener;
import android.widget.TextView;

import com.pplive.meetplayer.R;

public class MediaController extends android.widget.MediaController {
	
	@SuppressWarnings("unused")
	private final static String TAG = "MediaController";
	
	private AudioManager mAudioManager;
	private android.widget.MediaController.MediaPlayerControl mPlayer;
	private SeekBar mProgressBar;
	private ProgressBar mVolumeBar;
	private TextView mEndTime;
	private TextView mCurrentTime;
	StringBuilder mFormatBuilder;
    Formatter mFormatter;
    
    private boolean mVolumerDragging = false;
    
    private ImageButton mPlayPauseBtn;
    private ImageButton mFwdBtn;
    private ImageButton mBwdBtn;

	public MediaController(Context context) {
		super(context, true);
	}
	
	@Override
	public void setMediaPlayer(android.widget.MediaController.MediaPlayerControl player) {
		super.setMediaPlayer(player);
		
		mPlayer = player;
	}
	
	protected View makeControllerView() {
		LayoutInflater inflater = (LayoutInflater) getContext().getSystemService(Context.LAYOUT_INFLATER_SERVICE);
		View v = inflater.inflate(R.layout.layout_media_controller, null);
		
		initControllerView(v);
		return v;
	}
	
	private void initControllerView(View v) {
		mPlayPauseBtn = (ImageButton) v.findViewById(R.id.player_play_pause_btn);
		if (mPlayPauseBtn != null) {
			mPlayPauseBtn.requestFocus();
			mPlayPauseBtn.setOnClickListener(mPlayPauseListener);
        }
		
		mBwdBtn = (ImageButton) v.findViewById(R.id.player_bf_btn);
		if (mBwdBtn != null) {
			mBwdBtn.setOnClickListener(mBwdListener);
        }
		
		mFwdBtn = (ImageButton) v.findViewById(R.id.player_ff_btn);
		if (mFwdBtn != null) {
			mFwdBtn.setOnClickListener(mFwdListener);
        }
		
		mProgressBar = (SeekBar) v.findViewById(R.id.mediacontroller_progress);
		if (mProgressBar != null) {
			if (mProgressBar instanceof SeekBar) {
				SeekBar seeker = (SeekBar) mProgressBar;
				seeker.setOnSeekBarChangeListener(mProgressChangeListener);
			}
			mProgressBar.setMax(1000);
		}
		
		mVolumeBar = (ProgressBar) v.findViewById(R.id.mediacontroller_volume);
		if (mVolumeBar != null) {
			if (mVolumeBar instanceof SeekBar) {
				SeekBar seeker = (SeekBar) mVolumeBar;
				seeker.setOnSeekBarChangeListener(mVolumeChangeListener);
			}
			
			mAudioManager = (AudioManager) getContext().getSystemService(Context.AUDIO_SERVICE);
			mVolumeBar.setMax(1000);
		}
		
		mEndTime = (TextView) v.findViewById(R.id.end_time);
		mCurrentTime = (TextView) v.findViewById(R.id.current_time);
		mFormatBuilder = new StringBuilder();
        mFormatter = new Formatter(mFormatBuilder, Locale.getDefault());
        
        this.setOnTouchListener(mTouchListener);
	}
	
	private String stringForTime(int timeMs) {
        int totalSeconds = timeMs / 1000;

        int seconds = totalSeconds % 60;
        int minutes = (totalSeconds / 60) % 60;
        int hours   = totalSeconds / 3600;

        mFormatBuilder.setLength(0);
        if (hours > 0) {
            return mFormatter.format("%d:%02d:%02d", hours, minutes, seconds).toString();
        } else {
            return mFormatter.format("%02d:%02d", minutes, seconds).toString();
        }
    }
	
	private int setProgress() {
		int position = mPlayer.getCurrentPosition();
		return setProgress(position);
	}
	
	private int setProgress(int position) {
		if (mPlayer == null) {
			return 0;
		}
		
		int duration = mPlayer.getDuration();
		
		if (mProgressBar != null) {
			if (duration > 0) {
				long pos = 1000L * position / duration;
				mProgressBar.setProgress((int)pos);
			}
			int percent = mPlayer.getBufferPercentage();
			mProgressBar.setSecondaryProgress(percent * 10);
		}
		
		if (mEndTime != null) {
            mEndTime.setText(stringForTime(duration));
		}
		
        if (mCurrentTime != null) {
            mCurrentTime.setText(stringForTime(position));
        }
		
		return position;
	}
	
	private void setVolume(int volume) {
		mAudioManager.setStreamVolume(AudioManager.STREAM_MUSIC, volume, 0 /* flags */);
	}
	
	private void updateVolumeProgress() {
		int maxVolume = mAudioManager.getStreamMaxVolume(AudioManager.STREAM_MUSIC);
		int volume = mAudioManager.getStreamVolume(AudioManager.STREAM_MUSIC);
		
		if (mVolumeBar != null && !mVolumerDragging) {
			if (maxVolume > 0) {
				long pos = 1000 * volume / maxVolume;
				mVolumeBar.setProgress((int)pos);
			}
		}
	}
	
	private static final int sDefaultTimeout = 5000;
    private static final int FADE_OUT = 1;
    private static final int SHOW_PROGRESS = 2;
	
	@Override
	public void show() {
		this.show(sDefaultTimeout);
	}
	
	@Override
	public void show(int timeout) {
		super.show(timeout);
		
		mHandler.sendEmptyMessage(SHOW_PROGRESS);

        Message msg = mHandler.obtainMessage(FADE_OUT);
        if (timeout != 0) {
            mHandler.removeMessages(FADE_OUT);
            mHandler.sendMessageDelayed(msg, timeout);
        }
	}
	
	@Override
	public void hide() {
		super.hide();
	}
	
	private Handler mHandler = new Handler() {
		@Override
        public void handleMessage(Message msg) {
            int pos;
            switch (msg.what) {
                case FADE_OUT:
                    hide();
                    break;
                case SHOW_PROGRESS:
                	updateVolumeProgress();
                    pos = setProgress();
                    if (isShowing() && mPlayer.isPlaying()) {
                        msg = obtainMessage(SHOW_PROGRESS);
                        sendMessageDelayed(msg, 1000 - (pos % 1000));
                    }
                    break;
            }
        }
	};
	
	private View.OnClickListener mPlayPauseListener = new View.OnClickListener() {
        public void onClick(View v) {
            doPauseResume();
            show(sDefaultTimeout);
        }
    };
    
    private void updatePausePlay() {
        if (mPlayPauseBtn == null)
            return;

        if (mPlayer.isPlaying()) {
            mPlayPauseBtn.setImageResource(R.drawable.player_pause_btn);
        } else {
        	mPlayPauseBtn.setImageResource(R.drawable.player_play_btn);
        }
    }
    
    private View.OnClickListener mBwdListener = new View.OnClickListener() {
        public void onClick(View v) {
            int pos = mPlayer.getCurrentPosition();
            pos -= 5000; // milliseconds
            mPlayer.seekTo(pos);
            setProgress();

            show(sDefaultTimeout);
        }
    };

    private View.OnClickListener mFwdListener = new View.OnClickListener() {
        public void onClick(View v) {
            int pos = mPlayer.getCurrentPosition();
            pos += 15000; // milliseconds
            mPlayer.seekTo(pos);
            setProgress();

            show(sDefaultTimeout);
        }
    };

    private void doPauseResume() {
        if (mPlayer.isPlaying()) {
            mPlayer.pause();
        } else {
            mPlayer.start();
        }
        updatePausePlay();
    }
    
    private GestureDetector mGestureDetector = new GestureDetector(new GestureDetector.SimpleOnGestureListener(){
    	
    	public boolean onDoubleTap(MotionEvent e) {
//    		Log.d(TAG, "onDoubleTap!!!");
    		
    		if (mPlayer instanceof MediaPlayerControl) {
    			((MediaPlayerControl) mPlayer).switchDisplayMode();
    		}
    		
    		return false;
    	};
    	
    	public boolean onSingleTapConfirmed(MotionEvent e) {
//    		Log.d(TAG, "onSingleTapConfirmed!!!");
    		
    		if (isShowing()) {
    			hide();
    		}
    		
    		return false;
    	};
    });
    
    private OnTouchListener mTouchListener = new OnTouchListener() {
		
		@Override
		public boolean onTouch(View v, MotionEvent event) {
			
			return mGestureDetector.onTouchEvent(event);
		}
	};
	
	private OnSeekBarChangeListener mProgressChangeListener = new OnSeekBarChangeListener() {
		
		@Override
		public void onStopTrackingTouch(SeekBar seekBar) {
			
			int progress = seekBar.getProgress();
			if (mPlayer != null) {
				int duration = mPlayer.getDuration();
				if (duration > 0) {
					long position = (duration / 1000L) * progress;
					
					int pos = setProgress((int)position);
					mPlayer.seekTo(pos);
				}
			}
			
			show(sDefaultTimeout);
			
			mHandler.sendEmptyMessage(SHOW_PROGRESS);
		}
		
		@Override
		public void onStartTrackingTouch(SeekBar seekBar) {
			show(3600000);
			
			mHandler.removeMessages(SHOW_PROGRESS);
		}
		
		@Override
		public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
			if (fromUser) {
				if (mPlayer != null) {
					int duration = mPlayer.getDuration();
					if (duration > 0) {
						long position = (duration / 1000L) * progress;
						setProgress((int)position);
					}
				}
			}
		}
	};
	
	private OnSeekBarChangeListener mVolumeChangeListener = new OnSeekBarChangeListener() {
		
		@Override
		public void onStopTrackingTouch(SeekBar seekBar) {
			show(sDefaultTimeout);
			mVolumerDragging = false;
		}
		
		@Override
		public void onStartTrackingTouch(SeekBar seekBar) {
			show(3600000);
			mVolumerDragging = true;
		}
		
		@Override
		public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
			if (mVolumerDragging) {
				int maxVolume = mAudioManager.getStreamMaxVolume(AudioManager.STREAM_MUSIC);
				long volume = maxVolume * progress / 1000L;
				
				setVolume((int)volume);
			}
		}
	};
	
	public interface MediaPlayerControl extends android.widget.MediaController.MediaPlayerControl {
		void switchDisplayMode();
	}
}
