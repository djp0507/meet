/* mediaplayer.cpp
**
** Copyright 2006, The Android Open Source Project
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/
 
//#define LOG_NDEBUG 0
#define LOG_TAG "MediaPlayer"
//#include "include-pp/log.h"

#include "include-pp/MediaPlayerService.h"
#include "include-pp/mediaplayer.h"

#include <utils/Log.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

//#include <binder/IServiceManager.h>
//#include <binder/IPCThreadState.h>
//#include <binder/MemoryHeapBase.h>
//#include <binder/IMemory.h>
//#include <binder/MemoryBase.h>

//#include <media/AudioTrack.h>


//#include <surfaceflinger/Surface.h>
#include "include-pp/Surface.h"

#include <utils/KeyedVector.h>
#include <utils/String8.h>





namespace android {

MediaPlayer::MediaPlayer()
{
	LOGD("memory allocation at %p", this);
    mListener = NULL;
    mCookie = NULL;
    mDuration = -1;
    mStreamType = AudioSystemWrapper::MUSIC;
    mCurrentPosition = -1;
    mSeekPosition = -1;
    mCurrentState = MEDIA_PLAYER_IDLE;
    mPrepareSync = false;
    mPrepareStatus = NO_ERROR;
    mLoop = false;
    mLeftVolume = mRightVolume = 1.0;
    mVideoWidth = mVideoHeight = 0;
    mLockThreadId = 0;
}

MediaPlayer::~MediaPlayer()
{
	LOGD("memory release at %p", this);
    disconnect();
}

void MediaPlayer::disconnect()
{
    LOGV("disconnect");
    LOGD("++++++++ disconnect 1");
    sp<IMediaPlayer> p;
    {
    LOGD("++++++++ disconnect 2");
        Mutex::Autolock _l(mLock);
        p = mPlayer;
        mPlayer.clear();
    }

    LOGD("++++++++ disconnect 3");
    if (p != 0) {
    LOGD("++++++++ disconnect 4");
        p->disconnect();
    }
    LOGD("++++++++ disconnect 5");
}

// always call with lock held
void MediaPlayer::clear_l()
{
    mDuration = -1;
    mCurrentPosition = -1;
    mSeekPosition = -1;
    mVideoWidth = mVideoHeight = 0;
}

status_t MediaPlayer::setListener(const sp<MediaPlayerListener>& listener)
{
    LOGV("setListener");
    Mutex::Autolock _l(mLock);
    mListener = listener;
    return NO_ERROR;
}


status_t MediaPlayer::setDataSource(const sp<IMediaPlayer>& player)
{
    status_t err = UNKNOWN_ERROR;
    sp<IMediaPlayer> p;
    { // scope for the lock
        Mutex::Autolock _l(mLock);

        if (!((mCurrentState & MEDIA_PLAYER_IDLE) ||
                (mCurrentState == MEDIA_PLAYER_STATE_ERROR ))) {
            LOGE("setDataSource called in state %d", mCurrentState);
            return INVALID_OPERATION;
        }

        clear_l();
        p = mPlayer;
        mPlayer = player;
        if (player != 0) {
            mCurrentState = MEDIA_PLAYER_INITIALIZED;
            err = NO_ERROR;
        } else {
            LOGE("Unable to to create media player");
        }
    }

    if (p != 0) {
        p->disconnect();
    }

    return err;
}

status_t MediaPlayer::setDataSource(
        const char *url, const KeyedVector<String8, String8> *headers)
{
    LOGE("setDataSource(%s)", url);


    status_t err = BAD_VALUE;
    if (url != NULL) {
        //roger
        //const sp<IMediaPlayerService>& service(getMediaPlayerService());
        const sp<MediaPlayerService>& service(MediaPlayerService::getInstance());
        if (service.get() != 0) {
	     LOGE("Start creating player");
            sp<IMediaPlayer> player(
                    service->create(getpid(), this, url, headers));
            err = setDataSource(player);
        }
    }
    return err;
}

status_t MediaPlayer::setDataSource(int fd, int64_t offset, int64_t length)
{/*
    LOGV("setDataSource(%d, %lld, %lld)", fd, offset, length);
	
    status_t err = UNKNOWN_ERROR;
    const sp<IMediaPlayerService>& service(getMediaPlayerService());
    if (service != 0) {
        sp<IMediaPlayer> player(service->create(getpid(), this, fd, offset, length));
        err = setDataSource(player);
    }
    return err;
    */
    return OK;
}

