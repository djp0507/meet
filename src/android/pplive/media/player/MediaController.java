package android.pplive.media.player;

import android.content.Context;
import android.pplive.media.player.MeetVideoView.DecodeMode;
import android.util.AttributeSet;
import android.view.View;
import android.widget.RelativeLayout;

public abstract class MediaController extends RelativeLayout {
    protected MediaPlayerControl mPlayer;
    protected View mRoot;

    public MediaController(Context context) {
        super(context);
    }

    public MediaController(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    @Override
    public void onFinishInflate() {
        super.onFinishInflate();
        mRoot = this;
    }

    public void setMediaPlayer(MediaPlayerControl player) {
        mPlayer = player;
    }

    protected void doPauseResume() {
        if (mPlayer == null) {
            return;
        }
        if (mPlayer.isPlaying()) {
            mPlayer.pause();
        } else {
            mPlayer.start();
        }
    }

    public interface MediaPlayerControl {
        void start();

        void pause();

        int getDuration();

        int getCurrentPosition();

        void seekTo(int pos);

        boolean isPlaying();

        int getBufferPercentage();

        boolean canPause();

        boolean canSeekBackward();

        boolean canSeekForward();

        int getDisplayMode();

        void setDisplayMode(int mode);

        void switchDisplayMode();

        void setDecodeMode(DecodeMode mode);

        DecodeMode getDecodeMode();
    }

}
