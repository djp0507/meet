/* //device/extlibs/pv/android/AudioTrack_cm.cpp
**
** Copyright 2007, The Android Open Source Project
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
#define LOG_TAG "AudioTrack_cm"

#include <stdint.h>
#include <sys/types.h>
#include <limits.h>

#include <sched.h>
#include <sys/resource.h>

#include "include-pp/private/media/AudioTrackShared.h"
#include "include-pp/AudioSystem_cm.h"
#include "include-pp/MediaBuffer.h"
#include "include-pp/AudioTrack_cm.h"

//#include <private/media/AudioTrackShared.h>
//#include <media/AudioSystem.h>
//#include <media/AudioTrack.h>

#include "include-pp/utils/Log.h"
//#include "include-pp/log.h"
#include "include-pp/binder/Parcel.h"
#include "include-pp/binder/IPCThreadState.h"
#include "include-pp/utils/Timers.h"
#include "include-pp/cutils/atomic.h"

#define LIKELY( exp )       (__builtin_expect( (exp) != 0, true  ))
#define UNLIKELY( exp )     (__builtin_expect( (exp) != 0, false ))

namespace android {
// ---------------------------------------------------------------------------

// static
status_t AudioTrack_cm::getMinFrameCount(
        int* frameCount,
        int streamType,
        uint32_t sampleRate)
{
    int afSampleRate;
    if (AudioSystem_cm::getOutputSamplingRate(&afSampleRate, streamType) != NO_ERROR) {
        return NO_INIT;
    }
    int afFrameCount;
    if (AudioSystem_cm::getOutputFrameCount(&afFrameCount, streamType) != NO_ERROR) {
        return NO_INIT;
    }
    uint32_t afLatency;
    if (AudioSystem_cm::getOutputLatency(&afLatency, streamType) != NO_ERROR) {
        return NO_INIT;
    }

    // Ensure that buffer depth covers at least audio hardware latency
    uint32_t minBufCount = afLatency / ((1000 * afFrameCount) / afSampleRate);
    if (minBufCount < 2) minBufCount = 2;

    *frameCount = (sampleRate == 0) ? afFrameCount * minBufCount :
              afFrameCount * minBufCount * sampleRate / afSampleRate;
    return NO_ERROR;
}

// ---------------------------------------------------------------------------

AudioTrack_cm::AudioTrack_cm()
    : mStatus(NO_INIT)
{
}

AudioTrack_cm::AudioTrack_cm(
        int streamType,
        uint32_t sampleRate,
        int format,
        int channels,
        int frameCount,
        uint32_t flags,
        callback_t cbf,
        void* user,
        int notificationFrames,
        int sessionId)
    : mStatus(NO_INIT)
{
    LOGD("Constructing");
    mStatus = set(streamType, sampleRate, format, channels,
            frameCount, flags, cbf, user, notificationFrames,
            0, false, sessionId);
}

#ifdef USE_KINETO_COMPATIBILITY
// Really dirty hack to give a Froyo-compatible constructor
extern "C" AudioTrack_cm *_ZN7android10AudioTrackC1EijiiijPFviPvS1_ES1_ii(
        AudioTrack_cm *This,
        int streamType,
        uint32_t sampleRate,
        int format,
        int channels,
        int frameCount,
        uint32_t flags,
        AudioTrack_cm::callback_t cbf,
        void* user,
        int notificationFrames,
        int sessionId);
extern "C" AudioTrack_cm *_ZN7android10AudioTrackC1EijiiijPFviPvS1_ES1_i(
        AudioTrack_cm *This,
        int streamType,
        uint32_t sampleRate,
        int format,
        int channels,
        int frameCount,
        uint32_t flags,
        AudioTrack_cm::callback_t cbf,
        void* user,
        int notificationFrames)
{
    return _ZN7android10AudioTrackC1EijiiijPFviPvS1_ES1_ii(This,
        streamType, sampleRate, format, channels,
        frameCount, flags, cbf, user, notificationFrames, 0);
}
#endif

AudioTrack_cm::AudioTrack_cm(
        int streamType,
        uint32_t sampleRate,
        int format,
        int channels,
        const sp<IMemory>& sharedBuffer,
        uint32_t flags,
        callback_t cbf,
        void* user,
        int notificationFrames,
        int sessionId)
    : mStatus(NO_INIT)
{
    mStatus = set(streamType, sampleRate, format, channels,
            0, flags, cbf, user, notificationFrames,
            sharedBuffer, false, sessionId);
}

AudioTrack_cm::~AudioTrack_cm()
{
    LOGV_IF(mSharedBuffer != 0, "Destructor sharedBuffer: %p", mSharedBuffer->pointer());

    if (mStatus == NO_ERROR) {
        // Make sure that callback function exits in the case where
        // it is looping on buffer full condition in obtainBuffer().
        // Otherwise the callback thread will never exit.
        stop();
        if (mAudioTrackThread != 0) {
            mAudioTrackThread->requestExitAndWait();
            mAudioTrackThread.clear();
        }
        mAudioTrack.clear();
        IPCThreadState::self()->flushCommands();
    }
}

#ifdef USE_KINETO_COMPATIBILITY
// another hack, this time for a Froyo-compatible AudioTrack_cm::set method
extern "C" status_t _ZN7android10AudioTrack3setEijiiijPFviPvS1_ES1_iRKNS_2spINS_7IMemoryEEEbi(
        AudioTrack_cm *This,
        int streamType,
        uint32_t sampleRate,
        int format,
        int channels,
        int frameCount,
        uint32_t flags,
        AudioTrack_cm::callback_t cbf,
        void* user,
        int notificationFrames,
        const sp<IMemory>& sharedBuffer,
        bool threadCanCallJava,
        int sessionId);
extern "C" status_t _ZN7android10AudioTrack3setEijiiijPFviPvS1_ES1_iRKNS_2spINS_7IMemoryEEEb(
        AudioTrack_cm *This,
        int streamType,
        uint32_t sampleRate,
        int format,
        int channels,
        int frameCount,
        uint32_t flags,
        AudioTrack_cm::callback_t cbf,
        void* user,
        int notificationFrames,
        const sp<IMemory>& sharedBuffer,
        bool threadCanCallJava)
{
    return _ZN7android10AudioTrack3setEijiiijPFviPvS1_ES1_iRKNS_2spINS_7IMemoryEEEbi(
        This, streamType, sampleRate, format, channels, frameCount, flags, cbf,
        user, notificationFrames, sharedBuffer, threadCanCallJava, 0);
}
#endif

status_t AudioTrack_cm::set(
        int streamType,
        uint32_t sampleRate,
        int format,
        int channels,
        int frameCount,
        uint32_t flags,
        callback_t cbf,
        void* user,
        int notificationFrames,
        const sp<IMemory>& sharedBuffer,
        bool threadCanCallJava,
        int sessionId)
{

    LOGV_IF(sharedBuffer != 0, "sharedBuffer: %p, size: %d", sharedBuffer->pointer(), sharedBuffer->size());

    if (mAudioTrack != 0) {
        LOGE("Track already in use");
        return INVALID_OPERATION;
    }
    LOGD("1");
    int afSampleRate;
    if (AudioSystem_cm::getOutputSamplingRate(&afSampleRate, streamType) != NO_ERROR) {
        return NO_INIT;
    }
    LOGD("2");
    uint32_t afLatency;
    if (AudioSystem_cm::getOutputLatency(&afLatency, streamType) != NO_ERROR) {
        return NO_INIT;
    }

    LOGD("3");
    // handle default values first.
    if (streamType == AudioSystem_cm::DEFAULT) {
        streamType = AudioSystem_cm::MUSIC;
    }
    if (sampleRate == 0) {
        sampleRate = afSampleRate;
    }
    // these below should probably come from the audioFlinger too...
    if (format == 0) {
        format = AudioSystem_cm::PCM_16_BIT;
    }
    if (channels == 0) {
        channels = AudioSystem_cm::CHANNEL_OUT_STEREO;
    }

    LOGD("4");
    // validate parameters
    if (!AudioSystem_cm::isValidFormat(format)) {
        LOGE("Invalid format");
        return BAD_VALUE;
    }
    LOGD("5");

    // force direct flag if format is not linear PCM
    if (!AudioSystem_cm::isLinearPCM(format)) {
        flags |= AudioSystem_cm::OUTPUT_FLAG_DIRECT;
    }

    LOGD("6");
    if (!AudioSystem_cm::isOutputChannel(channels)) {
        LOGE("Invalid channel mask");
        return BAD_VALUE;
    }
    LOGD("7");
    uint32_t channelCount = AudioSystem_cm::popCount(channels);

    LOGD("8");
    audio_io_handle_t output = AudioSystem_cm::getOutput((AudioSystem_cm::stream_type)streamType,
            sampleRate, format, channels, (AudioSystem_cm::output_flags)flags);

    LOGD("9");
    if (output == 0) {
        LOGE("Could not get audio output for stream type %d", streamType);
        return BAD_VALUE;
    }

    mVolume[LEFT] = 1.0f;
    mVolume[RIGHT] = 1.0f;
    mSendLevel = 0;
    mFrameCount = frameCount;
    mNotificationFramesReq = notificationFrames;
    mSessionId = sessionId;
    mAuxEffectId = 0;

    LOGD("10");
    // create the IAudioTrack
    LOGD("Start createTrack, sampleRate:%u, format:%d, channelCount:%d, frameCount:%d, flags:%u",
		    sampleRate, format, channelCount, frameCount, flags);
    status_t status = createTrack(streamType, sampleRate, format, channelCount,
                                  frameCount, flags, sharedBuffer, output, true);
    LOGD("End createTrack");

    if (status != NO_ERROR) {
        return status;
    }

    if (cbf != 0) {
		LOGE("AudioTrack_cm this %p", this);
        mAudioTrackThread = new AudioTrackThread(*this, threadCanCallJava);
        if (mAudioTrackThread == 0) {
          LOGE("Could not create callback thread");
          return NO_INIT;
        }
    }

    mStatus = NO_ERROR;

    mStreamType = streamType;
    mFormat = format;
    mChannels = channels;
    mChannelCount = channelCount;
    mSharedBuffer = sharedBuffer;
    mMuted = false;
    mActive = 0;
    mCbf = cbf;
    mUserData = user;
    mLoopCount = 0;
    mMarkerPosition = 0;
    mMarkerReached = false;
    mNewPosition = 0;
    mUpdatePeriod = 0;
    mFlags = flags;

    return NO_ERROR;
}

status_t AudioTrack_cm::initCheck() const
{
    return mStatus;
}

// -------------------------------------------------------------------------

uint32_t AudioTrack_cm::latency() const
{
    return mLatency;
}

int AudioTrack_cm::streamType() const
{
    return mStreamType;
}

int AudioTrack_cm::format() const
{
    return mFormat;
}

int AudioTrack_cm::channelCount() const
{
    return mChannelCount;
}

uint32_t AudioTrack_cm::frameCount() const
{
    return mCblk->frameCount;
}

int AudioTrack_cm::frameSize() const
{
    if (AudioSystem_cm::isLinearPCM(mFormat)) {
        return channelCount()*((format() == AudioSystem_cm::PCM_8_BIT) ? sizeof(uint8_t) : sizeof(int16_t));
    } else {
        return sizeof(uint8_t);
    }
}

sp<IMemory>& AudioTrack_cm::sharedBuffer()
{
    return mSharedBuffer;
}

// -------------------------------------------------------------------------

void AudioTrack_cm::start()
{	
    sp<AudioTrackThread> t = mAudioTrackThread;
    status_t status;

    LOGD("start %p", this);
    if (t != 0) {
        if (t->exitPending()) {
            if (t->requestExitAndWait() == WOULD_BLOCK) {
                LOGE("AudioTrack_cm::start called from thread");
                return;
            }
        }
        t->mLock.lock();
     }

    if (android_atomic_or(1, &mActive) == 0) {
        mNewPosition = mCblk->server + mUpdatePeriod;
        mCblk->bufferTimeoutMs = MAX_STARTUP_TIMEOUT_MS;
        mCblk->waitTimeMs = 0;
        mCblk->flags &= ~CBLK_DISABLED_ON;
        if (t != 0) {
			LOGD("AudioTrackThread run");
           	t->run("AudioTrackThread", THREAD_PRIORITY_AUDIO_CLIENT);
        } else {
            setpriority(PRIO_PROCESS, 0, THREAD_PRIORITY_AUDIO_CLIENT);
        }

        if (mCblk->flags & CBLK_INVALID_MSK) {
            LOGE("start() track %p invalidated, creating a new one", this);
            // no need to clear the invalid flag as this cblk will not be used anymore
            // force new track creation
            status = DEAD_OBJECT;
        } else {
            status = mAudioTrack->start();
        	LOGD("1. mAudioTrack->start(): %d", status);
        }

        if (status == DEAD_OBJECT) {
            LOGI("start() dead IAudioTrack: creating a new one");
            mSessionId = 0;
            status = createTrack(mStreamType, mCblk->sampleRate, mFormat, mChannelCount,
                                 mFrameCount, mFlags, mSharedBuffer, getOutput(), false);
            if (status == NO_ERROR) {
                status = mAudioTrack->start();
	        	LOGD("2. mAudioTrack->start(): %d", status);
                if (status == NO_ERROR) {
                    mNewPosition = mCblk->server + mUpdatePeriod;
                }
            }
        }
        if (status != NO_ERROR) {
            LOGV("start() failed");
            android_atomic_and(~1, &mActive);
            if (t != 0) {
                t->requestExit();
            } else {
                setpriority(PRIO_PROCESS, 0, ANDROID_PRIORITY_NORMAL);
            }
        }
    }

    if (t != 0) {
        t->mLock.unlock();
    }
}

void AudioTrack_cm::stop()
{
    sp<AudioTrackThread> t = mAudioTrackThread;

    LOGV("stop %p", this);
    if (t != 0) {
        t->mLock.lock();
    }

    if (android_atomic_and(~1, &mActive) == 1) {
        mCblk->cv.signal();
        mAudioTrack->stop();
        // Cancel loops (If we are in the middle of a loop, playback
        // would not stop until loopCount reaches 0).
        setLoop(0, 0, 0);
        // the playback head position will reset to 0, so if a marker is set, we need
        // to activate it again
        mMarkerReached = false;
        // Force flush if a shared buffer is used otherwise audioflinger
        // will not stop before end of buffer is reached.
        if (mSharedBuffer != 0) {
            flush();
        }
        if (t != 0) {
            t->requestExit();
        } else {
            setpriority(PRIO_PROCESS, 0, ANDROID_PRIORITY_NORMAL);
        }
    }

    if (t != 0) {
        t->mLock.unlock();
    }
}

bool AudioTrack_cm::stopped() const
{
    return !mActive;
}

void AudioTrack_cm::flush()
{
    LOGD("flush");

    // clear playback marker and periodic update counter
    mMarkerPosition = 0;
    mMarkerReached = false;
    mUpdatePeriod = 0;


    if (!mActive) {
        mAudioTrack->flush();
        // Release AudioTrack_cm callback thread in case it was waiting for new buffers
        // in AudioTrack_cm::obtainBuffer()
        mCblk->cv.signal();
    }
}

void AudioTrack_cm::pause()
{
    LOGV("pause");
    if (android_atomic_and(~1, &mActive) == 1) {
        mAudioTrack->pause();
    }
}

void AudioTrack_cm::mute(bool e)
{
    mAudioTrack->mute(e);
    mMuted = e;
}

bool AudioTrack_cm::muted() const
{
    return mMuted;
}

status_t AudioTrack_cm::setVolume(float left, float right)
{
    if (left > 1.0f || right > 1.0f) {
        return BAD_VALUE;
    }

    mVolume[LEFT] = left;
    mVolume[RIGHT] = right;

    // write must be atomic
    mCblk->volumeLR = (uint32_t(uint16_t(right * 0x1000)) << 16) | uint16_t(left * 0x1000);

    return NO_ERROR;
}

void AudioTrack_cm::getVolume(float* left, float* right)
{
    if (left != NULL) {
        *left  = mVolume[LEFT];
    }
    if (right != NULL) {
        *right = mVolume[RIGHT];
    }
}

status_t AudioTrack_cm::setAuxEffectSendLevel(float level)
{
    LOGV("setAuxEffectSendLevel(%f)", level);
    if (level > 1.0f) {
        return BAD_VALUE;
    }

    mSendLevel = level;

    mCblk->sendLevel = uint16_t(level * 0x1000);

    return NO_ERROR;
}

void AudioTrack_cm::getAuxEffectSendLevel(float* level)
{
    if (level != NULL) {
        *level  = mSendLevel;
    }
}

status_t AudioTrack_cm::setSampleRate(int rate)
{
    int afSamplingRate;

    if (AudioSystem_cm::getOutputSamplingRate(&afSamplingRate, mStreamType) != NO_ERROR) {
        return NO_INIT;
    }
    // Resampler implementation limits input sampling rate to 2 x output sampling rate.
    if (rate <= 0 || rate > afSamplingRate*2 ) return BAD_VALUE;

    mCblk->sampleRate = rate;
    return NO_ERROR;
}

uint32_t AudioTrack_cm::getSampleRate()
{
    return mCblk->sampleRate;
}

status_t AudioTrack_cm::setLoop(uint32_t loopStart, uint32_t loopEnd, int loopCount)
{
    audio_track_cblk_t* cblk = mCblk;

    Mutex::Autolock _l(cblk->lock);

    if (loopCount == 0) {
        cblk->loopStart = UINT_MAX;
        cblk->loopEnd = UINT_MAX;
        cblk->loopCount = 0;
        mLoopCount = 0;
        return NO_ERROR;
    }

    if (loopStart >= loopEnd ||
        loopEnd - loopStart > cblk->frameCount) {
        LOGE("setLoop invalid value: loopStart %d, loopEnd %d, loopCount %d, framecount %d, user %d", loopStart, loopEnd, loopCount, cblk->frameCount, cblk->user);
        return BAD_VALUE;
    }

    if ((mSharedBuffer != 0) && (loopEnd   > cblk->frameCount)) {
        LOGE("setLoop invalid value: loop markers beyond data: loopStart %d, loopEnd %d, framecount %d",
            loopStart, loopEnd, cblk->frameCount);
        return BAD_VALUE;
    }

    cblk->loopStart = loopStart;
    cblk->loopEnd = loopEnd;
    cblk->loopCount = loopCount;
    mLoopCount = loopCount;

    return NO_ERROR;
}

status_t AudioTrack_cm::getLoop(uint32_t *loopStart, uint32_t *loopEnd, int *loopCount)
{
    if (loopStart != 0) {
        *loopStart = mCblk->loopStart;
    }
    if (loopEnd != 0) {
        *loopEnd = mCblk->loopEnd;
    }
    if (loopCount != 0) {
        if (mCblk->loopCount < 0) {
            *loopCount = -1;
        } else {
            *loopCount = mCblk->loopCount;
        }
    }

    return NO_ERROR;
}

status_t AudioTrack_cm::setMarkerPosition(uint32_t marker)
{
    if (mCbf == 0) return INVALID_OPERATION;

    mMarkerPosition = marker;
    mMarkerReached = false;

    return NO_ERROR;
}

status_t AudioTrack_cm::getMarkerPosition(uint32_t *marker)
{
    if (marker == 0) return BAD_VALUE;

    *marker = mMarkerPosition;

    return NO_ERROR;
}

status_t AudioTrack_cm::setPositionUpdatePeriod(uint32_t updatePeriod)
{
    if (mCbf == 0) return INVALID_OPERATION;

    uint32_t curPosition;
    getPosition(&curPosition);
    mNewPosition = curPosition + updatePeriod;
    mUpdatePeriod = updatePeriod;

    return NO_ERROR;
}

status_t AudioTrack_cm::getPositionUpdatePeriod(uint32_t *updatePeriod)
{
    if (updatePeriod == 0) return BAD_VALUE;

    *updatePeriod = mUpdatePeriod;

    return NO_ERROR;
}

status_t AudioTrack_cm::setPosition(uint32_t position)
{
    Mutex::Autolock _l(mCblk->lock);

    if (!stopped()) return INVALID_OPERATION;

    if (position > mCblk->user) return BAD_VALUE;

    mCblk->server = position;
    mCblk->flags |= CBLK_FORCEREADY_ON;

    return NO_ERROR;
}

status_t AudioTrack_cm::getPosition(uint32_t *position)
{
    if (position == 0) return BAD_VALUE;

    *position = mCblk->server;

    return NO_ERROR;
}

status_t AudioTrack_cm::reload()
{
    if (!stopped()) return INVALID_OPERATION;

    flush();

    mCblk->stepUser(mCblk->frameCount);

    return NO_ERROR;
}

audio_io_handle_t AudioTrack_cm::getOutput()
{
    return AudioSystem_cm::getOutput((AudioSystem_cm::stream_type)mStreamType,
            mCblk->sampleRate, mFormat, mChannels, (AudioSystem_cm::output_flags)mFlags);
}

int AudioTrack_cm::getSessionId()
{
    return mSessionId;
}

status_t AudioTrack_cm::attachAuxEffect(int effectId)
{
    LOGV("attachAuxEffect(%d)", effectId);
    status_t status = mAudioTrack->attachAuxEffect(effectId);
    if (status == NO_ERROR) {
        mAuxEffectId = effectId;
    }
    return status;
}

// -------------------------------------------------------------------------

status_t AudioTrack_cm::createTrack(
        int streamType,
        uint32_t sampleRate,
        int format,
        int channelCount,
        int frameCount,
        uint32_t flags,
        const sp<IMemory>& sharedBuffer,
        audio_io_handle_t output,
        bool enforceFrameCount)
{
    status_t status;
    const sp<IAudioFlinger>& audioFlinger = AudioSystem_cm::get_audio_flinger();
    if (audioFlinger == 0) {
       LOGE("Could not get audioflinger");
       return NO_INIT;
    }

    int afSampleRate;
    if (AudioSystem_cm::getOutputSamplingRate(&afSampleRate, streamType) != NO_ERROR) {
        return NO_INIT;
    }
    int afFrameCount;
    if (AudioSystem_cm::getOutputFrameCount(&afFrameCount, streamType) != NO_ERROR) {
        return NO_INIT;
    }
    uint32_t afLatency;
    if (AudioSystem_cm::getOutputLatency(&afLatency, streamType) != NO_ERROR) {
        return NO_INIT;
    }

    mNotificationFramesAct = mNotificationFramesReq;
    if (!AudioSystem_cm::isLinearPCM(format)) {
        if (sharedBuffer != 0) {
            frameCount = sharedBuffer->size();
        }
    } else {
        // Ensure that buffer depth covers at least audio hardware latency
        uint32_t minBufCount = afLatency / ((1000 * afFrameCount)/afSampleRate);
        if (minBufCount < 2) minBufCount = 2;

        int minFrameCount = (afFrameCount*sampleRate*minBufCount)/afSampleRate;

        if (sharedBuffer == 0) {
            if (frameCount == 0) {
                frameCount = minFrameCount;
            }
            if (mNotificationFramesAct == 0) {
                mNotificationFramesAct = frameCount/2;
            }
            // Make sure that application is notified with sufficient margin
            // before underrun
            if (mNotificationFramesAct > (uint32_t)frameCount/2) {
                mNotificationFramesAct = frameCount/2;
            }
            if (frameCount < minFrameCount) {
                if (enforceFrameCount) {
                    LOGE("Invalid buffer size: minFrameCount %d, frameCount %d", minFrameCount, frameCount);
                    return BAD_VALUE;
                } else {
                    frameCount = minFrameCount;
                }
            }
        } else {
            // Ensure that buffer alignment matches channelcount
            if (((uint32_t)sharedBuffer->pointer() & (channelCount | 1)) != 0) {
                LOGE("Invalid buffer alignement: address %p, channelCount %d", sharedBuffer->pointer(), channelCount);
                return BAD_VALUE;
            }
            frameCount = sharedBuffer->size()/channelCount/sizeof(int16_t);
        }
    }

    sp<IAudioTrack> track = audioFlinger->createTrack(getpid(),
                                                      streamType,
                                                      sampleRate,
                                                      format,
                                                      channelCount,
                                                      frameCount,
                                                      ((uint16_t)flags) << 16,
                                                      sharedBuffer,
                                                      output,
                                                      &mSessionId,
                                                      &status);

    if (track == 0) {
        LOGE("AudioFlinger could not create track, status: %d", status);
        return status;
    }
    sp<IMemory> cblk = track->getCblk();
    if (cblk == 0) {
        LOGE("Could not get control block");
        return NO_INIT;
    }
    mAudioTrack.clear();
    mAudioTrack = track;
    mCblkMemory.clear();
    mCblkMemory = cblk;
    mCblk = static_cast<audio_track_cblk_t*>(cblk->pointer());
	LOGD("mCblk:%p", mCblk);
    mCblk->flags |= CBLK_DIRECTION_OUT;
    if (sharedBuffer == 0) {
        mCblk->buffers = (char*)mCblk + sizeof(audio_track_cblk_t);
	LOGD("mCblk->buffers:%p, sizeof(audio_track_cblk_t):%d", mCblk->buffers, sizeof(audio_track_cblk_t));
    } else {
        mCblk->buffers = sharedBuffer->pointer();
         // Force buffer full condition as data is already present in shared memory
        mCblk->stepUser(mCblk->frameCount);
    }

    mCblk->volumeLR = (uint32_t(uint16_t(mVolume[RIGHT] * 0x1000)) << 16) | uint16_t(mVolume[LEFT] * 0x1000);
	LOGD("mCblk->volumeLR:%d", mCblk->volumeLR);
    mCblk->sendLevel = uint16_t(mSendLevel * 0x1000);
	LOGD("mCblk->sendLevel:%d", mCblk->sendLevel);
    mAudioTrack->attachAuxEffect(mAuxEffectId);
    mCblk->bufferTimeoutMs = MAX_STARTUP_TIMEOUT_MS;
    mCblk->waitTimeMs = 0;
    mRemainingFrames = mNotificationFramesAct;
    mLatency = afLatency + (1000*mCblk->frameCount) / sampleRate;
	LOGD("mLatency:%d", mLatency);
	LOGD("mCblk->frameCount:%d", mCblk->frameCount);
	LOGD("mCblk->sampleRate:%d", mCblk->sampleRate);
	LOGD("mCblk->frameSize:%d", mCblk->frameSize);
	LOGD("mCblk->channelCount:%d", mCblk->channelCount);
    return NO_ERROR;
}

status_t AudioTrack_cm::obtainBuffer(Buffer* audioBuffer, int32_t waitCount)
{
    int active;
    status_t result;
    audio_track_cblk_t* cblk = mCblk;
    uint32_t framesReq = audioBuffer->frameCount;
    uint32_t waitTimeMs = (waitCount < 0) ? cblk->bufferTimeoutMs : WAIT_PERIOD_MS;

LOGD("obtainBuffer:2, waitTimeMs:%d", waitTimeMs);
    audioBuffer->frameCount  = 0;
    audioBuffer->size = 0;

    uint32_t framesAvail = cblk->framesAvailable();

    if (framesAvail == 0) {
	LOGD("cblk:: lock.lock()");
        cblk->lock.lock();
        goto start_loop_here;
        while (framesAvail == 0) {
            active = mActive;
            if (UNLIKELY(!active)) {
                LOGV("Not active and NO_MORE_BUFFERS");
				LOGE("cblk:: lock.unlock()");
                cblk->lock.unlock();
                return NO_MORE_BUFFERS;
            }
            if (UNLIKELY(!waitCount)) {
				LOGD("cblk:: lock.unlock()");
                cblk->lock.unlock();
                return WOULD_BLOCK;
            }
            if (!(cblk->flags & CBLK_INVALID_MSK)) {
				LOGD("cblk::obtainBuffer cv.waitRelative() start");
                result = cblk->cv.waitRelative(cblk->lock, milliseconds(waitTimeMs));
				LOGD("cblk::obtainBuffer cv.waitRelative() end with:%d", result);
            }
            if (cblk->flags & CBLK_INVALID_MSK) {
                LOGW("obtainBuffer() track %p invalidated, creating a new one", this);
                // no need to clear the invalid flag as this cblk will not be used anymore
                cblk->lock.unlock();
                goto create_new_track;
            }
            if (__builtin_expect(result!=NO_ERROR, false)) {
                cblk->waitTimeMs += waitTimeMs;
                if (cblk->waitTimeMs >= cblk->bufferTimeoutMs) {
                    // timing out when a loop has been set and we have already written upto loop end
                    // is a normal condition: no need to wake AudioFlinger up.
                    if (cblk->user < cblk->loopEnd) {
                        LOGW(   "obtainBuffer timed out (is the CPU pegged?) %p "
                                "user=%08x, server=%08x", this, cblk->user, cblk->server);
                        //unlock cblk mutex before calling mAudioTrack->start() (see issue #1617140)
                        cblk->lock.unlock();
                        result = mAudioTrack->start();
                        if (result == DEAD_OBJECT) {
                            LOGE("obtainBuffer() dead IAudioTrack: creating a new one");
create_new_track:
                            mSessionId = 0;
                            result = createTrack(mStreamType, cblk->sampleRate, mFormat, mChannelCount,
                                                 mFrameCount, mFlags, mSharedBuffer, getOutput(), false);
                            if (result == NO_ERROR) {
                                cblk = mCblk;
                                cblk->bufferTimeoutMs = MAX_RUN_TIMEOUT_MS;
                                mAudioTrack->start();
                            }
                        }
						LOGD("cblk:: lock.lock()");
                        cblk->lock.lock();
                    }
                    cblk->waitTimeMs = 0;
                }

                if (--waitCount == 0) {
                    cblk->lock.unlock();
                    return TIMED_OUT;
                }
            }
            // read the server count again
        start_loop_here:
            framesAvail = cblk->framesAvailable_l();
        }
        cblk->lock.unlock();
    }

    // restart track if it was disabled by audioflinger due to previous underrun
    if (cblk->flags & CBLK_DISABLED_MSK) {
        cblk->flags &= ~CBLK_DISABLED_ON;
        mAudioTrack->start();
    }

    cblk->waitTimeMs = 0;

    if (framesReq > framesAvail) {
        framesReq = framesAvail;
    }

    uint32_t u = cblk->user;
    uint32_t bufferEnd = cblk->userBase + cblk->frameCount;

    if (u + framesReq > bufferEnd) {
        framesReq = bufferEnd - u;
    }

    audioBuffer->flags = mMuted ? Buffer::MUTE : 0;
    audioBuffer->channelCount = mChannelCount;
    audioBuffer->frameCount = framesReq;
    audioBuffer->size = framesReq * cblk->frameSize;
    if (AudioSystem_cm::isLinearPCM(mFormat)) {
        audioBuffer->format = AudioSystem_cm::PCM_16_BIT;
    } else {
        audioBuffer->format = mFormat;
    }
    audioBuffer->raw = (int8_t *)cblk->buffer(u);
    active = mActive;
    return active ? status_t(NO_ERROR) : status_t(STOPPED);
}

void AudioTrack_cm::releaseBuffer(Buffer* audioBuffer)
{
    audio_track_cblk_t* cblk = mCblk;
	
	LOGD("cblk:: releaseBuffer with framecount:%d", audioBuffer->frameCount);
    cblk->stepUser(audioBuffer->frameCount);
}

// -------------------------------------------------------------------------

ssize_t AudioTrack_cm::write(const void* buffer, size_t userSize)
{
    if (mSharedBuffer != 0) return INVALID_OPERATION;

    if (ssize_t(userSize) < 0) {
        // sanity-check. user is most-likely passing an error code.
        LOGE("AudioTrack_cm::write(buffer=%p, size=%u (%d)",
                buffer, userSize, userSize);
        return BAD_VALUE;
    }

    LOGV("write %p: %d bytes, mActive=%d", this, userSize, mActive);

    ssize_t written = 0;
    const int8_t *src = (const int8_t *)buffer;
    Buffer audioBuffer;

    do {
        audioBuffer.frameCount = userSize/frameSize();

        // Calling obtainBuffer() with a negative wait count causes
        // an (almost) infinite wait time.
        status_t err = obtainBuffer(&audioBuffer, -1);
        if (err < 0) {
            // out of buffers, return #bytes written
            if (err == status_t(NO_MORE_BUFFERS))
                break;
            return ssize_t(err);
        }

        size_t toWrite;

        if (mFormat == AudioSystem_cm::PCM_8_BIT && !(mFlags & AudioSystem_cm::OUTPUT_FLAG_DIRECT)) {
            // Divide capacity by 2 to take expansion into account
            toWrite = audioBuffer.size>>1;
            // 8 to 16 bit conversion
            int count = toWrite;
            int16_t *dst = (int16_t *)(audioBuffer.i8);
            while(count--) {
                *dst++ = (int16_t)(*src++^0x80) << 8;
            }
        } else {
            toWrite = audioBuffer.size;
            memcpy(audioBuffer.i8, src, toWrite);
            src += toWrite;
        }
        userSize -= toWrite;
        written += toWrite;

        releaseBuffer(&audioBuffer);
    } while (userSize);

    return written;
}

// -------------------------------------------------------------------------

bool AudioTrack_cm::processAudioBuffer(const sp<AudioTrackThread>& thread)
{
    Buffer audioBuffer;
    uint32_t frames;
    size_t writtenSize;

/*
	{
		int i=0;
		while(1)
		{
			MediaBuffer* p = new MediaBuffer(10);
			LOGE("Sleepingxx:%d",++i);
			usleep(1000000);
			delete p;
		}
	}
*/	

    // Manage underrun callback
    if (mActive && (mCblk->framesReady() == 0)) {
        LOGD("Underrun user: %x, server: %x, flags %04x", mCblk->user, mCblk->server, mCblk->flags);
        if ((mCblk->flags & CBLK_UNDERRUN_MSK) == CBLK_UNDERRUN_OFF) {
            mCbf(EVENT_UNDERRUN, mUserData, 0);
            if (mCblk->server == mCblk->frameCount) {
                mCbf(EVENT_BUFFER_END, mUserData, 0);
            }
            mCblk->flags |= CBLK_UNDERRUN_ON;
            if (mSharedBuffer != 0) return false;
        }
    }

    // Manage loop end callback
    while (mLoopCount > mCblk->loopCount) {
        int loopCount = -1;
        mLoopCount--;
        if (mLoopCount >= 0) loopCount = mLoopCount;

        mCbf(EVENT_LOOP_END, mUserData, (void *)&loopCount);
    }

    // Manage marker callback
    if (!mMarkerReached && (mMarkerPosition > 0)) {
        if (mCblk->server >= mMarkerPosition) {
            mCbf(EVENT_MARKER, mUserData, (void *)&mMarkerPosition);
            mMarkerReached = true;
        }
    }

    // Manage new position callback
    if (mUpdatePeriod > 0) {
        while (mCblk->server >= mNewPosition) {
            mCbf(EVENT_NEW_POS, mUserData, (void *)&mNewPosition);
            mNewPosition += mUpdatePeriod;
        }
    }

    // If Shared buffer is used, no data is requested from client.
    if (mSharedBuffer != 0) {
        frames = 0;
    } else {
        frames = mRemainingFrames;
    }

    do {

        audioBuffer.frameCount = frames;

        // Calling obtainBuffer() with a wait count of 1
        // limits wait time to WAIT_PERIOD_MS. This prevents from being
        // stuck here not being able to handle timed events (position, markers, loops).
        status_t err = obtainBuffer(&audioBuffer, 1);
        if (err < NO_ERROR) {
LOGE("processAudioBuffer:6.1, err:%d", err);
            if (err != TIMED_OUT) {
                LOGE_IF(err != status_t(NO_MORE_BUFFERS), "Error obtaining an audio buffer, giving up.");
                return false;
            }
            break;
        }
        if (err == status_t(STOPPED)) return false;

        // Divide buffer size by 2 to take into account the expansion
        // due to 8 to 16 bit conversion: the callback must fill only half
        // of the destination buffer
        if (mFormat == AudioSystem_cm::PCM_8_BIT && !(mFlags & AudioSystem_cm::OUTPUT_FLAG_DIRECT)) {
            audioBuffer.size >>= 1;
        }

        size_t reqSize = audioBuffer.size;
		LOGD("cblk:: fillbuffer with request size:%d", reqSize);
        mCbf(EVENT_MORE_DATA, mUserData, &audioBuffer);
        writtenSize = audioBuffer.size;
		LOGD("cblk:: fillbuffer with written size:%d", writtenSize);

        // Sanity check on returned size
        if (ssize_t(writtenSize) <= 0) {
            // The callback is done filling buffers
            // Keep this thread going to handle timed events and
            // still try to get more data in intervals of WAIT_PERIOD_MS
            // but don't just loop and block the CPU, so wait
            usleep(WAIT_PERIOD_MS*1000);
            break;
        }
        if (writtenSize > reqSize) writtenSize = reqSize;

        if (mFormat == AudioSystem_cm::PCM_8_BIT && !(mFlags & AudioSystem_cm::OUTPUT_FLAG_DIRECT)) {
			LOGD("cblk:: convert");
            // 8 to 16 bit conversion
            const int8_t *src = audioBuffer.i8 + writtenSize-1;
            int count = writtenSize;
            int16_t *dst = audioBuffer.i16 + writtenSize-1;
            while(count--) {
                *dst-- = (int16_t)(*src--^0x80) << 8;
            }
            writtenSize <<= 1;
        }

        audioBuffer.size = writtenSize;
        // NOTE: mCblk->frameSize is not equal to AudioTrack_cm::frameSize() for
        // 8 bit PCM data: in this case,  mCblk->frameSize is based on a sampel size of
        // 16 bit.
        audioBuffer.frameCount = writtenSize/mCblk->frameSize;

        frames -= audioBuffer.frameCount;

        releaseBuffer(&audioBuffer);
    }
    while (frames);

    if (frames == 0) {
        mRemainingFrames = mNotificationFramesAct;
    } else {
        mRemainingFrames = frames;
    }
    return true;
}