status_t MediaPlayer::invoke(const Parcel& request, Parcel *reply)
{
    Mutex::Autolock _l(mLock);
    const bool hasBeenInitialized =
            (mCurrentState != MEDIA_PLAYER_STATE_ERROR) &&
            ((mCurrentState & MEDIA_PLAYER_IDLE) != MEDIA_PLAYER_IDLE);
    if ((mPlayer != NULL) && hasBeenInitialized) {
         LOGV("invoke %d", request.dataSize());
         return  mPlayer->invoke(request, reply);
    }
    LOGE("invoke failed: wrong state %X", mCurrentState);
    return INVALID_OPERATION;
}

status_t MediaPlayer::suspend() {
    Mutex::Autolock _l(mLock);
    return mPlayer->suspend();
}

status_t MediaPlayer::resume() {
    Mutex::Autolock _l(mLock);
    return mPlayer->resume();
}

status_t MediaPlayer::setMetadataFilter(const Parcel& filter)
{
    LOGD("setMetadataFilter");
    Mutex::Autolock lock(mLock);
    if (mPlayer == NULL) {
        return NO_INIT;
    }
    return mPlayer->setMetadataFilter(filter);
}

status_t MediaPlayer::getMetadata(bool update_only, bool apply_filter, Parcel *metadata)
{
    LOGE("getMetadata");
    Mutex::Autolock lock(mLock);
    if (mPlayer == NULL) {
        return NO_INIT;
    }
    return mPlayer->getMetadata(update_only, apply_filter, metadata);
}

status_t MediaPlayer::setVideoSurface(const sp<Surface>& surface)
{
    LOGE("setVideoSurface: %p", surface.get());
    Mutex::Autolock _l(mLock);
    if (mPlayer == 0) return NO_INIT;
    if (surface != NULL)
        //return  mPlayer->setVideoSurface(surface->getISurface());
        return  mPlayer->setVideoSurface(surface->mSurface);
    else
        return  mPlayer->setVideoSurface(NULL);
}

// must call with lock held
status_t MediaPlayer::prepareAsync_l()
{
    if ( (mPlayer != 0) && ( mCurrentState & ( MEDIA_PLAYER_INITIALIZED | MEDIA_PLAYER_STOPPED) ) ) {
        mPlayer->setAudioStreamType(mStreamType);
        mCurrentState = MEDIA_PLAYER_PREPARING;
	 LOGE("Preparing asyn...");
        return mPlayer->prepareAsync();
    }
    LOGE("prepareAsync called in state %d", mCurrentState);
    return INVALID_OPERATION;
}

// TODO: In case of error, prepareAsync provides the caller with 2 error codes,
// one defined in the Android framework and one provided by the implementation
// that generated the error. The sync version of prepare returns only 1 error
// code.
status_t MediaPlayer::prepare()
{
    LOGE("prepare");
    Mutex::Autolock _l(mLock);
    mLockThreadId = getThreadId();
    if (mPrepareSync) {
        mLockThreadId = 0;
        return -EALREADY;
    }
    mPrepareSync = true;
    status_t ret = prepareAsync_l();
    if (ret != NO_ERROR) {
        mLockThreadId = 0;
        return ret;
    }

    if (mPrepareSync) {
	 LOGE("pending");
        mSignal.wait(mLock);  // wait for prepare done
        mPrepareSync = false;
    }
    LOGE("prepare complete - status=%d", mPrepareStatus);
    mLockThreadId = 0;
    return mPrepareStatus;
}

