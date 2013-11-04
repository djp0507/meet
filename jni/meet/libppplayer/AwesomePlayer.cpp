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
#define LOG_TAG "AwesomePlayer"

#include "platform-pp/PPPlatForm.h"

#include "include-pp/MediaSource.h"
#include "include-pp/AwesomePlayer.h"
#include "include-pp/Prefetcher.h"
#include "include-pp/AudioPlayer.h"
#include "include-pp/MediaBuffer.h"
#include "include-pp/ppDataSource.h"
#include "include-pp/MediaDefs.h"
#include "include-pp/MediaExtractor.h"
#include "include-pp/FileSource.h"
#include "include-pp/CachingDataSource.h"
#include "include-pp/MediaDebug.h"
#include "include-pp/ColorConverter.h"
#include "include-pp/Utils.h"

#include "include-pp/sf/MetaData.h"

#include "include-pp/utils/Log.h"
//#include "include-pp/log.h"
#include <dlfcn.h>
#include <stdio.h>

//#include "include/SoftwareRenderer.h"

//#include <binder/IPCThreadState.h>
//#include <media/stagefright/FileSource.h>
//#include <media/stagefright/MediaDebug.h>
//#include <media/stagefright/MediaBuffer.h>
//#include <media/stagefright/MetaData.h>

#include <media/stagefright/openmax/OMX_Core.h>
#include <media/stagefright/openmax/OMX_Video.h>
#include <media/stagefright/openmax/OMX_Index.h>

//#include <surfaceflinger/ISurface.h>
#include "include-pp/ui/Overlay.h"

namespace android {

static const int FRAME_NUM = 10;
static const int64_t FRAME_YVALUE[FRAME_NUM] = {0, 0, 0, 0, 0,
										  0, 0, 0, 0, 0};

struct AwesomeEvent : public TimedEventQueue::Event {
    AwesomeEvent(
            AwesomePlayer *player,
            void (AwesomePlayer::*method)())
        : mPlayer(player),
          mMethod(method) {
    }

protected:
    virtual ~AwesomeEvent() {}

    virtual void fire(TimedEventQueue *queue, int64_t /* now_us */) {
        (mPlayer->*mMethod)();
    }

private:
    AwesomePlayer *mPlayer;
    void (AwesomePlayer::*mMethod)();

    AwesomeEvent(const AwesomeEvent &);
    AwesomeEvent &operator=(const AwesomeEvent &);
};

struct AwesomeRemoteRenderer : public AwesomeRenderer {
    AwesomeRemoteRenderer(IPPOMXRenderer* target)
        : mTarget(target)
    {
        if(mTarget==NULL) LOGE("constructor: create AwesomeRemoteRenderer fail");
    }

    virtual void render(MediaBuffer *buffer) {
		/*
		void *id;
		if (buffer->meta_data()->findPointer(kKeyBufferID, &id)) {
		    //mTarget->render((IOMX::buffer_id)id);
		    mTarget->render((void*)id);
		}
		*/
        mTarget->render(buffer);
    }
	virtual bool ownBuffer()
	{
		return mTarget->ownBuffer();
	}
	
	virtual ~AwesomeRemoteRenderer()
	{
		if(mTarget!=NULL)
		{
		    LOGD("Destroying IPPOMXRenderer start");
		    delete mTarget;
			mTarget = NULL;
		    LOGD("Destroying IPPOMXRenderer end");
		}
	}

private:
    IPPOMXRenderer* mTarget;