status_t AudioTrack_cm::dump(int fd, const Vector<String16>& args) const
{

    const size_t SIZE = 256;
    char buffer[SIZE];
    String8 result;

    result.append(" AudioTrack_cm::dump\n");
    snprintf(buffer, 255, "  stream type(%d), left - right volume(%f, %f)\n", mStreamType, mVolume[0], mVolume[1]);
    result.append(buffer);
    snprintf(buffer, 255, "  format(%d), channel count(%d), frame count(%d)\n", mFormat, mChannelCount, mCblk->frameCount);
    result.append(buffer);
    snprintf(buffer, 255, "  sample rate(%d), status(%d), muted(%d)\n", (mCblk == 0) ? 0 : mCblk->sampleRate, mStatus, mMuted);
    result.append(buffer);
    snprintf(buffer, 255, "  active(%d), latency (%d)\n", mActive, mLatency);
    result.append(buffer);
    ::write(fd, result.string(), result.size());
    return NO_ERROR;
}

// =========================================================================

AudioTrack_cm::AudioTrackThread::AudioTrackThread(AudioTrack_cm& receiver, bool bCanCallJava)
    : Thread(bCanCallJava), mReceiver(receiver)
{
}

bool AudioTrack_cm::AudioTrackThread::threadLoop()
{
	LOGD("mReceiver: %p", &mReceiver);
	LOGD("Outside this: %p", this);
    return mReceiver.processAudioBuffer(this);
}