status_t MediaPlayer::prepareAsync()
{
    LOGV("prepareAsync");
    Mutex::Autolock _l(mLock);
    return prepareAsync_l();
}

status_t MediaPlayer::start()
{
    LOGV("MediaPlayer::start()");
    Mutex::Autolock _l(mLock);
    if (mCurrentState & MEDIA_PLAYER_STARTED)
        return NO_ERROR;
    if ( (mPlayer != 0) && ( mCurrentState & ( MEDIA_PLAYER_PREPARED |
                    MEDIA_PLAYER_PLAYBACK_COMPLETE | MEDIA_PLAYER_PAUSED ) ) ) {
        mPlayer->setLooping(mLoop);
        mPlayer->setVolume(mLeftVolume, mRightVolume);
        mCurrentState = MEDIA_PLAYER_STARTED;
        status_t ret = mPlayer->start();
        if (ret != NO_ERROR) {
            mCurrentState = MEDIA_PLAYER_STATE_ERROR;
        } else {
            if (mCurrentState == MEDIA_PLAYER_PLAYBACK_COMPLETE) {
                LOGV("playback completed immediately following start()");
            }
        }
        return ret;
    }
    LOGE("start called in state %d", mCurrentState);
    return INVALID_OPERATION;
}

status_t MediaPlayer::stop()
{
    LOGE("++++++++Start stopping");
    Mutex::Autolock _l(mLock);
    if (mCurrentState & MEDIA_PLAYER_STOPPED) return NO_ERROR;
    if ( (mPlayer != 0) && ( mCurrentState & ( MEDIA_PLAYER_STARTED | MEDIA_PLAYER_PREPARED |
                    MEDIA_PLAYER_PAUSED | MEDIA_PLAYER_PLAYBACK_COMPLETE ) ) ) {
        status_t ret = mPlayer->stop();
        if (ret != NO_ERROR) {
            mCurrentState = MEDIA_PLAYER_STATE_ERROR;
        } else {
            mCurrentState = MEDIA_PLAYER_STOPPED;
        }
	    LOGE("++++++++End stopping");
        return ret;
    }
    LOGE("stop called in state %d", mCurrentState);
    return INVALID_OPERATION;
}

status_t MediaPlayer::pause()
{
	LOGE("++++++++Start pausing");
    //Mutex::Autolock _l(mLock);
    if (mCurrentState & (MEDIA_PLAYER_PAUSED|MEDIA_PLAYER_PLAYBACK_COMPLETE))
        return NO_ERROR;
    if ((mPlayer != 0) && (mCurrentState & MEDIA_PLAYER_STARTED)) {
        status_t ret = mPlayer->pause();
        if (ret != NO_ERROR) {
            mCurrentState = MEDIA_PLAYER_STATE_ERROR;
        } else {
            mCurrentState = MEDIA_PLAYER_PAUSED;
        }
		LOGE("++++++++End pausing");
        return ret;
    }
    LOGE("pause called in state %d", mCurrentState);
    return INVALID_OPERATION;
}

bool MediaPlayer::isPlaying()
{
    Mutex::Autolock _l(mLock);
    if (mPlayer != 0) {
        bool temp = false;
        mPlayer->isPlaying(&temp);
        LOGV("isPlaying: %d", temp);
        if ((mCurrentState & MEDIA_PLAYER_STARTED) && ! temp) {
            LOGE("internal/external state mismatch corrected");
            mCurrentState = MEDIA_PLAYER_PAUSED;
        }
        return temp;
    }
    LOGV("isPlaying: no active player");
    return false;
}

status_t MediaPlayer::getVideoWidth(int *w)
{
    LOGV("getVideoWidth");
    Mutex::Autolock _l(mLock);
    if (mPlayer == 0) return INVALID_OPERATION;
    *w = mVideoWidth;
    return NO_ERROR;
}

status_t MediaPlayer::getVideoHeight(int *h)
{
    LOGV("getVideoHeight");
    Mutex::Autolock _l(mLock);
    if (mPlayer == 0) return INVALID_OPERATION;
    *h = mVideoHeight;
    return NO_ERROR;
}