    AwesomeRemoteRenderer(const AwesomeRemoteRenderer &);
    AwesomeRemoteRenderer &operator=(const AwesomeRemoteRenderer &);
};
/*
struct AwesomeLocalRenderer : public AwesomeRenderer {
    AwesomeLocalRenderer(
            bool previewOnly,
            const char *componentName,
            OMX_COLOR_FORMATTYPE colorFormat,
            const sp<ISurface> &surface,
            size_t displayWidth, size_t displayHeight,
            size_t decodedWidth, size_t decodedHeight)
        : mTarget(NULL),
          mLibHandle(NULL) {
            init(previewOnly, componentName,
                 colorFormat, surface, displayWidth,
                 displayHeight, decodedWidth, decodedHeight);
    }

    virtual void render(MediaBuffer *buffer) {
        render((const uint8_t *)buffer->data() + buffer->range_offset(),
               buffer->range_length());
    }

    void render(const void *data, size_t size) {
        mTarget->render(data, size, NULL);
    }

protected:
    virtual ~AwesomeLocalRenderer() {
        delete mTarget;
        mTarget = NULL;

        if (mLibHandle) {
            dlclose(mLibHandle);
            mLibHandle = NULL;
        }
    }

private:
    VideoRenderer *mTarget;
    void *mLibHandle;

    void init(
            bool previewOnly,
            const char *componentName,
            OMX_COLOR_FORMATTYPE colorFormat,
            const sp<ISurface> &surface,
            size_t displayWidth, size_t displayHeight,
            size_t decodedWidth, size_t decodedHeight);

    AwesomeLocalRenderer(const AwesomeLocalRenderer &);
    AwesomeLocalRenderer &operator=(const AwesomeLocalRenderer &);;
};

void AwesomeLocalRenderer::init(
        bool previewOnly,
        const char *componentName,
        OMX_COLOR_FORMATTYPE colorFormat,
        const sp<ISurface> &surface,
        size_t displayWidth, size_t displayHeight,
        size_t decodedWidth, size_t decodedHeight) {
    if (!previewOnly) {
        // We will stick to the vanilla software-color-converting renderer
        // for "previewOnly" mode, to avoid unneccessarily switching overlays
        // more often than necessary.

        mLibHandle = dlopen("libstagefrighthw.so", RTLD_NOW);

        if (mLibHandle) {
            typedef VideoRenderer *(*CreateRendererFunc)(
                    const sp<ISurface> &surface,
                    const char *componentName,
                    OMX_COLOR_FORMATTYPE colorFormat,
                    size_t displayWidth, size_t displayHeight,
                    size_t decodedWidth, size_t decodedHeight);

            CreateRendererFunc func =
                (CreateRendererFunc)dlsym(
                        mLibHandle,
                        "_Z14createRendererRKN7android2spINS_8ISurfaceEEEPKc20"
                        "OMX_COLOR_FORMATTYPEjjjj");

            if (func) {
                mTarget =
                    (*func)(surface, componentName, colorFormat,
                        displayWidth, displayHeight,
                        decodedWidth, decodedHeight);
            }
        }
    }

    if (mTarget == NULL) {
        mTarget = new SoftwareRenderer(
                colorFormat, surface, displayWidth, displayHeight,
                decodedWidth, decodedHeight);
    }

}
*/

static int getNowSec() {
    struct timeval tv;
    gettimeofday(&tv, NULL);

    return (int)tv.tv_sec;
}

//static int PLAYER_START_TIME=0;

AwesomePlayer::AwesomePlayer()
    : mQueueStarted(false),
      mTimeSource(NULL),
      mVideoRendererIsPreview(false),
      mVideoRenderer(NULL),
      mAudioPlayer(NULL),
      mFlags(0),
      mListener(NULL),
      mExtractorFlags(0),
      mLastVideoBuffer(NULL),
      mVideoBuffer(NULL),
      mVideoBufferMap(NULL),
//	  mConverter(NULL),
      mVideoRenderTimeout(40000),
      mReadingVideo(false),
      mFristReadingVideoDone(false),
      mSuspensionState(NULL),
      mRunningCompatibilityTest(false) ,
      //mIsBuffering(false),
      mSeekingTimeUs(0){
      
	LOGD("AwesomePlayer constructor");
		
    //CHECK_EQ(mClient.connect(), OK);
	//mOMX = OMXWrapper::Create();

	mDeviceLoaded = OMXWrapper::Create();

    DataSource::RegisterDefaultSniffers();

    mVideoEvent = new AwesomeEvent(this, &AwesomePlayer::onVideoEvent);
    mVideoEventPending = false;
    mStreamDoneEvent = new AwesomeEvent(this, &AwesomePlayer::onStreamDone);
    mStreamDoneEventPending = false;
    mBufferingEvent = new AwesomeEvent(this, &AwesomePlayer::onBufferingUpdate);
    mBufferingEventPending = false;
    mSeekingEvent = new AwesomeEvent(this, &AwesomePlayer::onSeekingEvent);
    mSeekingEventPending = false;
    mCheckAudioStatusEvent = new AwesomeEvent(this, &AwesomePlayer::onCheckAudioStatus);
    mAudioStatusEventPending = false;
    mCheckSeekingStatusEvent = new AwesomeEvent(this, &AwesomePlayer::onCheckSeekingStatus);
    mSeekingStatusEventPending = false;

	//mVideoBufferMap = new KeyedVector<int64_t, MediaBuffer*>;

    //reset();
}

AwesomePlayer::~AwesomePlayer() {
    
	LOGD("AwesomePlayer destructor");

    if(mPPExtractor!=NULL)
		mPPExtractor->stop();
	if(mPPDataSource!=NULL)
		mPPDataSource->closeStream();
	  
    LOGD("++++++++ deconstructor 1");
    
    if (mQueueStarted) {
	    LOGD("++++++++ deconstructor 2");
		//stop mQueue after stopping mPPExtractor
        mQueue.stop();
    }
    
    LOGD("++++++++ deconstructor 3");

    reset();
    
    LOGD("++++++++ deconstructor 4");

    //mClient.disconnect();
    //mOMX:TODO
    LOGD("++++++++ deconstructor 5");

    if (mVideoBufferMap) {
		for (size_t i = 0; i < mVideoBufferMap->size(); ++i) {
			((MediaBuffer*)(mVideoBufferMap->valueAt(i)))->release();
        }
		mVideoBufferMap->clear();
        delete mVideoBufferMap;
        mVideoBufferMap = NULL;
    }
	OMXWrapper::Destroy();
}

void AwesomePlayer::cancelPlayerEvents(bool keepBufferingGoing) {
    LOGD("cancelPlayerEvents");
    mQueue.cancelEvent(mVideoEvent->eventID());
    mVideoEventPending = false;
    mQueue.cancelEvent(mStreamDoneEvent->eventID());
    mStreamDoneEventPending = false;
    mQueue.cancelEvent(mCheckAudioStatusEvent->eventID());
    mAudioStatusEventPending = false;

    if (!keepBufferingGoing) {
        mQueue.cancelEvent(mBufferingEvent->eventID());
        mBufferingEventPending = false;
    }
}

//void AwesomePlayer::setListener(const wp<MediaPlayerBase> &listener)
void AwesomePlayer::setListener(IPlayer* listener)
{
    Mutex::Autolock autoLock(mLock);
    mListener = listener;
}

status_t AwesomePlayer::setDataSource(
        const char *uri, const KeyedVector<String8, String8> *headers) {
    Mutex::Autolock autoLock(mLock);
    return setDataSource_l(uri, headers);
}

status_t AwesomePlayer::setDataSource_l(
        const char *uri, const KeyedVector<String8, String8> *headers) {
    reset_l();

    mUri = uri;

    if (headers) {
        mUriHeaders = *headers;
    }

    // The actual work will be done during preparation in the call to
    // ::finishSetDataSource_l to avoid blocking the calling thread in
    // setDataSource for any significant time.

    return OK;
}

status_t AwesomePlayer::setDataSource(
        int fd, int64_t offset, int64_t length) {
    Mutex::Autolock autoLock(mLock);

    reset_l();

    sp<DataSource> dataSource = new FileSource(fd, offset, length);

    status_t err = dataSource->initCheck();

    if (err != OK) {
        return err;
    }

    mFileSource = dataSource;

    return setDataSource_l(dataSource);
}

status_t AwesomePlayer::setDataSource_l(
        const sp<DataSource> &dataSource) {
    sp<MediaExtractor> extractor = MediaExtractor::Create(dataSource);

    if (extractor == NULL) {
        return UNKNOWN_ERROR;
    }

    return setDataSource_l(extractor);
}

status_t AwesomePlayer::setDataSource_l(const sp<MediaExtractor> &extractor) {
    bool haveAudio = false;
    bool haveVideo = false;
    for (size_t i = 0; i < extractor->countTracks(); ++i) {
        sp<MetaData> meta = extractor->getTrackMetaData(i);

        const char *mime;
        CHECK(meta->findCString(kKeyMIMEType, &mime));

        if (!haveVideo && !strncasecmp(mime, "video/", 6)) {
            setVideoSource(extractor->getTrack(i));
            haveVideo = true;
        } else if (!haveAudio && !strncasecmp(mime, "audio/", 6)) {
            setAudioSource(extractor->getTrack(i));
            haveAudio = true;
        }

        if (haveAudio && haveVideo) {
            break;
        }
    }

    if (!haveAudio && !haveVideo) {
        return UNKNOWN_ERROR;
    }
    
    mExtractorFlags = extractor->flags();

    return OK;
}

void AwesomePlayer::reset() {

    LOGD("++++++++mLock wait ");
    
    Mutex::Autolock autoLock(mLock);
    
    LOGD("++++++++mLock got ");
    
    LOGD("++++++++Start reset");
    
    reset_l();
    
    LOGD("++++++++End reset");
}

void AwesomePlayer::reset_l() {
	
    if(mPPExtractor!=NULL)
		mPPExtractor->stop();
	if(mPPDataSource!=NULL)
		mPPDataSource->closeStream();
		
	LOGD("reset 1");
	
    if (mFlags & PREPARING) {
        mFlags |= PREPARE_CANCELLED;
	    LOGD("reset 1.1 mFlags:%d", mFlags);
        if (mConnectingDataSource != NULL) {
		/*
            	LOGI("interrupting the connection process");
            	mConnectingDataSource->disconnect();
           	*/
        }
        if (mPPDataSource.get() != NULL) {
            LOGD("interrupting the connection process");
            mPPDataSource->closeStream();
        }
    }

	LOGD("reset 2");
	
    while (mFlags & PREPARING) {
        mPreparedCondition.wait(mLock);

    }
	LOGD("reset 3");

    cancelPlayerEvents();
    
	LOGD("reset 4");
	/*
    	if (mPrefetcher != NULL) {
        	CHECK_EQ(mPrefetcher->getStrongCount(), 1);
    	}
    	mPrefetcher.clear();
	*/
	LOGD("reset 5");
	
    mAudioTrack.clear();
    mVideoTrack.clear();

    // Shutdown audio first, so that the respone to the reset request
    // appears to happen instantaneously as far as the user is concerned
    // If we did this later, audio would continue playing while we
    // shutdown the video-related resources and the player appear to
    // not be as responsive to a reset request.
    
	LOGD("reset 6");
	
    if (mAudioPlayer == NULL && mAudioSource != NULL) {
        // If we had an audio player, it would have effectively
        // taken possession of the audio source and stopped it when
        // _it_ is stopped. Otherwise this is still our responsibility.
        
		LOGD("reset 6.1");
        mAudioSource->stop();
    }
    
	LOGD("reset 7");
    mAudioSource.clear();

    if (mTimeSource != mAudioPlayer) {
		LOGD("reset 7.1");
        delete mTimeSource;
    }
	LOGD("reset 8");
    mTimeSource = NULL;

    while(!mFristReadingVideoDone && mReadingVideo)
    {
		LOGD("reset 8.1");
        usleep(100000);
    }
	if(mAudioPlayer!=NULL)
	{
		mAudioPlayer->pause();
	    delete mAudioPlayer;
	    mAudioPlayer = NULL;
		LOGD("reset 9");
	}

	LOGD("reset 9.1, mLastVideoBuffer: %p", mLastVideoBuffer);

    if (mLastVideoBuffer != NULL) {
		LOGD("reset 9.2");
		
        mLastVideoBuffer->release();
        mLastVideoBuffer = NULL;
        
		LOGD("reset 9.3");
    }

	LOGD("reset 10");
    if (mVideoBuffer != NULL) {
		LOGD("reset 10.1");
        mVideoBuffer->release();
        mVideoBuffer = NULL;
    }
	
	LOGD("reset 10.2");
    if (mVideoBufferMap) {
		for (size_t i = 0; i < mVideoBufferMap->size(); ++i) {
			((MediaBuffer*)(mVideoBufferMap->valueAt(i)))->release();
        }
		mVideoBufferMap->clear();
    }

	LOGD("reset 10.3");
    if(mVideoRenderer!=NULL)
	{
	    LOGD("Destory AwesomeRemoteRenderer");
        mVideoRenderer.clear();
		mVideoRenderer=NULL;
	    //IPCThreadState::self()->flushCommands();
		OMXWrapper::FlushCommands();
	}

	LOGD("reset 10.3.1");
    if (mVideoSource != NULL) {
        mVideoSource->stop();
		//todo: the following code checking probably could be removed.
		LOGD("check mReadingVideo %d", mReadingVideo);
		while(mReadingVideo)
		{
			LOGD("reset 10.4");
            usleep(100000);
		}

        // The following hack is necessary to ensure that the OMX
        // component is completely released by the time we may try
        // to instantiate it again.
        wp<MediaSource> tmp = mVideoSource;
        mVideoSource.clear();
        
        while (tmp.promote() != NULL) {
			LOGD("reset 10.5");
            usleep(1000);
        }
        //IPCThreadState::self()->flushCommands();
		OMXWrapper::FlushCommands();
        
    }
	LOGD("reset 11");

    mDurationUs = -1;
    mFlags = 0;
    LOGV("reset 12 mFlags:%d", mFlags);
    mExtractorFlags = 0;
    mVideoWidth = mVideoHeight = -1;
    mDecodedWidth = mDecodedHeight = -1;
    mTimeSourceDeltaUs = 0;
    mVideoTimeUs = 0;

	LOGD("reset_l: set mSeeking to false");
    mSeeking = false;
    mSeekNotificationSent = true;
    mSeekTimeUs = 0;

    mUri.setTo("");
    mUriHeaders.clear();

    mFileSource.clear();
	LOGD("reset 13");

    //delete mSuspensionState;
    //mSuspensionState = NULL;
	LOGD("reset 14");
}

void AwesomePlayer::notifyListener_l(int msg, int ext1, int ext2) {
    LOGD("notifyListener_l");
    if (mListener != NULL) {
        //sp<MediaPlayerListener> listener = mListener.promote();

        //if (listener != NULL) {
	     	//LOGE("send event to listener");
            //listener->sendEvent(msg, ext1, ext2);
            mListener->notify(msg, ext1, ext2);
        //}
    } else {
	     LOGD("mListener is null");
	}
}

void AwesomePlayer::checkBufferingStatus()
{
    static bool needBufferingEndNotification = false;
    static int bufferingBeginTime = 0;
    static int nowTime = 0;
    if(needBufferingEndNotification)
    {
        if(!mPPExtractor->isBuffering() && !mSeeking)
        {
            LOGD("MEDIA_INFO_BUFFERING_END");
            notifyListener_l(MEDIA_INFO, MEDIA_INFO_BUFFERING_END);
            needBufferingEndNotification = false;

            if(!mSeekNotificationSent)
            {
                LOGD("send event MEDIA_SEEK_COMPLETE");
                notifyListener_l(MEDIA_SEEK_COMPLETE);
                mSeekNotificationSent = true;

                if(mAudioPlayer!=NULL&&(mFlags & PLAYING))
            	{ 
    				LOGD("resume audio player");
    				mAudioPlayer->resume();
            	}
            }
        }
    }
    else
    {
        if(mPPExtractor->isBuffering() || mSeeking)
        {
            if(bufferingBeginTime == 0)
            {
                bufferingBeginTime = getNowSec() ;
            }
            nowTime = getNowSec() ;
            if(nowTime -bufferingBeginTime > 1 || mSeeking)
            {
                LOGD("MEDIA_INFO_BUFFERING_START");
                notifyListener_l(MEDIA_INFO, MEDIA_INFO_BUFFERING_START);
                needBufferingEndNotification = true;
                bufferingBeginTime = 0;
                LOGD("nowTime:%d bufferingBeginTime:%d", nowTime, bufferingBeginTime);
            }
            else
            {
                LOGD("nowTime:%d bufferingBeginTime:%d, Ingore event MEDIA_INFO_BUFFERING_START, as player starting", nowTime, bufferingBeginTime);
            }
        }
        else
        {
            bufferingBeginTime = 0;
        }
    }
}

void AwesomePlayer::onBufferingUpdate() {

	LOGD("onBufferingUpdate 1");
    Mutex::Autolock autoLock(mLock);
    if (!mBufferingEventPending) {
        return;
    }
    mBufferingEventPending = false;

    int64_t durationUs;
    {
        Mutex::Autolock autoLock(mMiscStateLock);
        durationUs = mDurationUs;
    }
    
	LOGD("onBufferingUpdate 1.1");

    if (durationUs > 0) {
        //int64_t cachedDurationUs = mPrefetcher->getCachedDurationUs();
        if(mPPExtractor == NULL) return;
        int64_t cachedDurationUs = mPPExtractor->getCachedDurationUs();
        int64_t positionUs;
        getPosition(&positionUs);

        cachedDurationUs += positionUs;

        //float percentage = (float)(((int)(cachedDurationUs/1000) )/ ((int)(durationUs/1000)));
        //LOGE("getCachedDurationUs percentage %d.", (int)(percentage*100.0));
		LOGD("onBufferingUpdate 5, cachedDurationUs: %lld, durationUs: %lld", cachedDurationUs, durationUs);
		
		mLock.unlock();
		
		LOGD("onBufferingUpdate 6");
		
		//int percent100 = (int)(percentage*100);
		//int cachedTime = cachedDurationUs/1000;
		//int durationTime = durationUs/1000;
		int percent100 = (int)(cachedDurationUs*100/durationUs)+2; // 2 is to compensate lost data.
		LOGD("onBufferingUpdate 7, percent: %d", percent100);
		
        notifyListener_l(MEDIA_BUFFERING_UPDATE, percent100);
        
		LOGD("onBufferingUpdate 8");
		
		mLock.lock();

        postBufferingEvent_l();
    } else {
        LOGE("Not sending buffering status because duration is unknown.");
    }

}

void AwesomePlayer::onSeekingEvent()
{
	LOGD("onSeekingEvent");
    Mutex::Autolock autoLock(mLock);
    if (!mSeekingEventPending) {
        return;
    }
    mSeekingEventPending = false;

	seekTo_l(mSeekingTimeUs);

}

void AwesomePlayer::onStreamDone() {
    // Posted whenever any stream finishes playing.
    LOGD("onStreamDone");

    Mutex::Autolock autoLock(mLock);
    if (!mStreamDoneEventPending) {
        return;
    }
    mStreamDoneEventPending = false;

    if (mStreamDoneStatus == ERROR_END_OF_STREAM && (mFlags & LOOPING)) {
        seekTo_l(0);

        if (mVideoSource != NULL) {
            postVideoEvent_l();
        }
    } else {
        if (mStreamDoneStatus == ERROR_END_OF_STREAM) {
            LOGD("MEDIA_PLAYBACK_COMPLETE");
			mLock.unlock();
            notifyListener_l(MEDIA_PLAYBACK_COMPLETE);
			mLock.lock();
        } else {
            LOGE("MEDIA_ERROR %d", mStreamDoneStatus);
		    mLock.unlock();
			if(!mRunningCompatibilityTest)
			{
	            notifyListener_l(
	                    MEDIA_ERROR, MEDIA_ERROR_UNKNOWN, mStreamDoneStatus);
			}
			else
			{
				//notify check result
		        notifyListener_l(MEDIA_COMPATIBILITY_TEST_COMPLETE, 0, 0); //0:video, 1:audio
			}
			mLock.lock();
        }

        pause_l();

        mFlags |= AT_EOS;
	    LOGV("onstreamdone mFlags:%d", mFlags);
    }
}

status_t AwesomePlayer::play() {
    LOGD("play");
    //Mutex::Autolock autoLock(mLock);
    return play_l();
	
	//postPlayEvent_l();
    //return OK;
}

status_t AwesomePlayer::play_l() {
    LOGD("play_l 1: %d", mFlags);
    if (mFlags & PLAYING) {
        return OK;
    }

    if (!(mFlags & PREPARED)) {
        status_t err = prepare_l();

        if (err != OK) {
            return err;
        }
    }

    mFlags |= PLAYING;
    mFlags |= FIRST_FRAME;
    LOGV("play 1.1 mFlags:%d", mFlags);

    bool deferredAudioSeek = false;

    LOGD("play_l 2");
    if (mAudioSource != NULL) {
        if (mAudioPlayer == NULL) {
            if (mAudioSink != NULL) {
                mAudioPlayer = new AudioPlayer(mAudioSink);
                mAudioPlayer->setSource(mAudioSource);

    			LOGD("play_l 3");
    			
                // We've already started the MediaSource in order to enable
                // the prefetcher to read its data.
                
                status_t err;
				if(mRunningCompatibilityTest)
				{
					mAudioPlayer->setListener(mListener);
					err = mAudioPlayer->startCompatibilityTest(true /* sourceAlreadyStarted */);
				} else {
					err = mAudioPlayer->start(true /* sourceAlreadyStarted */);
				}

    			LOGD("play_l 4");
                if (err != OK) {
                
                    delete mAudioPlayer;
                    mAudioPlayer = NULL;

                    mFlags &= ~(PLAYING | FIRST_FRAME);
				    LOGV("play_1 4.5 mFlags:%d", mFlags);

		      		LOGD("Start audioplayer failed");

                    return err;
                }

    			LOGD("play_l 5");
    			
                delete mTimeSource;
                mTimeSource = mAudioPlayer;

                deferredAudioSeek = true;

                //mWatchForAudioSeekComplete = false;
                mWatchForAudioEOS = true;
            }
        } else {
            mAudioPlayer->resume();
        }
        
    	LOGD("play_l 6");
	    if(!mRunningCompatibilityTest)
	    {
	        postCheckAudioStatusEvent_l();
	    }
    }

    if (mTimeSource == NULL && mAudioPlayer == NULL) {
        mTimeSource = new SystemTimeSource;
    }

    LOGD("play_l 7");
    if (mVideoSource != NULL) {
        // Kick off video playback
        postVideoEvent_l();
    }

    LOGD("play_l 8");
    if (deferredAudioSeek) {
        // If there was a seek request while we were paused
        // and we're just starting up again, honor the request now.
        //seekAudioIfNecessary_l();
    }

    LOGD("play_l 9");
    if (mFlags & AT_EOS) {
        // Legacy behaviour, if a stream finishes playing and then
        // is started again, we play from the start...
        seekTo_l(0);
    }
    LOGD("play_l");

    return OK;
}

void AwesomePlayer::initRenderer_l() {
	LOGD("start initRenderer_l");
	
    // Our OMX codecs allocate buffers on the media_server side
    // therefore they require a remote IOMXRenderer that knows how
    // to display them.
	LOGD("mSurface: %p", mSurface);
    if (mSurface != NULL) {
		
        sp<MetaData> meta = mVideoSource->getFormat();

        int32_t format = 0;
        const char *component = NULL;
        //int32_t decodedWidth = 0, decodedHeight = 0;
        
        CHECK(meta->findInt32(kKeyColorFormat, &format));
        CHECK(meta->findCString(kKeyDecoderComponent, &component));
        CHECK(meta->findInt32(kKeyWidth, &mDecodedWidth));
        CHECK(meta->findInt32(kKeyHeight, &mDecodedHeight));

        //mVideoRenderer.clear();
        if(mVideoRenderer != NULL)
    	{
		    LOGD("Destory AwesomeRemoteRenderer 1");
	    	mVideoRenderer.clear();
			mVideoRenderer = NULL;
	        // Must ensure that mVideoRenderer's destructor is actually executed
	        // before creating a new one.
	        // IPCThreadState::self()->flushCommands();
			OMXWrapper::FlushCommands();
    	}
			
        LOGI("component: %s", component);
        LOGI("colorformat: %d", format);
        LOGI("decodedWidth: %d", mDecodedWidth);
        LOGI("decodedHeight: %d", mDecodedHeight);
        LOGI("mVideoWidth: %d", mVideoWidth);
        LOGI("mVideoHeight: %d", mVideoHeight);

        if (!strncmp("OMX.", component, 4))
        {	
            LOGD("Create AwesomeRemoteRenderer");
			IPPOMXRenderer* ppRender = OMXRendererWrapper::Create(
			                //mOMX,//mClient.interface(),
	                        mSurface, component,
	                        (OMX_COLOR_FORMATTYPE)format,
	                        mDecodedWidth, mDecodedHeight,
	                        mVideoWidth, mVideoHeight
	                        );
            LOGD("End Create AwesomeRemoteRenderer");
            
			if(ppRender != NULL)
			{
			    mVideoRenderer = new AwesomeRemoteRenderer(ppRender);
				mVideoRenderTimeout = ppRender->getVideoRenderTimeout();
			} else {
	            LOGE("Create PPRender failed");
			}

        } else {
            LOGE("local render is not supported");
        }
    }
	else
	{
		LOGE("mSurface is NULL");
	}
}

status_t AwesomePlayer::pause() {
    //Mutex::Autolock autoLock(mLock);
	//LOGD("++++++++Start puasing");
    //status_t ret=pause_l();
	//LOGD("++++++++End puasing");
	//return ret;

	//pause command should been executed immediately, but not through event handling cycle
	return pause_l();
	//postPauseEvent_l();
    //return OK;
}

status_t AwesomePlayer::pause_l() {
    if (!(mFlags & PLAYING)) {
        return OK;
    }

    cancelPlayerEvents(true /* keepBufferingGoing */);

    if (mAudioPlayer != NULL) {
        mAudioPlayer->pause();
    }

    mFlags &= ~PLAYING;
    LOGV("pause_l mFlags:%d", mFlags);

    return OK;
}

bool AwesomePlayer::isPlaying() const {
    return mFlags & PLAYING;
}

void AwesomePlayer::setSurface(void* surface) {
    LOGD("setSurface");
    Mutex::Autolock autoLock(mLock);

    mSurface = surface;
}

//void AwesomePlayer::setAudioSink(const sp<MediaPlayerBase::AudioSink> &audioSink)
void AwesomePlayer::setAudioSink(const sp<AudioSink> &audioSink)
{
    LOGD("setAudioSink");
    Mutex::Autolock autoLock(mLock);

    mAudioSink = audioSink;
}

status_t AwesomePlayer::setLooping(bool shouldLoop) {
    LOGD("setLooping");
    Mutex::Autolock autoLock(mLock);

    mFlags = mFlags & ~LOOPING;
    LOGV("setlooping 1 mFlags:%d", mFlags);

    if (shouldLoop) {
        mFlags |= LOOPING;
	    LOGV("setlooping 2 mFlags:%d", mFlags);
    }

    return OK;
}

status_t AwesomePlayer::getDuration(int64_t *durationUs) {
    LOGD("getDuration");
    Mutex::Autolock autoLock(mMiscStateLock);

    if (mDurationUs < 0) {
        return UNKNOWN_ERROR;
    }

    *durationUs = mDurationUs;

    return OK;
}

status_t AwesomePlayer::getPosition(int64_t *positionUs) {
    //LOGE("getPosition");
    if(mSeekingEventPending){
        *positionUs = mSeekingTimeUs;
	} else if (mSeeking) {
		//LOGE("getPosition seek : 1");
        *positionUs = mSeekTimeUs;
    } else if (mVideoSource != NULL) {
		//LOGE("getPosition seek : 2");
        Mutex::Autolock autoLock(mMiscStateLock);
        *positionUs = mVideoTimeUs;
		//LOGE("getPosition seek : %lld (usec)", (*positionUs));
    } else if (mAudioPlayer != NULL) {
		//LOGE("getPosition seek : 4");
        *positionUs = mAudioPlayer->getMediaTimeUs();
    } else {
		//LOGE("getPosition seek : 5");
        *positionUs = 0;
    }
    //LOGE("getPosition end");

    return OK;
}

status_t AwesomePlayer::seekTo(int64_t timeUs) {
    if (mExtractorFlags
            & (MediaExtractor::CAN_SEEK_FORWARD
                | MediaExtractor::CAN_SEEK_BACKWARD)) {
        mSeekingTimeUs = timeUs;
        //return seekTo_l(timeUs);
        postSeekingEvent_l();
    }
    return OK;
}

status_t AwesomePlayer::seekTo_l(int64_t timeUs) {
	LOGD("Assign seek task to: %lld us", timeUs);
	LOGD("Set mSeeking to true");
    mSeeking = true;
    mSeekNotificationSent = false;
    mSeekTimeUs = timeUs;
    mFlags &= ~AT_EOS;
    LOGV("seekto_l 1 mFlags:%d", mFlags);

    //seekAudioIfNecessary_l();

	if (mAudioPlayer != NULL) {
		LOGD("pause audio player");
		mAudioPlayer->pause();
	}

    if (!(mFlags & PLAYING)) {
        LOGV("seeking while paused, sending SEEK_COMPLETE notification"
             " immediately.");
        //notifyListener_l(MEDIA_SEEK_COMPLETE);
        //mSeekNotificationSent = true;

    	//if(mPPExtractor!=NULL)
    		//mPPExtractor->seekTo(mSeekTimeUs);
        postVideoEvent_l(1);//to display seek point frame
        postCheckSeekingStatusEvent_l();
    }

    return OK;
}
/*
void AwesomePlayer::seekAudioIfNecessary_l() {
    LOGD("seekAudioIfNecessary_l");
    if (mSeeking && mVideoSource == NULL && mAudioPlayer != NULL) {
        LOGD("seeking audio to %lld us (%.2f secs).", mSeekTimeUs, mSeekTimeUs / 1E6);
        mAudioPlayer->seekTo(mSeekTimeUs);

        mWatchForAudioSeekComplete = true;
        mWatchForAudioEOS = true;
        mSeekNotificationSent = false;
    }
}
*/
status_t AwesomePlayer::getVideoDimensions(
        int32_t *width, int32_t *height) const {
    LOGD("getVideoDimensions");
    Mutex::Autolock autoLock(mLock);

    if (mVideoWidth < 0 || mVideoHeight < 0) {
        return UNKNOWN_ERROR;
    }

    *width = mVideoWidth;
    *height = mVideoHeight;

    return OK;
}

void AwesomePlayer::setAudioSource(sp<MediaSource> source) {
    CHECK(source != NULL);

    if (mPrefetcher != NULL) {
        //source = mPrefetcher->addSource(source);
    }

    mAudioTrack = source;
}

status_t AwesomePlayer::initAudioDecoder() {
    sp<MetaData> meta = mAudioTrack->getFormat();

    const char *mime;
    
    CHECK(meta->findCString(kKeyMIMEType, &mime));
    
    LOGD("audio source format is %s", mime);
    
    if (!strcasecmp(mime, MEDIA_MIMETYPE_AUDIO_RAW)) {
        mAudioSource = mAudioTrack;
    } else {
	    int32_t sampleRate=0;
    	meta->findInt32(kKeySampleRate, &sampleRate);
		LOGD("Got audio sample rate is: %d", sampleRate);
		
        mAudioSource = OMXCodecWrapper::Create(
                //mOMX,//mClient.interface(),
                meta,
                false, // createEncoder
                mAudioTrack);
    }

    if (mAudioSource != NULL) {
        int64_t durationUs;
        if (mAudioTrack->getFormat()->findInt64(kKeyDuration, &durationUs)) {
            Mutex::Autolock autoLock(mMiscStateLock);
            if (mDurationUs < 0 || durationUs > mDurationUs) {
                mDurationUs = durationUs;
				  //LOGE("media duration is updated to %lld", mDurationUs);
            }
        }
		//LOGE("media duration is %lld", mDurationUs);

        status_t err = mAudioSource->start();

        if (err != OK) {
            mAudioSource.clear();
            return err;
        }
    } else {
		LOGE("create audio decoder failed");
	}

    return mAudioSource != NULL ? OK : UNKNOWN_ERROR;
}

void AwesomePlayer::setVideoSource(sp<MediaSource> source) {
    CHECK(source != NULL);

    if (mPrefetcher != NULL) {
        //source = mPrefetcher->addSource(source);
    }

    mVideoTrack = source;
}

status_t AwesomePlayer::initVideoDecoder() {
    const sp<MetaData> meta = mVideoTrack->getFormat();

	if(meta.get() == NULL)
	{
		LOGE("Invalid mVideoTrack");
		return ERROR_MALFORMED;
	}
	
    CHECK(meta->findInt32(kKeyWidth, &mVideoWidth));
    CHECK(meta->findInt32(kKeyHeight, &mVideoHeight));
    
    LOGI("compress video width: %d", mVideoWidth);
    LOGI("compress video height: %d", mVideoHeight);

	mVideoSource = OMXCodecWrapper::Create(
		//mOMX,//mClient.interface(),
		meta,
		false, // createEncoder
		mVideoTrack);
	
    //uint32_t quirks = ((OMXCodec*)mVideoSource.get())->getComponentQuirks("OMX.ST.VFM.H264Dec", false);
	//LOGE("quirks:%d", quirks);

    if (mVideoSource != NULL) {
        int64_t durationUs;
        if (mVideoTrack->getFormat()->findInt64(kKeyDuration, &durationUs)) {
            Mutex::Autolock autoLock(mMiscStateLock);
            if (mDurationUs < 0 || durationUs > mDurationUs) {
                mDurationUs = durationUs;
			  	// LOGE("media duration is updated to %lld", mDurationUs);
            }
        }
        // LOGE("media duration is %lld", mDurationUs);
        // roger
		// initRenderer_l();
		
        LOGD("begin mVideoSource->start(): %p", mVideoSource.get());
        status_t err = mVideoSource->start();
        LOGD("end mVideoSource->start()");

        if (err != OK) {
    		LOGE("start video playback failed");
            mVideoSource.clear();
            return err;
        } else {
    		//LOGE("start video playback success");
    	}
    }else {
		LOGE("create video decoder failed");
	}

    return mVideoSource != NULL ? OK : UNKNOWN_ERROR;
}

MediaBuffer* AwesomePlayer::getOrderedVideoFrame()
{
    if(mVideoBufferMap==NULL) return NULL;

	MediaSource::ReadOptions options;
    if (mSeeking) {
        //LOGE("seeking to %lld us (%.2f secs)", mSeekTimeUs, mSeekTimeUs / 1E6);
        options.setSeekTo(mSeekTimeUs);
    }
    
    while (mVideoBufferMap->size()<4) {
    
		if(mVideoSource==NULL) return NULL;
		
		mReadingVideo=true; 
		
	    LOGV("Start reading video frame, buffermapsize:%d", mVideoBufferMap->size());
	    MediaBuffer* pVideoBuffer = NULL;
        status_t err = mVideoSource->read(&pVideoBuffer, &options);
	    LOGV("End reading video frame");
	    
		mReadingVideo=false;
		if(!mFristReadingVideoDone)
			mFristReadingVideoDone=true;
	    //LOGE("mReadingVideo %d", mReadingVideo);
        options.clearSeekTo();

		Mutex::Autolock autoLock(mLock);
        if (err != OK || pVideoBuffer==NULL) {
            CHECK_EQ(pVideoBuffer, NULL);

            if (err == INFO_FORMAT_CHANGED) {
                LOGI("VideoSource signalled format change.");

                if (mVideoRenderer != NULL) {
                    mVideoRendererIsPreview = false;						
                    initRenderer_l();
                }
                continue;
            }
		    LOGE("read video frame failed");
            postStreamDoneEvent_l(err);
            return NULL;
        }

		//LOGE("check mVideoBuffer %p", mVideoBuffer);
        if (pVideoBuffer->range_length() == 0) {
            // Some decoders, notably the PV AVC software decoder
            // return spurious empty buffers that we just want to ignore.
            pVideoBuffer->release();
            pVideoBuffer = NULL;
            continue;
        }

        // LOGD("reading video frame success");

	    int64_t timeUs;
	    CHECK(pVideoBuffer->meta_data()->findInt64(kKeyTime, &timeUs));
		mVideoBufferMap->add(timeUs, pVideoBuffer);
        //break;
    }
	
	MediaBuffer* pOldestBuffer=NULL;
	pOldestBuffer = mVideoBufferMap->valueAt(0);
	mVideoBufferMap->removeItemsAt(0);
	return pOldestBuffer;

}

void AwesomePlayer::onVideoEvent() {
    LOGD("on video event start : get frame begin");
	bool isFirstFrame = false;
	
    Mutex::Autolock autoLock(mLock); 
    if (!mVideoEventPending) {
		// LOGE("on video event start 1");
        // The event has been cancelled in reset_l() but had already
        // been scheduled for execution at that time.
        return;
    }
	// LOGE("on video event start 2");
    mVideoEventPending = false;

    if (mSeeking) {
		// LOGE("on video event start 3");
        if (mLastVideoBuffer) {
            mLastVideoBuffer->release();
            mLastVideoBuffer = NULL;
        }

        if (mVideoBuffer) {
            mVideoBuffer->release();
            mVideoBuffer = NULL;
        }

        //if (mVideoBufferMap) {
		//	for (size_t i = 0; i < mVideoBufferMap->size(); ++i) {
		//		((MediaBuffer*)(mVideoBufferMap->valueAt(i)))->release();
        //    }
	    //    mVideoBufferMap->clear();
        //}
		
    }

    if(mPPExtractor != NULL)
    {
        checkBufferingStatus();

        if(mPPExtractor->isBuffering() && !mSeeking)
        {
            //do not read buffer in case this thread is blocked, so just return.
    	    postVideoEvent_l(100000ll);
            return;
        }
    }

    if (!mVideoBuffer) {

		if(0)
		{
			// for codecs that do not output ordered frames.
			LOGV("getOrderedVideoFrame start");
			mVideoBuffer = getOrderedVideoFrame();
			LOGV("getOrderedVideoFrame end");
		}
		else
		{
			//for codecs that output ordered frames.
	        MediaSource::ReadOptions options;
			// LOGE("Checking mSeeking:%d",mSeeking);
	        if (mSeeking) {
	            LOGV("seeking to %lld us (%.2f secs)", mSeekTimeUs, mSeekTimeUs / 1E6);
	            options.setSeekTo(mSeekTimeUs);
	        }
	        for (;;) {
				if(mVideoSource==NULL) return;
				mReadingVideo=true;
			    // LOGE("mReadingVideo %d", mReadingVideo);
			    
			    LOGV("Start reading video frame");
				mLock.unlock();
	            status_t err = mVideoSource->read(&mVideoBuffer, &options); //it is a blocking call
	            mLock.lock();
			    LOGV("return from reading video frame");
			    
				mReadingVideo=false;
				if(!mFristReadingVideoDone)
					mFristReadingVideoDone=true;
			    // LOGE("mReadingVideo %d", mReadingVideo);
	            options.clearSeekTo();

				//Mutex::Autolock autoLock(mLock);
	            if (err != OK || mVideoBuffer==NULL) {
	            
	                if (err == INFO_FORMAT_CHANGED) {
	                    LOGV("VideoSource signalled format change.");

	                    if (mVideoRenderer != NULL) {
	                        mVideoRendererIsPreview = false;						
	                        initRenderer_l();
	                    }
	                    continue;
	                }
					if(err != ERROR_END_OF_STREAM)
					{
					    LOGE("read video frame failed:%d", err);
					}
	                //if(mVideoBuffer == NULL) return;
					
				    LOGV("postStreamDoneEvent_l");
	                postStreamDoneEvent_l(err);
	                return;
	            }

				// LOGE("check mVideoBuffer %p", mVideoBuffer);
	            if (mVideoBuffer->range_length() == 0) {
	                // Some decoders, notably the PV AVC software decoder
	                // return spurious empty buffers that we just want to ignore.
	                mVideoBuffer->release();
	                mVideoBuffer = NULL;
	                continue;
	            }
			    // LOGE("reading video frame success");

	            break;
	        }
		}
    }

    //LOGE("prepare lock mLock");
	//Mutex::Autolock autoLock(mLock);
    //LOGE("on video event start 7");
	
	if (mVideoBuffer==NULL || mTimeSource==NULL) 
		return;
	
	if (mRunningCompatibilityTest)
	{
		bool ret = true;
        sp<MetaData> meta = mVideoSource->getFormat();
        int32_t format;
        meta->findInt32(kKeyColorFormat, &format);
        meta->findInt32(kKeyWidth, &mDecodedWidth);
        meta->findInt32(kKeyHeight, &mDecodedHeight);
        //meta->findCString(kKeyDecoderComponent, &mVideoDecoderComponent);

	    //check video decoding
		if (/*format != 0 && */
			mDecodedWidth <= 0 || 
			mDecodedHeight <= 0 || 
			mVideoBuffer->range_length() <= 0) /*100 is a default value.*/ 
		{
			LOGE("Invalid video codec parameters as below:");
			LOGE("decodedWidth: %d", mDecodedWidth);
			LOGE("decodedHeight: %d", mDecodedHeight);
			LOGE("mVideoBuffer->range_length(): %d", mVideoBuffer->range_length());
			ret = false;
		}
		
	    //check video rendering
	    if(ret)
	    {
			initRenderer_l();
			if (mVideoRenderer == NULL)
			{
				LOGE("Check compatibility video render failed");
				ret = false;
			}
	    }
		//notify check result
        notifyListener_l(MEDIA_COMPATIBILITY_TEST_COMPLETE, 0, ret); //0:video, 1:audio
		
		//exit awesomeplayer.
		mQueue.forceStop();
        mVideoBuffer->release();
        mVideoBuffer = NULL;
		LOGD("End running test");
        return;
		
	    /*
		static int frameNum = 0;
		if(frameNum == 0)
		{
		}
		//Check image quality
		if(ret)
		{
			LOGI("Valid video codec parameters as below:");
			LOGI("decodedWidth: %d", mDecodedWidth);
			LOGI("decodedHeight: %d", mDecodedHeight);
			LOGI("mVideoBuffer->data(): %p", mVideoBuffer->data());
			LOGI("mVideoBuffer->range_length(): %d", mVideoBuffer->range_length());
			LOGI("mVideoBuffer->range_offset(): %d", mVideoBuffer->range_offset());
			//compute Y pixel values
			int64_t yValues = 0;
			uint8_t* data = NULL;
			data = (uint8_t*)mVideoBuffer->data();
			data = data+mVideoBuffer->range_offset();
			size_t yLength = mDecodedWidth*mDecodedHeight;
			size_t bufferRangeLength = mVideoBuffer->range_length();
			if(bufferRangeLength < yLength)
			{
			    //skip those components which 
			    //output buffers contain no video data, just some
		        //opaque information that allows the overlay to display their contents. like "OMX.SEC"
				yLength = bufferRangeLength;
			}

			//for test, should be removed for release version.
			{
				char path[100];
				sprintf(path, "/data/data/com.android.player/frame_%d", frameNum);
				saveFrame(data, yLength, path);
			}
			
			for(int i=0;i<yLength;i++)
			{
				yValues = yValues + *(data+i);
				//LOGI("pixel %d: %d, yValues:%lld", i, *(data+i), yValues);
			}
			LOGI("Frame %d yValues: %lld", frameNum, yValues);
			
			//compare yValues with pre-computed value
			if(yValues == 0)
			{
				//some decoder output black frame at the beginning, just skip it.
		        mVideoBuffer->release();
		        mVideoBuffer = NULL;
				postVideoEvent_l();
				return;
			}
			else if(yValues > 0 && yValues < 1000)
			{
			    //skip those components which 
			    //output buffers contain no video data, just some
		        //opaque information that allows the overlay to display their contents. like "OMX.SEC"
		        frameNum = 10;
			}
			else if(0)//abs(yValues-FRAME_YVALUE[frameNum]) > 1000)
			{
				LOGE("Frame %d yValues does not match %lld == %lld", frameNum, yValues, FRAME_YVALUE[frameNum]);
				ret = false;
			}
			else
			{
				frameNum++;
			}
		}
		
		if(ret && frameNum<FRAME_NUM)
		{
	        mVideoBuffer->release();
	        mVideoBuffer = NULL;
			postVideoEvent_l();
			return;
		}
		*/
		
		/*
		//static const uint OMX_QCOM_COLOR_FormatYVU420SemiPlanar = 0x7FA30C00;
		//static const uint QOMX_COLOR_FormatYUV420PackedSemiPlanar64x32Tile2m8ka = 0x7FA30C03;
		if(format == OMX_COLOR_FormatYUV411Planar ||
			format == OMX_COLOR_FormatYUV411PackedPlanar ||
			format == OMX_COLOR_FormatYUV420Planar ||
			format == OMX_COLOR_FormatYUV420PackedPlanar ||
			format == OMX_COLOR_FormatYUV420SemiPlanar ||
			format == OMX_COLOR_FormatYUV422Planar ||
			format == OMX_COLOR_FormatYUV422PackedPlanar ||
			format == OMX_COLOR_FormatYUV422SemiPlanar ||
			format == OMX_COLOR_FormatYUV420PackedSemiPlanar ||
			format == OMX_COLOR_FormatYUV422PackedSemiPlanar ||
			format == OMX_QCOM_COLOR_FormatYVU420SemiPlanar ||
			format == QOMX_COLOR_FormatYUV420PackedSemiPlanar64x32Tile2m8ka)
		{
		    //YUV Planar format.
			char path[100];
			sprintf(path, "/data/data/com.pplive.androidphone/lib/libframe.so");
	        	int32_t len = decodedWidth*decodedHeight;
			uint8_t* src = (uint8_t*)malloc(len);
			if (src == NULL)
			{
				LOGE("memory runs out");
				
				//notify check result
			        notifyListener_l(MEDIA_COMPATIBILITY_TEST_COMPLETE, 0, ret); //0:video, 1:audio
				
				//exit awesomeplayer.
				mQueue.forceStop();
				
			        mVideoBuffer->release();
			        mVideoBuffer = NULL;
		       		 return;
			}
			
			loadFrame(src, len, path);
		
			// check decoded video data for Y value.
			// sprintf(path, "/data/data/com.pplive.androidphone/libframe.so");
			// saveFrame(mVideoBuffer->data()+mVideoBuffer->range_offset(), len, path);
		    	double psnr = computePSNR(src, (uint8_t*)(mVideoBuffer->data()+mVideoBuffer->range_offset()), len);

			LOGE("Video PSNR:%llf", psnr);
			if(psnr > 27) //27 is lowest quality value can be accepted. 
			{
				ret = true;
			}
			
			if(src) free(src);
		}
		else
		{
		    //none YUV Planar format. default set to true.
			ret = true;
		}
		*/
	}
	
    int64_t timeUs;
    CHECK(mVideoBuffer->meta_data()->findInt64(kKeyTime, &timeUs));

    {
        Mutex::Autolock autoLock(mMiscStateLock);
        mVideoTimeUs = timeUs;
		// LOGE("set mVideoTimeUs %lld (%.2f usec)", mVideoTimeUs, mVideoTimeUs/1E6);
    }

    if (mSeeking) {
        if (mAudioPlayer != NULL) {
		    //display the first frame asap.
		    if (mVideoRenderer != NULL)
			{
				LOGD("Now rendering the first video");
		        mVideoRenderer->render(mVideoBuffer);
		    }
			
            //LOGE("seeking audio to %lld us (%.2f secs).", timeUs, timeUs / 1E6);
            mAudioPlayer->seekTo(timeUs);
			//while(mAudioPlayer->isSeeking())
			//{
			//	usleep(10000);//0.01sec
			//}
			//LOGE("End waiting audio seeking complete");
            //mWatchForAudioSeekComplete = true;
            mWatchForAudioEOS = true;
        } /*else if (!mSeekNotificationSent) {
            // If we're playing video only, report seek complete now,
            // otherwise audio player will notify us later.
            mLock.unlock();
            notifyListener_l(MEDIA_SEEK_COMPLETE);
			mLock.lock();
        }*/

        mFlags |= FIRST_FRAME;
	    LOGD("onVideoEvent xxx mFlags:%d", mFlags);
        mSeeking = false;
		LOGD("getPosition: set mSeeking false");

    	if(!(mFlags & PLAYING))
    	{
    		//support seeking while paused to just refresh first frame,
    		// and then keep pausing
    		return;
    	}
    }

    //LOGE("on video event start 11");
    if (mFlags & FIRST_FRAME) {
        mFlags &= ~FIRST_FRAME;
	    LOGV("onvideoevent 11.1 mFlags:%d", mFlags);
		isFirstFrame = true;

        mTimeSourceDeltaUs = mTimeSource->getRealTimeUs() - timeUs;
		//LOGE("get mTimeSourceDeltaUs 1");
    }

	if(mAudioPlayer && mAudioPlayer->isSeeking())
	{
	    LOGV("audio player is under seeking, skip render frame");
        postVideoEvent_l();
		return;
	}
	
	if(!(mFlags & PLAYING))
	{
		//as read frame is blocking call, it could happen that player is paused/stopped during blocking.
	    LOGV("it is paused/stopped, skip render frame");
		return;
	}

    int64_t realTimeUs, mediaTimeUs;
    if (mAudioPlayer != NULL
        && mAudioPlayer->getMediaTimeMapping(&realTimeUs, &mediaTimeUs)) {
        mTimeSourceDeltaUs = realTimeUs - mediaTimeUs;
		
		//LOGE("get mTimeSourceDeltaUs 2");
	    //LOGE("mediaTimeUs is %lld (%.2f secs) ,realTimeUs is %lld (%.2f secs)", mediaTimeUs, mediaTimeUs / 1E6, realTimeUs, realTimeUs / 1E6);
    }

    int64_t nowUs = mTimeSource->getRealTimeUs() - mTimeSourceDeltaUs;

    int64_t latenessUs = nowUs - timeUs;
	
    LOGD("realTimeUs is %lld (%.2f secs), mediaTimeUs(audio) is %lld (%.2f secs), mediaTimeUs(video) is %lld (%.2f secs), latenessUs is %lld (%.2f secs)",
		realTimeUs, realTimeUs / 1E6,
		mediaTimeUs, mediaTimeUs / 1E6,
		timeUs, timeUs / 1E6,
		latenessUs, latenessUs / 1E6);

    //LOGE("start check initRenderer_l, mVideoRenderer:%d", mVideoRenderer!=NULL);
    if (mVideoRendererIsPreview || mVideoRenderer == NULL) {
        mVideoRendererIsPreview = false;

        initRenderer_l();
    }
    
    //if (latenessUs > 40000 && (!isFirstFrame))
    if (latenessUs > mVideoRenderTimeout && (!isFirstFrame))
    {
        // We're more than 40ms(default) late.
        LOGV("we're late by %lld us (%.2f secs), the threadhold is :%lld", latenessUs, latenessUs / 1E6, mVideoRenderTimeout);

        mVideoBuffer->release();
        mVideoBuffer = NULL;

	    LOGV("postVideoEvent_l begin");
        postVideoEvent_l();
	    LOGV("postVideoEvent_l end");
        return;
    }

    if (latenessUs < -10000 && (!isFirstFrame))
	{
        // We're more than 10ms early.
        LOGV("we're early by %lld us (%.2f secs)", latenessUs, latenessUs / 1E6);

	    //LOGE("postVideoEvent_l begin");
        postVideoEvent_l();
	    //LOGE("postVideoEvent_l end");
        return;
    }

    //LOGE("end check initRenderer_l");
    if (mVideoRenderer != NULL) {

	    LOGV("render frame begin");
	 
		/*
		static int count=0;
		memset(mVideoBuffer->data()+mVideoBuffer->range_offset(), 0, mVideoBuffer->range_length());
		memset(mVideoBuffer->data()+mVideoBuffer->range_offset()+((count++)%25)*(mVideoBuffer->range_length()/25), 255, mVideoBuffer->range_length()/25);
		*/
	    
        mVideoRenderer->render(mVideoBuffer);
	    LOGV("on video event end: render frame end");
    }

	if (!mVideoRenderer->ownBuffer())
	{
	    if (mLastVideoBuffer) {
	        mLastVideoBuffer->release();
	        mLastVideoBuffer = NULL;
	    }
	    mLastVideoBuffer = mVideoBuffer;
	}

    // if (mLastVideoBuffer) {
    //	mLastVideoBuffer->release();
    //	 mLastVideoBuffer = NULL;
    // }
    // mLastVideoBuffer = mVideoBuffer;
    mVideoBuffer = NULL;

	postVideoEvent_l();
}

void AwesomePlayer::postVideoEvent_l(int64_t delayUs) {
    if (mVideoEventPending) {
        return;
    }

    mVideoEventPending = true;
    mQueue.postEventWithDelay(mVideoEvent, delayUs < 0 ? 10000 : delayUs);
}

void AwesomePlayer::postStreamDoneEvent_l(status_t status) {
    if (mStreamDoneEventPending) {
        return;
    }
    mStreamDoneEventPending = true;

    mStreamDoneStatus = status;
    mQueue.postEvent(mStreamDoneEvent);
}

void AwesomePlayer::postBufferingEvent_l() {
    //if (mPrefetcher == NULL) {
    if (mPPExtractor == NULL) {
	//LOGE("Not buffering event");
        return;
    }

    if (mBufferingEventPending) {
	//LOGE("buffering event is pending");
        return;
    }
    mBufferingEventPending = true;
	
    //LOGE("posted event to mQueue");
    mQueue.postEventWithDelay(mBufferingEvent, 1000000ll);
}

void AwesomePlayer::postSeekingEvent_l() {

    if (mSeekingEventPending) {
        return;
    }
    mSeekingEventPending = true;
	
    //LOGE("posted event to mQueue");
    mQueue.postEvent(mSeekingEvent);
}


void AwesomePlayer::postCheckAudioStatusEvent_l() {
    if (mAudioStatusEventPending) {
        return;
    }
    mAudioStatusEventPending = true;
    mQueue.postEventWithDelay(mCheckAudioStatusEvent, 1000000ll);
}

void AwesomePlayer::postCheckSeekingStatusEvent_l() {
    if (mSeekingStatusEventPending) {
        return;
    }
    mSeekingStatusEventPending = true;
    mQueue.postEventWithDelay(mCheckSeekingStatusEvent, 50000ll);
}

void AwesomePlayer::onCheckAudioStatus() {
    LOGD("onCheckAudioStatus");
    Mutex::Autolock autoLock(mLock);
    if (!mAudioStatusEventPending) {
        // Event was dispatched and while we were blocking on the mutex,
        // has already been cancelled.
        return;
    }

    mAudioStatusEventPending = false;

    //if (mWatchForAudioSeekComplete/* && !mAudioPlayer->isSeeking()*/) {
       // mWatchForAudioSeekComplete = false;

        //if (!mSeekNotificationSent) {
			//LOGD("Start notify MEDIA_SEEK_COMPLETE");
			//mLock.unlock();
            //notifyListener_l(MEDIA_SEEK_COMPLETE);
			//mLock.lock();
            //mSeekNotificationSent = true;
        //}
        //mSeeking = false;
        //if(mSeeking == false)
    	//{
		//	if (mAudioPlayer != NULL) {
		//		LOGD("resume audio player");
		//		mAudioPlayer->resume();
		//	}
    	//}
    //}

    status_t finalStatus;
    if (mWatchForAudioEOS && mAudioPlayer->reachedEOS(&finalStatus)) {
        mWatchForAudioEOS = false;
        
	 	LOGD("mAudioPlayer->reachedEOS");
        postStreamDoneEvent_l(finalStatus);
    }

    postCheckAudioStatusEvent_l();
}

void AwesomePlayer::onCheckSeekingStatus() {
    LOGD("onCheckSeekingStatus");
    Mutex::Autolock autoLock(mLock);
    if (!mSeekingStatusEventPending) {
        // Event was dispatched and while we were blocking on the mutex,
        // has already been cancelled.
        return;
    }

    mSeekingStatusEventPending = false;
    
    checkBufferingStatus();
    
    if(!mSeekNotificationSent)
    {
        postCheckSeekingStatusEvent_l();
    }
}

status_t AwesomePlayer::prepare() {
    LOGD("prepare");
    Mutex::Autolock autoLock(mLock);
    return prepare_l();
}

status_t AwesomePlayer::prepare_l() {
    LOGD("prepare_l");
	if(!mDeviceLoaded)
		return UNKNOWN_ERROR;
	
    if (mFlags & PREPARED) {
        return OK;
    }

    if (mFlags & PREPARING) {
        return UNKNOWN_ERROR;
    }

    //LOGE("prepare_l asyn...");
    mIsAsyncPrepare = false;
    status_t err = prepareAsync_l();

    if (err != OK) {
        return err;
    }

    while (mFlags & PREPARING) {
        mPreparedCondition.wait(mLock);
    }

    return mPrepareResult;
}

status_t AwesomePlayer::prepareAsync() {
    LOGD("prepareAsync");
    Mutex::Autolock autoLock(mLock);
    if (mFlags & PREPARING) {
        LOGD("Preparing asyn error: async prepare already pending");
        return UNKNOWN_ERROR;  // async prepare already pending
    }

    mIsAsyncPrepare = true;
    return prepareAsync_l();
}

status_t AwesomePlayer::prepareAsync_l() {
    LOGD("preparing asyn...");
    if (mFlags & PREPARING) {
        return UNKNOWN_ERROR;  // async prepare already pending
    }

    if (!mQueueStarted) {
	LOGD("mQueue starting.");
        mQueue.start();
        mQueueStarted = true;
	//LOGE("mQueue started.");
    }
	else
	{
		//LOGE("mQueue has started.");
	}

    mFlags |= PREPARING;
    LOGV("prepareAsync_l mFlags:%d", mFlags);
    mAsyncPrepareEvent = new AwesomeEvent(
            this, &AwesomePlayer::onPrepareAsyncEvent);
    //LOGE("Posting preparing event...");
    mQueue.postEvent(mAsyncPrepareEvent);

    return OK;
}

status_t AwesomePlayer::finishSetDataSource_l() {
	sp<DataSource> dataSource;

	if (!strncasecmp("http://", mUri.string(), 7)) {
	/*
	mConnectingDataSource = new HTTPDataSource(mUri, &mUriHeaders);

	mLock.unlock();
	status_t err = mConnectingDataSource->connect();
	mLock.lock();

	if (err != OK) {
		mConnectingDataSource.clear();

		LOGI("mConnectingDataSource->connect() returned %d", err);
		return err;
        }

	dataSource = new CachingDataSource(mConnectingDataSource, 64 * 1024, 10);

	mConnectingDataSource.clear();
	*/
	} 
	else if (!strncasecmp("ppvod", mUri.string(), 5) ||
			 !strncasecmp("pplive", mUri.string(), 6) ||
			 !strncasecmp("ppfile", mUri.string(), 6)) {
			
//		LOGE("start get ppDatasource");
		mPPDataSource = new PPDataSource();
		status_t err = mPPDataSource->initCheck();
		if(err!=OK) return err;
	 
		mLock.unlock();

		LOGD("start open stream");
		err = mPPDataSource->openStream(mUri.string());
		mLock.lock();
		LOGD("end open stream");

		if (err != OK) {
			mPPDataSource->closeStream();

			LOGE("PPDataSource does not started with error code: %d", err);
			return err;
		}
		dataSource = mPPDataSource;
	}
	else {
		dataSource = DataSource::CreateFromURI(mUri.string(), &mUriHeaders);
	}

	if (dataSource == NULL) {
		return UNKNOWN_ERROR;
	}

	sp<MediaExtractor> extractor = MediaExtractor::Create(dataSource);
	if(extractor.get()!=NULL && mPPDataSource.get()!=NULL)
	{
		mPPExtractor = (PPExtractor*)extractor.get();
	}

	if (extractor == NULL) {
		return UNKNOWN_ERROR;
	}

	if (dataSource->flags() & DataSource::kWantsPrefetching) {
//		mPrefetcher = new Prefetcher;
	}

	return setDataSource_l(extractor);
}

void AwesomePlayer::abortPrepare(status_t err) {
	CHECK(err != OK);

	if (mIsAsyncPrepare) {
		notifyListener_l(MEDIA_ERROR, MEDIA_ERROR_UNKNOWN, err);
	}

	mPrepareResult = err;
	mFlags &= ~(PREPARING|PREPARE_CANCELLED);
	LOGV("abortPrepare mFlags:%d", mFlags);
	mAsyncPrepareEvent = NULL;
	mPreparedCondition.broadcast();
}

// static
bool AwesomePlayer::ContinuePreparation(void *cookie) {
	AwesomePlayer *me = static_cast<AwesomePlayer *>(cookie);

	return (me->mFlags & PREPARE_CANCELLED) == 0;
}

void AwesomePlayer::onPrepareAsyncEvent() {
	LOGD("RUN AwesomePlayer::onPrepareAsyncEvent()");
//	sp<Prefetcher> prefetcher;

	{
		Mutex::Autolock autoLock(mLock);

		if (mFlags & PREPARE_CANCELLED) {
			LOGE("prepare was cancelled before doing anything");
			abortPrepare(UNKNOWN_ERROR);
			return;
		}

		if (mUri.size() > 0) {
			status_t err = finishSetDataSource_l();

			if (err != OK) {
				abortPrepare(err);
				return;
			}
		}

		if (mVideoTrack != NULL && mVideoSource == NULL) {
			LOGD("Initing video decoder");
			status_t err = initVideoDecoder();

			if (err != OK) {
				abortPrepare(err);
				return;
			}
			LOGD("Initing video decoder success");
		}

		if (mAudioTrack != NULL && mAudioSource == NULL) {
			LOGD("Initing audio decoder");
			status_t err = initAudioDecoder();

			if (err != OK) {
				abortPrepare(err);
				LOGE("Initing audio decoder failed");
				return;
			}
			LOGD("Initing audio decoder success");
		}

//		prefetcher = mPrefetcher;
	}

	/*
    	if (prefetcher != NULL) {
        {
	     //LOGE("Start enable prefetcher");
            Mutex::Autolock autoLock(mLock);
            if (mFlags & PREPARE_CANCELLED) {
                LOGI("prepare was cancelled before preparing the prefetcher");

                //prefetcher.clear();
                abortPrepare(UNKNOWN_ERROR);
                return;
            }
        }

        //LOGE("calling prefetcher->prepare()");
        status_t result = prefetcher->prepare(&AwesomePlayer::ContinuePreparation, this);

        prefetcher.clear();

        if (result == OK) {
            //LOGE("prefetcher is done preparing");
        } else {
            Mutex::Autolock autoLock(mLock);

            CHECK_EQ(result, -EINTR);

            LOGE("prefetcher->prepare() was cancelled early.");
            abortPrepare(UNKNOWN_ERROR);
            return;
        }
    }
    else
    	{
    	    //LOGE("do not use prefetcher");
    	}
	*/
	{
	    Mutex::Autolock autoLock(mLock);

	    mPrepareResult = OK;
	    mFlags &= ~(PREPARING|PREPARE_CANCELLED);
	    mFlags |= PREPARED;
	    LOGV("onPrepareAsyn x mFlags:%d", mFlags);
	    mAsyncPrepareEvent = NULL;
	    mPreparedCondition.broadcast();
	    postBufferingEvent_l();
	}

    if (mIsAsyncPrepare) 
	{
	 	// LOGE("use aysnc prepare");
        if (mVideoWidth < 0 || mVideoHeight < 0) {
            notifyListener_l(MEDIA_SET_VIDEO_SIZE, 0, 0);
        } else {
            notifyListener_l(MEDIA_SET_VIDEO_SIZE, mVideoWidth, mVideoHeight);
        }

        notifyListener_l(MEDIA_PREPARED);
    } else {
	    //LOGE("do not use aysnc prepare");
	}
	
	/*
    Mutex::Autolock autoLock(mLock);
    mPrepareResult = OK;
    mFlags &= ~(PREPARING|PREPARE_CANCELLED);
    mFlags |= PREPARED;
    LOGV("onPrepareAsyn x mFlags:%d", mFlags);
    mAsyncPrepareEvent = NULL;
    mPreparedCondition.broadcast();
    
    LOGD("start post buffering event");
    postBufferingEvent_l();
    LOGD("end post buffering event");
    */
    
}

status_t AwesomePlayer::suspend() {

    LOGD("suspend is not implemented");
	return ERROR_UNSUPPORTED;
	/*
	Mutex::Autolock autoLock(mLock);

	if  (mSuspensionState != NULL)  {
	
		if  (mLastVideoBuffer == NULL)  {
			// go into here if video is suspended again
			// after resuming without being played between
			// them
			SuspensionState *state = mSuspensionState;
			mSuspensionState = NULL;
			reset_l();
			mSuspensionState = state;
			return OK;
        	}

		delete mSuspensionState;
		mSuspensionState = NULL;
	}

   	 if  (mFlags & PREPARING)  {
        	mFlags |= PREPARE_CANCELLED;
        	if (mConnectingDataSource != NULL) {
			//LOGI("interrupting the connection process");
			//mConnectingDataSource->disconnect();
        }
        
        if  (mPPDataSource.get() != NULL)  {
            LOGI("interrupting the connection process");
            mPPDataSource->closeStream();
        }
        
    }

	while  (mFlags & PREPARING)  {
		mPreparedCondition.wait(mLock);
	}

	SuspensionState *state = new SuspensionState;
	state->mUri = mUri;
	state->mUriHeaders = mUriHeaders;
	state->mFileSource = mFileSource;

	state->mFlags = mFlags & (PLAYING | LOOPING | AT_EOS);
	getPosition(&state->mPositionUs);

	if (mLastVideoBuffer) {
		size_t size = mLastVideoBuffer->range_length();
		
		if (size) {
			state->mLastVideoFrameSize = size;
			state->mLastVideoFrame = malloc(size);
			memcpy(state->mLastVideoFrame,
			(const uint8_t *)mLastVideoBuffer->data() + mLastVideoBuffer->range_offset(), size);

			state->mVideoWidth = mVideoWidth;
			state->mVideoHeight = mVideoHeight;

			sp<MetaData> meta = mVideoSource->getFormat();
			CHECK(meta->findInt32(kKeyColorFormat, &state->mColorFormat));
			CHECK(meta->findInt32(kKeyWidth, &state->mDecodedWidth));
			CHECK(meta->findInt32(kKeyHeight, &state->mDecodedHeight));
		}
	}

	reset_l();

	mSuspensionState = state;

	return OK;
	*/
}

status_t AwesomePlayer::resume() {
    LOGD("resume is not implemented");
    return INVALID_OPERATION;
    /*
    Mutex::Autolock autoLock(mLock);

    if (mSuspensionState == NULL) {
        return INVALID_OPERATION;
    }

    SuspensionState *state = mSuspensionState;
    mSuspensionState = NULL;

    status_t err;
    if (state->mFileSource != NULL) {
        err = setDataSource_l(state->mFileSource);

        if (err == OK) {
            mFileSource = state->mFileSource;
        }
    } else {
        err = setDataSource_l(state->mUri, &state->mUriHeaders);
    }

    if (err != OK) {
        delete state;
        state = NULL;

        return err;
    }

    seekTo_l(state->mPositionUs);

    mFlags = state->mFlags & (LOOPING | AT_EOS);
    LOGV("resume mFlags:%d", mFlags);

    if (state->mLastVideoFrame && mSurface != NULL) {
		
        	mVideoRenderer =
            		new AwesomeLocalRenderer(
                    			true,  // previewOnly
                    			"",
                    			(OMX_COLOR_FORMATTYPE)state->mColorFormat,
                    			mISurface,
                    			state->mVideoWidth,
                    			state->mVideoHeight,
                    			state->mDecodedWidth,
                    			state->mDecodedHeight);

		mVideoRendererIsPreview = true;

		((AwesomeLocalRenderer *)mVideoRenderer.get())->render(
					state->mLastVideoFrame, state->mLastVideoFrameSize);
		
    }

    if (state->mFlags & PLAYING) {
        play_l();
    }

    mSuspensionState = state;
    state = NULL;

    return OK;
    */
}

uint32_t AwesomePlayer::flags() const {
	LOGD("flags()");
    return mExtractorFlags;
}


status_t AwesomePlayer::startCompatibilityTest()
{
    mRunningCompatibilityTest = true;
    return play();
}

void AwesomePlayer::stopCompatibilityTest()
{
    mRunningCompatibilityTest = false;
}

}  // namespace android

