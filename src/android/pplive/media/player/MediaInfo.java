package android.pplive.media.player;

import java.io.File;

public class MediaInfo {

	private String mTitle = null;
	private String mPath = null;

	private long mDurationMS = 0;
	private long mSizeByte = 0;

	private File mData = null;

	private int mWidth = 0;
	private int mHeight = 0;

	private String mFormatName = null;
	private String mAudioName = null;
	private String mVideoName = null;

	private int mThumbnailWidth = 0;
	private int mThumbnailHeight = 0;

	private int[] mThumbnail = null;

	private int mAudioChannels = 0;
	private int mVideoChannels = 0;

	MediaInfo() {
		this("");
	}

	MediaInfo(String path) {
		this(path, 0, 0);
	}

	public MediaInfo(String path, long durationMS, long sizeByte) {
		mPath = path;
		mDurationMS = durationMS;
		mSizeByte = sizeByte;
	}

	public String getTitle() {
		if (mTitle == null) {
			File f = getData();
			String name = f.getName();
			int endIndex = name.lastIndexOf(".");

			if (endIndex - 1 >= 0) {
				mTitle = name.substring(0, endIndex);
			}
		}

		if (mTitle == null) {
			mTitle = "";
		}

		return mTitle;
	}

	public String getPath() {
		if (mPath == null) {
			mPath = "";
		}

		return mPath;
	}

	public long getDuration() {
		return mDurationMS;
	}

	public long getSize() {
		return mSizeByte;
	}

	public File getData() {
		if (null == mData) {
			mData = new File(getPath());
		}

		return mData;
	}

	public long lastModified() {
		return getData().lastModified();
	}

	public int getWidth() {
		return mWidth;
	}

	public int getHeight() {
		return mHeight;
	}

	public String getFormatName() {
		return mFormatName;
	}

	public String getAudioName() {
		return mAudioName;
	}

	public String getVideoName() {
		return mVideoName;
	}

	public int getThumbnailWidth() {
		return mThumbnailWidth;
	}

	public int getThumbnailHeight() {
		return mThumbnailHeight;
	}

	public int[] getThumbnail() {
		return mThumbnail;
	}

	public int getAudioChannels() {
		return mAudioChannels;
	}

	public int getVideoChannels() {
		return mVideoChannels;
	}

    @Override
    public String toString()
    {
        StringBuffer sb = new StringBuffer();
        sb.append(mTitle).append('|').append(mPath).append('|').
            append(mDurationMS).append('|').append(mSizeByte).append('|').
            append(mWidth).append('x').append(mHeight).append('|').
            append(mFormatName).append(':').append(mAudioName).append(':').
            append(mVideoName).append('|');
        return sb.toString();
    }
	
	
}