status_t MediaPlayer::getCurrentPosition(int *msec)
{
    LOGV("getCurrentPosition");
    Mutex::Autolock _l(mLock);
    if (mPlayer != 0) {
        if (mCurrentPosition >= 0) {
            LOGE("Using cached seek position: %d", mCurrentPosition);
            *msec = mCurrentPosition;
            return NO_ERROR;
        }
        return mPlayer->getCurrentPosition(msec);
    }
    return INVALID_OPERATION;
}

status_t MediaPlayer::getDuration_l(int *msec)
{
    LOGV("getDuration");
    bool isValidState = (mCurrentState & (MEDIA_PLAYER_PREPARED | MEDIA_PLAYER_STARTED | MEDIA_PLAYER_PAUSED | MEDIA_PLAYER_STOPPED | MEDIA_PLAYER_PLAYBACK_COMPLETE));
    if (mPlayer != 0 && isValidState) {
        status_t ret = NO_ERROR;
        if (mDuration <= 0)
            ret = mPlayer->getDuration(&mDuration);
        if (msec)
            *msec = mDuration;
        return ret;
    }
    LOGE("Attempt to call getDuration without a valid mediaplayer");
    return INVALID_OPERATION;
}

status_t MediaPlayer::getDuration(int *msec)
{
    Mutex::Autolock _l(mLock);
    return getDuration_l(msec);
}

status_t MediaPlayer::seekTo_l(int msec)
{
    LOGE("seekTo %d", msec);
    if ((mPlayer != 0) && ( mCurrentState & ( MEDIA_PLAYER_STARTED | MEDIA_PLAYER_PREPARED | MEDIA_PLAYER_PAUSED |  MEDIA_PLAYER_PLAYBACK_COMPLETE) ) ) {
        if ( msec < 0 ) {
            LOGE("Attempt to seek to invalid position: %d", msec);
            msec = 0;
        } else if ((mDuration > 0) && (msec > mDuration)) {
            LOGW("Attempt to seek to past end of file: request = %d, EOF = %d", msec, mDuration);
            msec = mDuration;
        }

        // cache duration
        mCurrentPosition = msec;
        if (mSeekPosition < 0) {
            getDuration_l(NULL);
            mSeekPosition = msec;
            return mPlayer->seekTo(msec);
        }
        else {
            LOGE("Seek in progress - queue up seekTo[%d]", msec);
            return NO_ERROR;
        }
    }
    LOGE("Attempt to perform seekTo in wrong state: mPlayer=%p, mCurrentState=%u", mPlayer.get(), mCurrentState);
    return INVALID_OPERATION;
}

status_t MediaPlayer::seekTo(int msec)
{
    //mLockThreadId = getThreadId();
    //Mutex::Autolock _l(mLock);
    status_t result = seekTo_l(msec);
    //mLockThreadId = 0;

    return result;
}

status_t MediaPlayer::reset()
{
	LOGE("++++++++Start resetting");
    Mutex::Autolock _l(mLock);
    mLoop = false;
    if (mCurrentState == MEDIA_PLAYER_IDLE) return NO_ERROR;
    mPrepareSync = false;
    if (mPlayer != 0) {
		mLock.unlock(); 
        status_t ret = mPlayer->reset();
		mLock.lock(); 
        if (ret != NO_ERROR) {
            LOGE("reset() failed with return code (%d)", ret);
            mCurrentState = MEDIA_PLAYER_STATE_ERROR;
        } else {
            mCurrentState = MEDIA_PLAYER_IDLE;
        }
		LOGE("++++++++End resetting");
        return ret;
    }
    clear_l();
    return NO_ERROR;
}