status_t AudioTrack_cm::AudioTrackThread::readyToRun()
{
    return NO_ERROR;
}

void AudioTrack_cm::AudioTrackThread::onFirstRef()
{
}

// =========================================================================

audio_track_cblk_t::audio_track_cblk_t()
    : lock(Mutex::SHARED), cv(Condition::SHARED), user(0), server(0),
    userBase(0), serverBase(0), buffers(0), frameCount(0),
    loopStart(UINT_MAX), loopEnd(UINT_MAX), loopCount(0), volumeLR(0),
    flags(0), sendLevel(0)
{
}

uint32_t audio_track_cblk_t::stepUser(uint32_t frameCount)
{
	LOGD("cblk::stepUser");
    uint32_t u = this->user;

    u += frameCount;
    // Ensure that user is never ahead of server for AudioRecord
    if (flags & CBLK_DIRECTION_MSK) {
        // If stepServer() has been called once, switch to normal obtainBuffer() timeout period
        if (bufferTimeoutMs == MAX_STARTUP_TIMEOUT_MS-1) {
            bufferTimeoutMs = MAX_RUN_TIMEOUT_MS;
        }
    } else if (u > this->server) {
        LOGW("stepServer occured after track reset");
        u = this->server;
    }

    if (u >= userBase + this->frameCount) {
        userBase += this->frameCount;
    }

	LOGD("cblk::stepUser update this->user to %d", u);
    this->user = u;

    // Clear flow control error condition as new data has been written/read to/from buffer.
	LOGD("cblk::stepUser Clear flow control error condition on flags:%d", flags);
    flags &= ~CBLK_UNDERRUN_MSK;

	{
		uint32_t framesReq = 10240;
		uint32_t framesReady = user-server;
		LOGD("cblk::stepUser test, s:%d, u:%d", server, user);

		LOGD("cblk::stepUser test, framesReady:%d", framesReady);
		if (LIKELY(framesReady)) {
			uint32_t s = server;
			uint32_t bufferEnd = serverBase + this->frameCount;
			LOGD("cblk::stepUser test, serverBase:%d, frameCount:%d", serverBase, this->frameCount);

			bufferEnd = (loopEnd < bufferEnd) ? loopEnd : bufferEnd;
			LOGD("cblk::stepUser test, bufferEnd:%u, loopEnd:%u, check:%d", bufferEnd, loopEnd, (loopEnd < bufferEnd));
			
			LOGD("cblk::stepUser test, framesReq:%d, framesReady:%d", framesReq, framesReady);
			if (framesReq > framesReady) {
			    framesReq = framesReady;
				LOGD("cblk::stepUser test 1, framesReq:%d", framesReq);
			}
			LOGD("cblk::stepUser test, s + framesReq:%d, bufferEnd:%d", s + framesReq, bufferEnd);
			if (s + framesReq > bufferEnd) {
			    framesReq = bufferEnd - s;
				LOGD("cblk::stepUser test 2, framesReq:%d", framesReq);
			}

			//buffer->raw = getBuffer(s, framesReq);
			//if (buffer->raw == 0) goto getNextBuffer_exit;

			//buffer->frameCount = framesReq;
			//return NO_ERROR;
		}
	}

    return u;
}

