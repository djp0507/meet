package android.pplive.media.subtitle;

public class SubTitleSegment {

	enum SubTitleFormat {
		TEXT,
		RGB,
		YUV
	}
	
	enum SubTitleLanguate {
		CHS, 	/** 简体中文 */
		CHT, 	/** 繁体中文 */ 
		DAN, 	/** 丹麦文 */
		DEU, 	/** 德文 */
		ENG, 	/** 英文 */
		ESP, 	/** 西班牙文 */
		FIN, 	/** 芬兰文 */
		FRA, 	/** 法文(标准) */
		FRC, 	/** 加拿大法文 */
		ITA, 	/** 意大利文 */
		JPN, 	/** 日文 */
		KOR, 	/** 韩文 */
		NLD, 	/** 荷兰文 */
		NOR, 	/** 挪威文 */
		PLK, 	/** 波兰文 */
		PTB, 	/** 巴西葡萄牙文 */
		PTG, 	/** 葡萄牙文 */
		RUS, 	/** 俄文 */
		SVE, 	/** 瑞典文 */
		THA 	/** 泰文 */
	}

	private long mFromTime; /* milliseconds */
	
	private long mToTime; /* milliseconds */

	private SubTitleFormat mFormat;

	private String mData;

	public long getFromTime() {
		return mFromTime;
	}
	
	public long getToTime() {
		return mToTime;
	}
	
	public SubTitleFormat getFormat() {
		return mFormat;
	}
	
	public String getData() {
		return mData;
	}
	
	public void setData(String text){
		mData = text;
	}

	public void setFromTime(long time){
		mFromTime = time;
	}
	
	public void setToTime(long time){
		mToTime = time;
	}
}
