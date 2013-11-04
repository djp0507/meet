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

//#define LOG_NDEBUG 0
#define LOG_TAG "AudioPlayer"

#include "platform-pp/PPPlatForm.h"

#include "include-pp/AudioPlayer.h"
#include "include-pp/MediaDefs.h"
#include "include-pp/MediaSource.h"
//#include "include-pp/AudioTrack.h"
#include "include-pp/sf/MetaData.h"
#include "include-pp/MediaDebug.h"
#include "include-pp/Utils.h"

#include "include-pp/utils/Log.h"
//#include "include-pp/log.h"
//#include <binder/IPCThreadState.h>
//#include <media/AudioTrack.h>
//#include <media/stagefright/MediaDebug.h>

namespace android {


//AudioPlayer::AudioPlayer(const sp<MediaPlayerBase::AudioSink> &audioSink)
AudioPlayer::AudioPlayer(const sp<AudioSink> &audioSink)
    : //mAudioTrack(NULL),
      mInputBuffer(NULL),
      mSampleRate(0),
      mLatencyUs(0),
      mFrameSize(0),
      mNumFramesPlayed(0),
      mPositionTimeMediaUs(-1),
      mPositionTimeRealUs(-1),
      mSeeking(false),
      mStopping(false),
      mReachedEOS(false),
      mFinalStatus(OK),
      mStarted(false),
      mAudioSink(audioSink),
      mListener(NULL),
      mRunningCompatibilityTest(false){
      
      LOGD("memory allocation at %p", this);
}

AudioPlayer::~AudioPlayer() {

	LOGD("memory release at %p", this);
    if (mStarted) {
        stop();
    }
}

void AudioPlayer::setSource(const sp<MediaSource> &source) {
    CHECK_EQ(mSource, NULL);
    mSource = source;
}

status_t AudioPlayer::start(bool sourceAlreadyStarted) {
    CHECK(!mStarted);
    CHECK(mSource != NULL);

    status_t err;
    if (!sourceAlreadyStarted) {
        err = mSource->start();

        if (err != OK) {
            return err;
        }
    }

	err = initAudioSink();
	if(err!=OK) return err;
	/*
	sp<MetaData> format = mSource->getFormat();
	const char *mime;
	bool success = format->findCString(kKeyMIMEType, &mime);
	CHECK(success);
	CHECK(!strcasecmp(mime, MEDIA_MIMETYPE_AUDIO_RAW));
	LOGE("********audio format is %s", mime);

	success = format->findInt32(kKeySampleRate, &mSampleRate);
	CHECK(success);
	LOGE("********audio sample rate is %d", mSampleRate);

	int32_t numChannels;
	success = format->findInt32(kKeyChannelCount, &numChannels);
	CHECK(success);
	LOGE("********audio channel count is %d", numChannels);

	if (mAudioSink.get() != NULL)
	{
		LOGE("start open audio sink");
		status_t err = mAudioSink->open(
		mSampleRate, numChannels, AudioSystem::PCM_16_BIT,
		DEFAULT_AUDIOSINK_BUFFERCOUNT, &AudioPlayer::AudioSinkCallback, this);
		LOGE("end open audio sink");
		if (err != OK)
		{
			//LOGE("********open audio sink failed");
			mSource->stop();
			return err;
		}
		else
		{
			//LOGE("********open audio sink success");
		}

		mLatencyUs = (int64_t)mAudioSink->latency() * 1000;
		mFrameSize = mAudioSink->frameSize();

		mAudioSink->start();
	}
	else
	{
		//LOGE("********create new audio track");
		mAudioTrack = new PPAudioTrack(
							AudioSystem::MUSIC, 
							mSampleRate,
							AudioSystem::PCM_16_BIT,
							(numChannels == 2)? AudioSystem::CHANNEL_OUT_STEREO : AudioSystem::CHANNEL_OUT_MONO,
							0,
							0,
							&AudioCallback,
							this,
							0);

		if ((err = mAudioTrack->initCheck()) != OK) {
			delete mAudioTrack;
			mAudioTrack = NULL;

			mSource->stop();

			return err;
		}
		//LOGE("********create audio track success");

		mLatencyUs = (int64_t)mAudioTrack->latency() * 1000;
		//LOGE("audio latency is %d", mLatencyUs);
		mFrameSize = mAudioTrack->frameSize();
		//LOGE("audio frame size is %d", mFrameSize);

		//LOGE("starting mAudioTrack");
		mAudioTrack->start();
		//LOGE("end mAudioTrack");
	}
	*/
    mStarted = true;

    return OK;
}

void AudioPlayer::pause() {
    CHECK(mStarted);
    if (mAudioSink.get() != NULL) {
        mAudioSink->pause();
    } else {
        //mAudioTrack->stop();
    }
}

void AudioPlayer::resume() {
    CHECK(mStarted);

    if (mAudioSink.get() != NULL) {
        mAudioSink->start();
    } else {
        //mAudioTrack->start();
    }
}

void AudioPlayer::stop() {
    CHECK(mStarted);
	mStopping = true;
	LOGD("stop 1");
    if (mAudioSink.get() != NULL) {
	LOGD("stop 2");
        mAudioSink->flush();
        mAudioSink->stop();
		mAudioSink->close();
    } else {
	LOGD("stop 4.1");
        //mAudioTrack->stop();

        //delete mAudioTrack;
        //mAudioTrack = NULL;
    }

    // Make sure to release any buffer we hold onto so that the
    // source is able to stop().
    if (mInputBuffer != NULL) {
        LOGW("AudioPlayer releasing input buffer.");
        mInputBuffer->release();
        mInputBuffer = NULL;
    }
	LOGD("stop 4.2");

    mSource->stop();
	LOGD("stop 5");
	
    // The following hack is necessary to ensure that the OMX
    // component is completely released by the time we may try
    // to instantiate it again.
    wp<MediaSource> tmp = mSource;
	LOGD("stop 5.1");
    mSource.clear();
	LOGD("stop 5.2");
	
    while (tmp.promote() != NULL) {
	    LOGV("sleep");
        usleep(1000);
    }
	LOGD("stop 5.3");
    //IPCThreadState::self()->flushCommands();
	OMXWrapper::FlushCommands();

	LOGD("stop 6");
    mNumFramesPlayed = 0;
    mPositionTimeMediaUs = -1;
    mPositionTimeRealUs = -1;
    mSeeking = false;
    mReachedEOS = false;
    mFinalStatus = OK;
    mStarted = false;
	//mStopping = false;
}

// static
void AudioPlayer::AudioCallback(int event, void *user, void *info) {
    static_cast<AudioPlayer *>(user)->AudioCallback(event, info);
}

bool AudioPlayer::isSeeking() {
    Mutex::Autolock autoLock(mLock);
    return mSeeking;
}

bool AudioPlayer::reachedEOS(status_t *finalStatus) {
    *finalStatus = OK;

    Mutex::Autolock autoLock(mLock);
    *finalStatus = mFinalStatus;
    return mReachedEOS;
}

// static
size_t AudioPlayer::AudioSinkCallback(
        //MediaPlayerBase::AudioSink *audioSink,
        AudioSink *audioSink,
        void *buffer, size_t size, void *cookie) {
    AudioPlayer *me = (AudioPlayer *)cookie;

    return me->fillBuffer(buffer, size);
}

void AudioPlayer::AudioCallback(int event, void *info) {
    if (event != IPPAudioTrack::EVENT_MORE_DATA) {
        return;
    }

    //PPAudioTrack::Buffer *buffer = (PPAudioTrack::Buffer *)info;
    IPPAudioTrack::Buffer *buffer = (IPPAudioTrack::Buffer *)info;
    size_t numBytesWritten = fillBuffer(buffer->raw, buffer->size);

    buffer->size = numBytesWritten;
}

size_t AudioPlayer::fillBuffer(void *data, size_t size) {
	
    //if (mNumFramesPlayed == 0) {
        //LOGE("AudioCallback");
    //}

    if (mReachedEOS) {
        return 0;
    }
	
    LOGD("start filling buffer with size: %d", size);

    size_t size_done = 0;
    size_t size_remaining = size;
    while (size_remaining > 0)
	{
	    LOGD("mStopping:%d", mStopping);
		if(mStopping)
		{
			if(mInputBuffer!=NULL)
			{
		        mInputBuffer->release();
			    LOGD("release mInputBuffer 3, refcount:%d", mInputBuffer->refcount());
		        mInputBuffer = NULL;
			}
			return 0;
		}
	    MediaSource::ReadOptions options;
        {
            Mutex::Autolock autoLock(mLock);

            if (mSeeking) {
				LOGD("enter seek mSeekTimeUs: %lld", mSeekTimeUs);
                options.setSeekTo(mSeekTimeUs);

                if (mInputBuffer != NULL) {
                    mInputBuffer->release();
                    mInputBuffer = NULL;
                }

				size_done=0;
				size_remaining = size;

                //mSeeking = false;
            }
        }

        if (mInputBuffer == NULL) {
		    //LOGE("start reading audio data into mInputBuffer");
            status_t err = mSource->read(&mInputBuffer, &options);
		    //LOGE("Reading audio data with ret code:%d",err);

            CHECK((err == OK && mInputBuffer != NULL)
                   || (err != OK && mInputBuffer == NULL));
			
			if (err == INFO_FORMAT_CHANGED) {
                LOGW("AudioSource signalled format change.");
				if(mInputBuffer)
				{
					mInputBuffer->release();
                    mInputBuffer = NULL;
				}
				if (mAudioSink.get() != NULL) {
			        mAudioSink->stop();
			        mAudioSink->close();
				}
				if(initAudioSink()==OK)
				{
	                break;
				}
				else
				{
	                LOGE("reconfigure audio sink failed.");
				    return 0;
				}
            }

            Mutex::Autolock autoLock(mLock);

            if (err != OK) {
                mReachedEOS = true;
                mFinalStatus = err;
				if(err != ERROR_END_OF_STREAM)
				{
					LOGE("reading audio data failed:%d", err);
				}
                break;
            }
		    //LOGE("end reading audio data with len: %d", mInputBuffer->range_length());

            CHECK(mInputBuffer->meta_data()->findInt64(
                        kKeyTime, &mPositionTimeMediaUs));
		    if(mSeeking)
		    {
				//seeking is in progress, need to drop old audio data.
				LOGW("Ignore old audio data");
	            mInputBuffer->release();
                mInputBuffer = NULL;
                mSeeking = false;
                continue;
        	}
			
            mPositionTimeRealUs =
                ((mNumFramesPlayed + size_done / mFrameSize) * 1000000)
                    / mSampleRate;

            //LOGE("buffer->size() = %d, mPositionTimeMediaUs=%.2f mPositionTimeRealUs=%.2f mLatencyUs=%.2f", 
            	//mInputBuffer->range_length(),
                //mPositionTimeMediaUs / 1E6, mPositionTimeRealUs / 1E6, mLatencyUs / 1E6);
			
        }

        if (mInputBuffer->range_length() == 0) {
		    LOGD("release mInputBuffer 1, refcount:%d", mInputBuffer->refcount());
            mInputBuffer->release();
            mInputBuffer = NULL;
            continue;
        }

        size_t copy = size_remaining;
        if (copy > mInputBuffer->range_length()) {
            copy = mInputBuffer->range_length();
        }

		//LOGE("start copy data with size: %d",copy);
        memcpy((char *)data + size_done,
               (const char *)mInputBuffer->data() + mInputBuffer->range_offset(),
               copy);

        mInputBuffer->set_range(mInputBuffer->range_offset() + copy,
                                mInputBuffer->range_length() - copy);

        size_done += copy;
        size_remaining -= copy;
    }

    Mutex::Autolock autoLock(mLock);
    mNumFramesPlayed += size_done / mFrameSize;

    LOGD("finish filling buffer with size: %d", size_done);

	if(mRunningCompatibilityTest && size_done>0)
	{
		bool ret = false;
		if(size_done > 1024)
		{
			ret = true;
		}
		/*
		char path[100];
		sprintf(path, "/data/data/com.pplive.androidphone/lib/libaudio.so");
		int32_t len = 1024;//set default check audio data len.
		uint8_t* src = (uint8_t*)malloc(len);
		if(src == NULL)
		{
			LOGE("memory runs out");
			
			//notify check result
			notifyListener_l(MEDIA_COMPATIBILITY_TEST_COMPLETE, 1, ret); //0:video, 1:audio
			
			//exit awesomeplayer.
			mReachedEOS = true;
			return 0;
		}
		loadFrame(src, len, path);
		
		//check decoded audio data
		//sprintf(path, "/data/data/com.pplive.androidphone/libaudio.so");
		//saveFrame(data, len, path);
		
		double psnr = computePSNR(src, (uint8_t*)data, len);

		LOGE("Audio PSNR:%llf", psnr);
		
		if(psnr > 27) //27 is lowest quality value can be accepted. 
		{
			ret = true;
		}
		*/
		
		// notify check result
        notifyListener_l(MEDIA_COMPATIBILITY_TEST_COMPLETE, 1, ret); //0:video, 1:audio

		// wait compatible test done
		// while(mRunningCompatibilityTest) usleep(10000);//0.01sec
		
		// exit audioplayer.
        mReachedEOS = true;

		if(mInputBuffer!=NULL)
		{
	        mInputBuffer->release();
		    LOGD("release mInputBuffer 2, refcount:%d", mInputBuffer->refcount());
	        mInputBuffer = NULL;
		}
			
        return 0;
	}
    return size_done;
}

int64_t AudioPlayer::getRealTimeUs() {
    Mutex::Autolock autoLock(mLock);
    return getRealTimeUsLocked();
}

int64_t AudioPlayer::getRealTimeUsLocked() const {
    return -mLatencyUs + (mNumFramesPlayed * 1000000) / mSampleRate;
}

int64_t AudioPlayer::getMediaTimeUs() {
    Mutex::Autolock autoLock(mLock);

    if (mPositionTimeMediaUs < 0 || mPositionTimeRealUs < 0) {
        return 0;
    }

    int64_t realTimeOffset = getRealTimeUsLocked() - mPositionTimeRealUs;
    if (realTimeOffset < 0) {
        realTimeOffset = 0;
    }

    return mPositionTimeMediaUs + realTimeOffset;
}

bool AudioPlayer::getMediaTimeMapping(
        int64_t *realtime_us, int64_t *mediatime_us) {
    Mutex::Autolock autoLock(mLock);

    *realtime_us = mPositionTimeRealUs;
    *mediatime_us = mPositionTimeMediaUs;

    return mPositionTimeRealUs != -1 && mPositionTimeMediaUs != -1;
}

status_t AudioPlayer::seekTo(int64_t time_us) {
    Mutex::Autolock autoLock(mLock);

    mSeeking = true;
    mReachedEOS = false;
    mSeekTimeUs = time_us;

    if (mAudioSink != NULL) {
        mAudioSink->flush();
    } else {
        //mAudioTrack->flush();
    }

    return OK;
}

status_t AudioPlayer::initAudioSink() {
	LOGD("start initAudioSink");
	
    sp<MetaData> format = mSource->getFormat();
    const char *mime;
    bool success = format->findCString(kKeyMIMEType, &mime);
    CHECK(success);
    CHECK(!strcasecmp(mime, MEDIA_MIMETYPE_AUDIO_RAW));
    LOGD("********audio format is %s", mime);

    success = format->findInt32(kKeySampleRate, &mSampleRate);
    CHECK(success);
    LOGD("********audio sample rate is %d", mSampleRate);

    int32_t numChannels;
    success = format->findInt32(kKeyChannelCount, &numChannels);
    CHECK(success);
    LOGD("********audio channel count is %d", numChannels);
	status_t err;
	
    if (mAudioSink.get() != NULL)
	{
    	LOGD("use initied audio track");
        err = mAudioSink->open(
                mSampleRate, numChannels, AudioSystemWrapper::PCM_16_BIT,
                DEFAULT_AUDIOSINK_BUFFERCOUNT,
                &AudioPlayer::AudioSinkCallback, this);
        if (err != OK)
		{
			LOGE("********open audio sink failed");
			mSource->stop();
			mSource.clear();
            return err;
        }

        mLatencyUs = (int64_t)mAudioSink->latency() * 1000;
        mFrameSize = mAudioSink->frameSize();

        mAudioSink->start();
    }
	else
	{
    	LOGE("********create new audio track, but it does not implemented.");
		return ERROR_UNSUPPORTED;
		/*
		mAudioTrack = new AudioTrack(
				                AudioSystemWrapper::MUSIC, 
				                mSampleRate,
				                AudioSystemWrapper::PCM_16_BIT,
				                (numChannels == 2)? AudioSystemWrapper::CHANNEL_OUT_STEREO : AudioSystemWrapper::CHANNEL_OUT_MONO,
				                0,
				                0,
				                &AudioCallback,
				                this,
				                0);

		if ((err = mAudioTrack->initCheck()) != OK) {
			delete mAudioTrack;
			mAudioTrack = NULL;

			mSource->stop();

			return err;
		}
		//LOGE("********create audio track success");

		mLatencyUs = (int64_t)mAudioTrack->latency() * 1000;
		//LOGE("audio latency is %d", mLatencyUs);
		mFrameSize = mAudioTrack->frameSize();
		//LOGE("audio frame size is %d", mFrameSize);

		//LOGE("starting mAudioTrack");
		mAudioTrack->start();
		//LOGE("end mAudioTrack");
		*/
    }
	return OK;
}

void AudioPlayer::notifyListener_l(int msg, int ext1, int ext2) {
    LOGD("notifyListener_l");
    if (mListener != NULL) {
        //sp<MediaPlayerListener> listener = mListener.promote();

        //if (listener != NULL) {
			//LOGE("send event to listener");
            //listener->sendEvent(msg, ext1, ext2);
            mListener->notify(msg, ext1, ext2);
        //}
    }
	else
	{
	     LOGE("mListener is null");
	}
}

void AudioPlayer::setListener(IPlayer* listener)
{
    Mutex::Autolock autoLock(mLock);
	
    mListener = listener;
}
	
status_t AudioPlayer::startCompatibilityTest(bool sourceAlreadyStarted)
{
    Mutex::Autolock autoLock(mLock);
	
    mRunningCompatibilityTest = true;
    return start(sourceAlreadyStarted);
}

void AudioPlayer::stopCompatibilityTest()
{
    Mutex::Autolock autoLock(mLock);
	
	mRunningCompatibilityTest = false;
}


}