bool audio_track_cblk_t::stepServer(uint32_t frameCount)
{

	LOGD("cblk::stepServer");
    // the code below simulates lock-with-timeout
    // we MUST do this to protect the AudioFlinger server
    // as this lock is shared with the client.
    status_t err;

	LOGD("cblk:: lock.lock()");
    err = lock.tryLock();
    if (err == -EBUSY) { // just wait a bit
        usleep(1000);
        err = lock.tryLock();
    }
    if (err != NO_ERROR) {
        // probably, the client just died.
        return false;
    }

    uint32_t s = this->server;

    s += frameCount;
    if (flags & CBLK_DIRECTION_MSK) {
        // Mark that we have read the first buffer so that next time stepUser() is called
        // we switch to normal obtainBuffer() timeout period
        if (bufferTimeoutMs == MAX_STARTUP_TIMEOUT_MS) {
            bufferTimeoutMs = MAX_STARTUP_TIMEOUT_MS - 1;
        }
        // It is possible that we receive a flush()
        // while the mixer is processing a block: in this case,
        // stepServer() is called After the flush() has reset u & s and
        // we have s > u
        if (s > this->user) {
            LOGW("stepServer occured after track reset");
            s = this->user;
        }
    }

    if (s >= loopEnd) {
        LOGW_IF(s > loopEnd, "stepServer: s %u > loopEnd %u", s, loopEnd);
        s = loopStart;
        if (--loopCount == 0) {
            loopEnd = UINT_MAX;
            loopStart = UINT_MAX;
        }
    }
    if (s >= serverBase + this->frameCount) {
        serverBase += this->frameCount;
    }

	LOGD("cblk:: update this->server:%d", s);
    this->server = s;
	
	LOGD("cblk:: cv signal");
    cv.signal();
	
	LOGD("cblk:: lock.unlock()");
    lock.unlock();
    return true;
}

