#define LOG_TAG "PPPlayer"

#include "platform-pp/PPPlatForm.h"
#include "include-pp/AwesomePlayer.h"
#include "include-pp/Surface.h"

#include "include-pp/utils/Log.h"
#include "include-pp/utils/SystemClock.h"
#include "include-pp/cutils/atomic.h"
#include "include-pp/cutils/properties.h"

#include "include-pp/PPBox_Util.h"
#include "platform/platforminfo.h"

#include "PPPlayer.h"

namespace android {

JavaVM *gs_jvm = NULL;
// const char *MODEL_NAME = NULL;
// const char *BOARD_NAME = NULL;
// const char *CHIP_NAME = NULL;
// const char *MANUFACTURE_NAME = NULL;
// const char *APP_PATH = NULL;
// const char *RELEASE_VERSION = NULL;
PlatformInfo *gPlatformInfo = NULL;

bool START_P2P = false;

static bool isStartedP2P = false;

static bool IsOnEmulator;
static int MinBufferCount;  // 12 for emulator; otherwise 4
static const int NUMVIZBUF = 32;
static const int VIZBUFFRAMES = 1024;
static const int BUFTIMEMSEC = NUMVIZBUF * VIZBUFFRAMES * 1000 / 44100;
static const int TOTALBUFTIMEMSEC = NUMVIZBUF * BUFTIMEMSEC;
static uint64_t lastReadTime;
static uint64_t lastWriteTime;


#undef LOG_TAG
#define LOG_TAG "AudioOutput"
AudioOutput::AudioOutput()
    : mCallback(NULL),
      mCallbackCookie(NULL)
{
    mTrack = NULL;
    mStreamType = AudioSystemWrapper::MUSIC;
    mLeftVolume = 1.0;
    mRightVolume = 1.0;
    mLatency = 0;
    mMsecsPerFrame = 0;
    mNumFramesWritten = 0;
    setMinBufferCount();
}

AudioOutput::~AudioOutput()
{
    LOGD("AudioOutput desconstructor");

	close();
}

void AudioOutput::setMinBufferCount()
{
    char value[PROPERTY_VALUE_MAX]={0};
    if (property_get("ro.kernel.qemu", value, 0))
	{
        IsOnEmulator = true;
        MinBufferCount = 12;  // to prevent systematic buffer underrun for emulator
    }
}

bool AudioOutput::isOnEmulator()
{
    setMinBufferCount();
    return IsOnEmulator;
}

int AudioOutput::getMinBufferCount()
{
    setMinBufferCount();
    return MinBufferCount;
}

ssize_t AudioOutput::bufferSize() const
{
    if (mTrack == 0) return NO_INIT;
    return mTrack->frameCount() * frameSize();
}

ssize_t AudioOutput::frameCount() const
{
    if (mTrack == 0) return NO_INIT;
    return mTrack->frameCount();
}

ssize_t AudioOutput::channelCount() const
{
    if (mTrack == 0) return NO_INIT;
    return mTrack->channelCount();
}

ssize_t AudioOutput::frameSize() const
{
    if (mTrack == 0) return NO_INIT;
    return mTrack->frameSize();
}

uint32_t AudioOutput::latency () const
{
    return mLatency;
}

float AudioOutput::msecsPerFrame() const
{
    return mMsecsPerFrame;
}

status_t AudioOutput::getPosition(uint32_t *position)
{
    if (mTrack == 0) return NO_INIT;
    return mTrack->getPosition(position);
}

status_t AudioOutput::open(
        uint32_t sampleRate, int channelCount, int format, int bufferCount,
        AudioCallback cb, void *cookie)
{
    mCallback = cb;
    mCallbackCookie = cookie;

    // Check argument "bufferCount" against the mininum buffer count
    if (bufferCount < MinBufferCount) {
        LOGD("bufferCount (%d) is too small and increased to %d", bufferCount, MinBufferCount);
        bufferCount = MinBufferCount;

    }
    
    LOGI("open(%u, %d, %d, %d)", sampleRate, channelCount, format, bufferCount);
    
    if (mTrack)
	{
	    LOGV("Deleting mTrack");
		close();
		mTrack = NULL;
    }
    int afSampleRate = 0;
    int afFrameCount = 0;
    int frameCount = 0;

    if (AudioSystemWrapper::getOutputFrameCount(&afFrameCount, mStreamType) != NO_ERROR) {
        return NO_INIT;
    }
    if (AudioSystemWrapper::getOutputSamplingRate(&afSampleRate, mStreamType) != NO_ERROR) {
        return NO_INIT;
    }
    if(afSampleRate <= 0)
	{
	    afSampleRate = 44100;
		LOGE("Failed to get sample rate, update it to default value");
	}
    frameCount = (sampleRate*afFrameCount*bufferCount)/afSampleRate;
	
    LOGI("Device's audio sample rate: %d", afSampleRate);
    LOGI("Target audio sample rate: %d", sampleRate);
    LOGI("Device's audio frame count: %d", afFrameCount);
    LOGI("Target audio frame count: %d", frameCount);

/*
    AudioTrack *t;
    if (mCallback != NULL) {
        t = new AudioTrack(
                mStreamType,
                sampleRate,
                format,
                (channelCount == 2) ? AudioSystemWrapper::CHANNEL_OUT_STEREO : AudioSystemWrapper::CHANNEL_OUT_MONO,
                frameCount,
                0, // flags
                CallbackWrapper,
                this);
    } else {
        t = new AudioTrack(
                mStreamType,
                sampleRate,
                format,
                (channelCount == 2) ? AudioSystemWrapper::CHANNEL_OUT_STEREO : AudioSystemWrapper::CHANNEL_OUT_MONO,
                frameCount);
    }

    if ((t == 0) || (t->initCheck() != NO_ERROR)) {
        LOGE("Unable to create audio track");
        delete t;
        return NO_INIT;
    }

    LOGE("setVolume");
    t->setVolume(mLeftVolume, mRightVolume);
    mMsecsPerFrame = 1.e3 / (float) sampleRate;
    mLatency = t->latency();
    mTrack = t;
    */
    
    //AudioTrack *t;
    if (mCallback != NULL) {
        mTrack = AudioSystemWrapper::createAudioTrack(
                mStreamType,
                sampleRate,
                format,
                (channelCount == 2) ? AudioSystemWrapper::CHANNEL_OUT_STEREO : AudioSystemWrapper::CHANNEL_OUT_MONO,
                frameCount,
                0, // flags
                CallbackWrapper,
                this);
    } else {
        mTrack = AudioSystemWrapper::createAudioTrack(
                mStreamType,
                sampleRate,
                format,
                (channelCount == 2) ? AudioSystemWrapper::CHANNEL_OUT_STEREO : AudioSystemWrapper::CHANNEL_OUT_MONO,
                frameCount);
    }

    if (mTrack == NULL) {
        LOGE("Unable to create audio track");
        return NO_INIT;
    }
	LOGD("mTrack->initCheck()");
	if(mTrack->initCheck() != NO_ERROR)
	{
        LOGE("audio track init check failed");
        delete mTrack;
        mTrack = NULL;
        return NO_INIT;
	}

    LOGD("setVolume");
    mTrack->setVolume(mLeftVolume, mRightVolume);
    mMsecsPerFrame = 1.e3 / (float) sampleRate;
    mLatency = mTrack->latency();
    //mTrack = t;
	
    return NO_ERROR;
}

void AudioOutput::start()
{
    //LOGE("********starting Audio sink");
    if (mTrack) {
        mTrack->setVolume(mLeftVolume, mRightVolume);
        mTrack->start();
        mTrack->getPosition(&mNumFramesWritten);
    }
}

static void makeVizBuffers(const char *data, int len, uint64_t time)
{
}

void AudioOutput::snoopWrite(const void* buffer, size_t size) {
    // Only make visualization buffers if anyone recently requested visualization data
    uint64_t now = uptimeMillis();
    if (lastReadTime + TOTALBUFTIMEMSEC >= now) {
        // Based on the current play counter, the number of frames written and
        // the current real time we can calculate the approximate real start
        // time of the buffer we're about to write.
        uint32_t pos;
        mTrack->getPosition(&pos);

        // we're writing ahead by this many frames:
        int ahead = mNumFramesWritten - pos;
        //LOGI("@@@ written: %d, playpos: %d, latency: %d", mNumFramesWritten, pos, mTrack->latency());
        // which is this many milliseconds, assuming 44100 Hz:
        ahead /= 44;

        makeVizBuffers((const char*)buffer, size, now + ahead + mTrack->latency());
        lastWriteTime = now;
    }
}


ssize_t AudioOutput::write(const void* buffer, size_t size)
{
    LOG_FATAL_IF(mCallback != NULL, "Don't call write if supplying a callback.");

    LOGD("write(%p, %u)", buffer, size);
    if (mTrack) {
        snoopWrite(buffer, size);
        ssize_t ret = mTrack->write(buffer, size);
        mNumFramesWritten += ret / 4; // assume 16 bit stereo
        return ret;
    }
    return NO_INIT;
}

void AudioOutput::stop()
{
    LOGV("stop");
    if (mTrack) mTrack->stop();
    lastWriteTime = 0;
}

void AudioOutput::flush()
{
    LOGV("flush");
    if (mTrack) mTrack->flush();
}

void AudioOutput::pause()
{
    LOGV("pause");
    if (mTrack) mTrack->pause();
    lastWriteTime = 0;
}

void AudioOutput::close()
{
    LOGV("close");
	//roger
	//todo: temperoraily make this for fix an issue which pending closing process when failed to receive data.
    if(mTrack != NULL) 
    {
        delete mTrack;
        mTrack = NULL;
    }
}

void AudioOutput::setVolume(float left, float right)
{
    LOGV("setVolume(%f, %f)", left, right);
    mLeftVolume = left;
    mRightVolume = right;
    if (mTrack) {
        mTrack->setVolume(left, right);
    }
}

// static
void AudioOutput::CallbackWrapper(
        int event, void *cookie, void *info) {
    LOGD("callbackwrapper");
    if (event != IPPAudioTrack::EVENT_MORE_DATA) {
        return;
    }

    AudioOutput *me = (AudioOutput *)cookie;
    IPPAudioTrack::Buffer *buffer = (IPPAudioTrack::Buffer *)info;

    size_t actualSize = (*me->mCallback)(
            me, buffer->raw, buffer->size, me->mCallbackCookie);

    buffer->size = actualSize;

    if (actualSize > 0) {
        me->snoopWrite(buffer->raw, actualSize);
    }
}


///////////////////////////////////////////////////////////////////////////////

#undef LOG_TAG
#define LOG_TAG "PPPlayer"
	
PPPlayer::PPPlayer() {
	LOGD("PPPlayer constructor");
    mListener = NULL;
	mLoop = false;
    mStreamType = 3;//AudioSystemWrapper::MUSIC;
	//mp = new MediaPlayer();
	mp = new AwesomePlayer();
	mp->setListener(this);
}

PPPlayer::~PPPlayer() {
	LOGD("PPPlayer destructor");
    mAudioOutput.clear();
	if(mp!=NULL)
	{
	    delete mp;
		mp = NULL;
	}
    if(mListener!=NULL)
    {
        delete mListener;
        mListener = NULL;
    }
}

void PPPlayer::notify(int msg, int ext1, int ext2) {
	LOGD("PPPlayer::notify()");
	if(mListener != NULL)
	{
		mListener->notify(msg, ext1, ext2);
	}
	else
	{
		LOGE("mListener is NULL");
	}
}

void PPPlayer::disconnect() {
	LOGD("PPPlayer::disconnect()");
	//mp->reset();
}

status_t PPPlayer::suspend() {
	LOGD("PPPlayer::suspend()");
	if(!mp) return UNKNOWN_ERROR;
	
	return mp->suspend();
}

status_t PPPlayer::resume() {
	LOGD("PPPlayer::resume()");
	if(!mp) return UNKNOWN_ERROR;
	
	return mp->resume();
}

status_t PPPlayer::setDataSource(const char *url) {
	LOGD("PPPlayer::setDataSource()");
	if(!mp || !url) return UNKNOWN_ERROR;
    //url = "ppfile-mp4:///data/data/com.pplive.androidphone/lib/libsample2.so";

	if(START_P2P)
	{
		if(!strncasecmp("ppvod", url, 5) ||
			!strncasecmp("pplive", url, 6) ||
			!strncasecmp("ppfile", url, 6))
		{
			if (startP2PEngine() != OK) {
				return UNKNOWN_ERROR;
			}
		}
	}
	
	mAudioOutput = new AudioOutput();
	mp->setAudioSink(mAudioOutput);
	return mp->setDataSource(url, NULL);
}

status_t PPPlayer::setDataSource(int fd, int64_t offset, int64_t length) {
	LOGD("PPPlayer::setDataSource()");
	if(!mp) return UNKNOWN_ERROR;
	
	return mp->setDataSource(fd, offset, length);
}

status_t PPPlayer::setVideoSurface(void* surface) {
	LOGD("PPPlayer::setVideoSurface()");
	if(!mp) return UNKNOWN_ERROR;
	
	mp->setSurface(surface);
	return OK;
}

status_t PPPlayer::prepare() {
	LOGD("PPPlayer::prepare()");
	if(!mp) return UNKNOWN_ERROR;

	return mp->prepare();
}

status_t PPPlayer::prepareAsync() {
	LOGD("PPPlayer::prepareAsync()");
	if(!mp) return UNKNOWN_ERROR;
	
	return mp->prepareAsync();
}

status_t PPPlayer::start() {
	LOGD("PPPlayer::start()");
	if(!mp) return UNKNOWN_ERROR;
	
	return mp->play();
}

status_t PPPlayer::stop() {
	LOGD("PPPlayer::stop()");
	if(!mp) return UNKNOWN_ERROR;
	
	return mp->pause();
}

status_t PPPlayer::pause() {
	LOGD("PPPlayer::pause()");
	if(!mp) return UNKNOWN_ERROR;
	
	return mp->pause();
}

bool PPPlayer::isPlaying() {
	LOGD("PPPlayer::isPlaying()");
	if(!mp) return UNKNOWN_ERROR;
	
	return mp->isPlaying();
}

status_t PPPlayer::seekTo(int msec) {
	if(!mp) return UNKNOWN_ERROR;
	
    LOGD("seekTo 1: %d ms", msec);
    int64_t seekUs = msec;
	seekUs *= 1000;
    LOGD("seekTo 2: %lld us", seekUs);
    status_t err = mp->seekTo(seekUs);
    LOGD("seekTo 3");

    return err;
}

status_t PPPlayer::getVideoWidth(int *w) {
	LOGD("PPPlayer::getVideoWidth()");
	if(!mp) return UNKNOWN_ERROR;
	
	int h;
	return mp->getVideoDimensions(w, &h);
}

status_t PPPlayer::getVideoHeight(int *h) {
	LOGD("PPPlayer::getVideoHeight()");
	if(!mp) return UNKNOWN_ERROR;
	
	int w;
	return mp->getVideoDimensions(&w, h);
}

status_t PPPlayer::getCurrentPosition(int *msec) {
	LOGD("PPPlayer::getCurrentPosition()");
	if(!mp) return UNKNOWN_ERROR;

    int64_t positionUs;
    status_t err = mp->getPosition(&positionUs);

    if (err != OK) {
        return err;
    }

    *msec = (positionUs + 500) / 1000;
    return OK;}

status_t PPPlayer::getDuration(int *msec) {
	LOGD("PPPlayer::getDuration()");
	if(!mp) return UNKNOWN_ERROR;
	
    int64_t durationUs;
    status_t err = mp->getDuration(&durationUs);

    if (err != OK) {
        *msec = 0;
        return OK;
    }

    *msec = (durationUs + 500) / 1000;

    return OK;
}

status_t PPPlayer::reset() {
	LOGD("PPPlayer::reset()");
	if(!mp) return UNKNOWN_ERROR;
	
	mp->reset();
	return OK;
}

status_t PPPlayer::setAudioStreamType(int type) {
	LOGD("PPPlayer::setAudioStreamType()");
	if(!mp) return UNKNOWN_ERROR;
	
	mStreamType = type;
    if (mAudioOutput != 0) mAudioOutput->setAudioStreamType(mStreamType);
	return OK;
}

status_t PPPlayer::setLooping(int loop) {
	LOGD("PPPlayer::setLooping()");
	if(!mp) return UNKNOWN_ERROR;
	
    mLoop = (loop != 0);
	return mp->setLooping(loop);
}

bool PPPlayer::isLooping() {
	LOGD("PPPlayer::isLooping()");
	return mLoop;
}

status_t PPPlayer::setVolume(float leftVolume, float rightVolume) {
	LOGD("PPPlayer::setVolume()");
	if(mAudioOutput==NULL) return UNKNOWN_ERROR;
	
    mAudioOutput->setVolume(leftVolume, rightVolume);
	return OK;
}
/*
status_t PPPlayer::invoke(const Parcel& request, Parcel *reply) {
	LOGI("PPPlayer::invoke()");
	return mp->invoke(request, reply);
}

status_t PPPlayer::setMetadataFilter(const Parcel &filter) {
	LOGI("PPPlayer::setMetadataFilter()");
	return mp->setMetadataFilter(filter);
}

status_t PPPlayer::getMetadata(bool update_only, bool apply_filter, Parcel *metadata) {
	LOGI("PPPlayer::getMetadata()");
	return mp->getMetadata(update_only, apply_filter, metadata);
}
*/
status_t PPPlayer::setListener(MediaPlayerListener* listener) {
	LOGD("PPPlayer::setListener()");
	if(!mp) return UNKNOWN_ERROR;

    if(mListener !=NULL) delete mListener;
	mListener = listener;
	return OK;
}

int PPPlayer::flags()
{
	if(!mp) return UNKNOWN_ERROR;

	return mp->flags();
}

status_t PPPlayer::startCompatibilityTest()
{
	if(!mp) return UNKNOWN_ERROR;
	
	return mp->startCompatibilityTest();
}

void PPPlayer::stopCompatibilityTest()
{
	if(!mp) return;
	
	return mp->stopCompatibilityTest();
}

status_t PPPlayer::startP2PEngine() {
	if(!START_P2P || isStartedP2P) return OK;

	LOGI("=========> Calling PPBOX_StartP2PEngine start");
	//PP_int32 ec = PPBOX_StartP2PEngine("12", "161", "08ae1acd062ea3ab65924e07717d5994");
	PP_int32 ec = ((PPBoxHandle*)gPlatformInfo->ppbox)->startP2PEngine("12", "161", "08ae1acd062ea3ab65924e07717d5994");
	if (ppbox_success == ec || ppbox_already_start == ec)
	{
		isStartedP2P = true;
		LOGI("Start p2p engine success");
	}
	else
	{
		LOGE("Start p2p engine failed with code: %d", ec);
	}
	LOGI("=========> Calling PPBOX_StartP2PEngine end");
	
	return isStartedP2P ? OK : UNKNOWN_ERROR;
}

void PPPlayer::stopP2PEngine() {
	if(START_P2P)
	{
		//PPBOX_StopP2PEngine();
		((PPBoxHandle*)gPlatformInfo->ppbox)->stopP2PEngine();
	}
}

extern "C" IPlayer* getPlayer(void* context) {
	
	gPlatformInfo = (PlatformInfo*)context;
	gs_jvm =(JavaVM*)(gPlatformInfo->jvm);
	//START_P2P = startP2PEngine;
	
	return new PPPlayer();
}
}