status_t MediaPlayer::setAudioStreamType(int type)
{
    LOGV("MediaPlayer::setAudioStreamType");
    Mutex::Autolock _l(mLock);
    if (mStreamType == type) return NO_ERROR;
    if (mCurrentState & ( MEDIA_PLAYER_PREPARED | MEDIA_PLAYER_STARTED |
                MEDIA_PLAYER_PAUSED | MEDIA_PLAYER_PLAYBACK_COMPLETE ) ) {
        // Can't change the stream type after prepare
        LOGE("setAudioStream called in state %d", mCurrentState);
        return INVALID_OPERATION;
    }
    // cache
    mStreamType = type;
    return OK;
}

status_t MediaPlayer::setLooping(int loop)
{
    LOGV("MediaPlayer::setLooping");
    Mutex::Autolock _l(mLock);
    mLoop = (loop != 0);
    if (mPlayer != 0) {
        return mPlayer->setLooping(loop);
    }
    return OK;
}

bool MediaPlayer::isLooping() {
    LOGV("isLooping");
    Mutex::Autolock _l(mLock);
    if (mPlayer != 0) {
        return mLoop;
    }
    LOGV("isLooping: no active player");
    return false;
}

status_t MediaPlayer::setVolume(float leftVolume, float rightVolume)
{
    LOGV("MediaPlayer::setVolume(%f, %f)", leftVolume, rightVolume);
    Mutex::Autolock _l(mLock);
    mLeftVolume = leftVolume;
    mRightVolume = rightVolume;
    if (mPlayer != 0) {
        return mPlayer->setVolume(leftVolume, rightVolume);
    }
    return OK;
}

void MediaPlayer::notify(int msg, int ext1, int ext2)
{
    LOGE("message received msg=%d, ext1=%d, ext2=%d", msg, ext1, ext2);
    bool send = true;
    bool locked = false;

    // TODO: In the future, we might be on the same thread if the app is
    // running in the same process as the media server. In that case,
    // this will deadlock.
    //
    // The threadId hack below works around this for the care of prepare
    // and seekTo within the same process.
    // FIXME: Remember, this is a hack, it's not even a hack that is applied
    // consistently for all use-cases, this needs to be revisited.
    LOGE("Player creating threadID is: %d", mLockThreadId);
    LOGE("Player callback threadID is: %d", getThreadId());
     if (mLockThreadId != getThreadId()) {
        mLock.lock();
        locked = true;
    }

    if (mPlayer == 0) {
        LOGE("notify(%d, %d, %d) callback on disconnected mediaplayer", msg, ext1, ext2);
        if (locked) mLock.unlock();   // release the lock when done.
        return;
    }

    switch (msg) {
    case MEDIA_NOP: // interface test message
        break;
    case MEDIA_PREPARED:
        LOGE("prepared");
        mCurrentState = MEDIA_PLAYER_PREPARED;
        if (mPrepareSync) {
            LOGE("signal application thread");
            mPrepareSync = false;
            mPrepareStatus = NO_ERROR;
            mSignal.signal();
        }
        break;
    case MEDIA_PLAYBACK_COMPLETE:
        LOGV("playback complete");
        if (!mLoop) {
            mCurrentState = MEDIA_PLAYER_PLAYBACK_COMPLETE;
        }
        break;
    case MEDIA_ERROR:
        // Always log errors.
        // ext1: Media framework error code.
        // ext2: Implementation dependant error code.
        LOGE("error (%d, %d)", ext1, ext2);
        mCurrentState = MEDIA_PLAYER_STATE_ERROR;
        if (mPrepareSync)
        {
            LOGV("signal application thread");
            mPrepareSync = false;
            mPrepareStatus = ext1;
            mSignal.signal();
            send = false;
        }
        break;
    case MEDIA_INFO:
        // ext1: Media framework error code.
        // ext2: Implementation dependant error code.
        LOGW("info/warning (%d, %d)", ext1, ext2);
        break;
    case MEDIA_SEEK_COMPLETE:
        LOGE("Received seek complete");
        if (mSeekPosition != mCurrentPosition) {
            LOGE("Executing queued seekTo(%d)", mSeekPosition);
            mSeekPosition = -1;
            seekTo_l(mCurrentPosition); //roger, to avoid dead-lock.
        }
        else {
            LOGE("All seeks complete - return to regularly scheduled program");
            mCurrentPosition = mSeekPosition = -1;
        }
        break;
    case MEDIA_BUFFERING_UPDATE:
        LOGE("buffering %d", ext1);
        break;
    case MEDIA_SET_VIDEO_SIZE:
        LOGE("New video size %d x %d", ext1, ext2);
        mVideoWidth = ext1;
        mVideoHeight = ext2;
        break;
    default:
        LOGV("unrecognized message: (%d, %d, %d)", msg, ext1, ext2);
        break;
    }

    sp<MediaPlayerListener> listener = mListener;
    if (locked) mLock.unlock();

    // this prevents re-entrant calls into client code
    if ((listener != 0) && send) {
        Mutex::Autolock _l(mNotifyLock);
        LOGE("callback application");
        listener->notify(msg, ext1, ext2);
        LOGE("back from callback");
    }
	else
	{
	    LOGE("Mediaplayer.cpp::notify: listener is null");
	}
}

