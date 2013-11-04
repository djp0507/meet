/*
 * Copyright (C) 2009 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.android.player;

import android.app.Activity;
import android.content.Intent;
import android.ppmedia.MediaPlayer;
import android.ppmedia.MediaPlayer.OnBufferingUpdateListener;
import android.ppmedia.MediaPlayer.OnCompletionListener;
import android.ppmedia.MediaPlayer.OnPreparedListener;
import android.ppmedia.MediaPlayer.OnVideoSizeChangedListener;
import android.os.Bundle;
import android.util.Log;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;

public class Player extends Activity implements
        OnBufferingUpdateListener, OnCompletionListener,
        OnPreparedListener, OnVideoSizeChangedListener, SurfaceHolder.Callback {

    private static final String TAG = "MediaPlayerDemo";
    private int mVideoWidth;
    private int mVideoHeight;
    private MediaPlayer mMediaPlayer = null;
    private SurfaceView mPreview;
    private SurfaceHolder holder;
    private String path; 
    private boolean mIsVideoSizeKnown = false;
    private boolean mIsVideoReadyToBePlayed = false;

    /**
     * 
     * Called when the activity is first created.
     */
    @Override
    public void onCreate(Bundle icicle) {
        Log.d(TAG, "onCreate");
        super.onCreate(icicle);
        
        doCleanUp();
        setContentView(R.layout.mediaplayer_2);
        mPreview = (SurfaceView) findViewById(R.id.surface);
        holder = mPreview.getHolder();
        holder.addCallback(this);
        holder.setType(SurfaceHolder.SURFACE_TYPE_PUSH_BUFFERS);
    }

    @Override
    protected void onDestroy() {
        Log.d(TAG, "onDestroy");
        super.onDestroy();
        
        if (mMediaPlayer != null) {
            Log.d(TAG, "releasing mediaplayer");
            mMediaPlayer.release();
            mMediaPlayer = null;
        }
        doCleanUp();
    }

    @Override
    protected void onRestoreInstanceState(Bundle savedInstanceState) {
        Log.d(TAG, "onRestoreInstanceState");
        super.onRestoreInstanceState(savedInstanceState);
    }

    @Override
    protected void onSaveInstanceState(Bundle savedInstanceState) {
        Log.d(TAG, "onSaveInstanceState");
        super.onSaveInstanceState(savedInstanceState);
    }
 
    @Override
    protected void onStart() {
        Log.d(TAG, "onStart");
        super.onStart();
    }
 
    @Override
    protected void onRestart() {
        Log.d(TAG, "onRestart");
        super.onRestart();

        if (mMediaPlayer != null) {
            //mMediaPlayer.start();
        }
        
    }
 
    @Override
    protected void onStop() {
        Log.d(TAG, "onStop");
        super.onStop();

        if(mMediaPlayer!=null)
        {
        	mMediaPlayer.stop();
            mMediaPlayer.release();
            mMediaPlayer = null;
        }
        finish();
    }

    @Override
    protected void onResume()
    {
        Log.d(TAG, "onResume");  
        super.onResume();
    }
 
    @Override
    protected void onPause() {
        Log.d(TAG, "onPause");
        super.onPause();
        if (mMediaPlayer != null) {
            mMediaPlayer.pause();
        }
    }
    
    public void pause(View view)
    {
        if (mMediaPlayer != null) {
            mMediaPlayer.pause();
        }
    }
    
    public void play(View view)
    {
        if (mMediaPlayer != null) {
            mMediaPlayer.start();
        }
    }
    
    
    
    
    
    
    
    
    
    
    
    

    private void prepareVideo() {
        doCleanUp();
        try { 
        	//check device compatibility
        	Log.d(TAG, "Check comp start");
        	boolean ret = MediaPlayer.CheckCompatibility("/data/data/com.android.player/", holder.getSurface());
        	Log.d(TAG, "Check comp end:"+ret);
        	if(ret)
        	{
		        // Create a new media player and set the listeners 
		        path = "/data/data/com.android.player/lib/libsample.so";//"ppvod:///%5Bmobile%5D%B8%D0%B9%D9%D3%CE%CF%B7.mp4?t=1339652683901&bwtype=1"; 
		        Log.d(TAG, "start### new MediaPlayer"); 
		        mMediaPlayer = new MediaPlayer("/data/data/com.android.player/");
		        //mMediaPlayer.init("/data/data/com.android.player/");
		        Log.d(TAG, "start setting datasource");
		        //点播: ppvod://+url
		        //直播: pplive2://+url
		        //本地文件：ppfile-mp4:///mnt/xxx.mp4 或 ppfile-3gp:///mnt/xxx.3gp
		        mMediaPlayer.setDataSource(path);
		        //Log.e(TAG, "finish setting datasource");
		        Log.d(TAG, "start setting display");
		        mMediaPlayer.setDisplay(holder);
		        //Log.e(TAG, "finish setting display");
		        Log.d(TAG, "start prepare");
		        mMediaPlayer.prepareAsync();
		        mMediaPlayer.setOnBufferingUpdateListener(this);
		        mMediaPlayer.setOnCompletionListener(this);
		        mMediaPlayer.setOnPreparedListener(this);
		        mMediaPlayer.setOnVideoSizeChangedListener(this);
		        
	            Log.d(TAG, mMediaPlayer.getVersion());
        	}
        } catch (Exception e) {
            Log.e(TAG, "error: " + e.getMessage(), e);
        }
    }
 
    public void onBufferingUpdate(MediaPlayer arg0, int percent) {
        Log.d(TAG, "onBufferingUpdate percent:" + percent);

    }

    public void onCompletion(MediaPlayer arg0) {
        Log.d(TAG, "onCompletion called");
        if (mMediaPlayer != null) {
            Log.d(TAG, "releasing mediaplayer");
            mMediaPlayer.release();
            mMediaPlayer = null;
        }
        doCleanUp();
        finish();
    }

    public void onVideoSizeChanged(MediaPlayer mp, int width, int height) {
        Log.v(TAG, "onVideoSizeChanged called");
        if (width == 0 || height == 0) {
            Log.e(TAG, "invalid video width(" + width + ") or height(" + height + ")");
            return;
        }
        
        mIsVideoSizeKnown = true;
        mVideoWidth = width;
        mVideoHeight = height;
        if (mIsVideoReadyToBePlayed && mIsVideoSizeKnown) {
            startVideoPlayback();
        }
    }
 
    public void onPrepared(MediaPlayer mediaplayer) {
        Log.d(TAG, "onPrepared called");
        mIsVideoReadyToBePlayed = true;
        if (mIsVideoReadyToBePlayed && mIsVideoSizeKnown) {
            startVideoPlayback();
        }
    }

    public void surfaceChanged(SurfaceHolder surfaceholder, int i, int j, int k) {
        Log.d(TAG, "surfaceChanged called 2");

    }
 
    public void surfaceDestroyed(SurfaceHolder surfaceholder) {
        Log.d(TAG, "surfaceDestroyed called");
    }


    public void surfaceCreated(SurfaceHolder holder) {
        Log.d(TAG, "surfaceCreated called 1");
        prepareVideo();
    }

    private void doCleanUp() {
        mVideoWidth = 0;
        mVideoHeight = 0;
        mIsVideoReadyToBePlayed = false;
        mIsVideoSizeKnown = false;
        mMediaPlayer = null;
    }

    private void startVideoPlayback() {
        Log.v(TAG, "startVideoPlayback");
        holder.setFixedSize(mVideoWidth, mVideoHeight);
        if(mMediaPlayer!=null)
        {
        	mMediaPlayer.start();
        }
    }
   
 
	static {
		System.loadLibrary("ppbox-armandroid-r4-gcc44-mt-1.0.0");
	}
    
}
