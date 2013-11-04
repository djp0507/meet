package com.android.player;

import android.os.Bundle;
import android.app.Activity;
import android.content.Intent;
import android.util.Log;
import android.view.Menu;
import android.view.MenuItem;
import android.widget.TextView;

public class DisplayMessageActivity extends Activity {

    private static final String TAG = "MediaPlayerDemo.DisplayMessageActivity";
    
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        //setContentView(R.layout.activity_display_message);
        
        
        setContentView(R.layout.activity_display_message);
        Log.d(TAG, "onCreate");
    }
    
    @Override
    public void onStart()
    {
    	super.onStart();

        Log.d(TAG, "onStart");
    }
    
    @Override
    public void onResume()
    {
    	super.onResume();
    	
    	Log.d(TAG, "onResume");
    }

    @Override
    public void onPause()
    {
    	super.onPause();

        Log.d(TAG, "onPause");
    }

    @Override
    public void onStop()
    {
    	super.onStop();

        Log.d(TAG, "onStop");
    }
    
    @Override
    public void onDestroy()
    {
    	super.onDestroy();
    	
    	Log.d(TAG, "onDestroy");
    }

}