void* audio_track_cblk_t::buffer(uint32_t offset) const
{
    return (int8_t *)this->buffers + (offset - userBase) * this->frameSize;
}

uint32_t audio_track_cblk_t::framesAvailable()
{
    Mutex::Autolock _l(lock);
    return framesAvailable_l();
}

uint32_t audio_track_cblk_t::framesAvailable_l()
{
    uint32_t u = this->user;
    uint32_t s = this->server;

    LOGD("cblk::framesAvailable_l u:%d, s:%d, frameCount:%d", this->user, this->server, frameCount);

    if (flags & CBLK_DIRECTION_MSK) {
        uint32_t limit = (s < loopStart) ? s : loopStart;
		LOGD("cblk::framesAvailable_l ret1:%d", limit + frameCount - u);
        return limit + frameCount - u;
    } else {
		LOGD("cblk::framesAvailable_l ret2:%d", frameCount + u - s);
        return frameCount + u - s;
    }
}

uint32_t audio_track_cblk_t::framesReady()
{
    uint32_t u = this->user;
    uint32_t s = this->server;
    LOGD("cblk::framesReady, u:%u, s:%u, loopStart:%u, loopEnd:%u, loopCount:%d", u, s, loopStart, loopEnd, loopCount);

    if (flags & CBLK_DIRECTION_MSK) {
        if (u < loopEnd) {
		LOGD("cblk::framesReady 1 ret:%u, loopStart:%u, loopEnd:%u, loopCount:%d, ", u - s, loopStart, loopEnd, loopCount);
            return u - s;
        } else {
            Mutex::Autolock _l(lock);
            if (loopCount >= 0) {
		LOGD("cblk::framesReady 2 ret:%u, loopStart:%u, loopEnd:%u, loopCount:%d, ", ((loopEnd - loopStart)*loopCount + u - s), loopStart, loopEnd, loopCount);
                return (loopEnd - loopStart)*loopCount + u - s;
            } else {
                return UINT_MAX;
            }
        }
    } else {
		LOGD("cblk::framesReady 3 ret:%u, loopStart:%u, loopEnd:%u, loopCount:%u, ", (s - u), loopStart, loopEnd, loopCount);
        return s - u;
    }
}

// -------------------------------------------------------------------------

}; // namespace android