/*static*/ sp<IMemory> MediaPlayer::decode(const char* url, uint32_t *pSampleRate, int* pNumChannels, int* pFormat)
{
/*
    LOGV("decode(%s)", url);
    sp<IMemory> p;
    const sp<IMediaPlayerService>& service = getMediaPlayerService();
    if (service != 0) {
        p = service->decode(url, pSampleRate, pNumChannels, pFormat);
    } else {
        LOGE("Unable to locate media service");
    }
    return p;
*/
   return NULL;
}

void MediaPlayer::died()
{
    LOGV("died");
    notify(MEDIA_ERROR, MEDIA_ERROR_SERVER_DIED, 0);
}

/*static*/ sp<IMemory> MediaPlayer::decode(int fd, int64_t offset, int64_t length, uint32_t *pSampleRate, int* pNumChannels, int* pFormat)
{
/*
    LOGV("decode(%d, %lld, %lld)", fd, offset, length);
    sp<IMemory> p;
    const sp<IMediaPlayerService>& service = getMediaPlayerService();
    if (service != 0) {
        p = service->decode(fd, offset, length, pSampleRate, pNumChannels, pFormat);
    } else {
        LOGE("Unable to locate media service");
    }
    return p;
    */
    return NULL;

}

extern "C" {
#define FLOATING_POINT 1
//#include "fftwrap.h"
}

//static void *ffttable = NULL;

// peeks at the audio data and fills 'data' with the requested kind
// (currently kind=0 returns mono 16 bit PCM data, and kind=1 returns
// 256 point FFT data). Return value is number of samples returned,
// which may be 0.
/*static*/ int MediaPlayer::snoop(short* data, int len, int kind) {
/*
    sp<IMemory> p;
    const sp<IMediaPlayerService>& service = getMediaPlayerService();
    if (service != 0) {
        // Take a peek at the waveform. The returned data consists of 16 bit mono PCM data.
        p = service->snoop();

        if (p == NULL) {
            return 0;
        }

        if (kind == 0) { // return waveform data
            int plen = p->size();
            len *= 2; // number of shorts -> number of bytes
            short *src = (short*) p->pointer();
            if (plen > len) {
                plen = len;
            }
            memcpy(data, src, plen);
            return plen / sizeof(short); // return number of samples
        } else if (kind == 1) {
            // TODO: use a more efficient FFT
            // Right now this uses the speex library, which is compiled to do a float FFT
            if (!ffttable) ffttable = spx_fft_init(512);
            short *usrc = (short*) p->pointer();
            float fsrc[512];
            for (int i=0;i<512;i++)
                fsrc[i] = usrc[i];
            float fdst[512];
            spx_fft_float(ffttable, fsrc, fdst);
            if (len > 512) {
                len = 512;
            }
            len /= 2; // only half the output data is valid
            for (int i=0; i < len; i++)
                data[i] = fdst[i];
            return len;
        }

    } else {
        LOGE("Unable to locate media service");
    }
    */
    return 0;
}

}; // namespace android
