package android.pplive.media.subtitle;

public interface SubTitleParser {

	public void close();

	public SubTitleSegment next();

	public void prepareAsync();

	public void seekTo(long msec);

	public void setDataSource(String filePath);

	public void setOnPreparedListener(Callback listener);

	interface Callback {
		
		void onPrepared(boolean success, String msg);
		
		void onSeekComplete();
	}
}
