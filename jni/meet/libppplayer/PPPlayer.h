#ifndef ANDROID_PPPLAYER_H
#define ANDROID_PPPLAYER_H

#include "jni.h"
#include "include-pp/AudioSink.h"
#include "include-pp/IPPAudioTrack.h"

#define OS_ANDROID
namespace android {
#include "player/player.h"
}

namespace android {

struct AwesomePlayer;

class AudioOutput : public AudioSink
{
public:
                            AudioOutput();
    virtual                 ~AudioOutput();

    virtual bool            ready() const { return mTrack != NULL; }
    virtual bool            realtime() const { return true; }
    virtual ssize_t         bufferSize() const;
    virtual ssize_t         frameCount() const;
    virtual ssize_t         channelCount() const;
    virtual ssize_t         frameSize() const;
    virtual uint32_t        latency() const;
    virtual float           msecsPerFrame() const;
    virtual status_t        getPosition(uint32_t *position);

    virtual status_t        open(
            uint32_t sampleRate, int channelCount,
            int format, int bufferCount,
            AudioCallback cb, void *cookie);

    virtual void            start();
    virtual ssize_t         write(const void* buffer, size_t size);
    virtual void            stop();
    virtual void            flush();
    virtual void            pause();
    virtual void            close();
    void                    setAudioStreamType(int streamType) { mStreamType = streamType; }
    void                    setVolume(float left, float right);
//    virtual status_t        dump(int fd, const Vector<String16>& args) const;

    static bool             isOnEmulator();
    static int              getMinBufferCount();
private:
    static void             setMinBufferCount();
    static void             CallbackWrapper(int event, void *me, void *info);

    //AudioTrack*             mTrack;
    IPPAudioTrack*          mTrack;
    AudioCallback           mCallback;
    void *                  mCallbackCookie;
    int                     mStreamType;
    float                   mLeftVolume;
    float                   mRightVolume;
    float                   mMsecsPerFrame;
    uint32_t                mLatency;

public: // visualization hack support
    uint32_t                mNumFramesWritten;
    void                    snoopWrite(const void*, size_t);
};

class PPPlayer : public IPlayer
{
public:
	PPPlayer();			

	void notify(int msg, int ext1, int ext2);
	void disconnect();
	status_t suspend();
	status_t resume();
	status_t setDataSource(const char *url);
	status_t setDataSource(int fd, int64_t offset, int64_t length);
	status_t setVideoSurface(void* surface);
	status_t prepare();
	status_t prepareAsync();
	status_t start();
	status_t stop();
	status_t pause();
	bool isPlaying();
	status_t seekTo(int msec);
	status_t getVideoWidth(int *w);
	status_t getVideoHeight(int *h);
	status_t getCurrentPosition(int *msec);
	status_t getDuration(int *msec);
	status_t reset();
	status_t setAudioStreamType(int type);
	status_t setLooping(int loop);
	bool isLooping();
	status_t setVolume(float leftVolume, float rightVolume);
//		status_t invoke(const Parcel& request, Parcel *reply);
//		status_t setMetadataFilter(const Parcel& filter);
//		status_t getMetadata(bool update_only, bool apply_filter, Parcel *metadata);
	status_t setListener(MediaPlayerListener* listener);
    int flags();
	status_t startCompatibilityTest();
	void stopCompatibilityTest();
	
    status_t startP2PEngine();
    void stopP2PEngine();

	virtual ~PPPlayer();

private:
    sp<AudioOutput> mAudioOutput;
	bool mLoop;
    int mStreamType;
	AwesomePlayer* mp;
	MediaPlayerListener* mListener;
};

}

#endif