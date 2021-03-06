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

#ifndef AWESOME_PLAYER_H_

#define AWESOME_PLAYER_H_

#include "include-pp/TimedEventQueue.h"
//#include "include-pp/MediaPlayerInterface.h"

#include "include-pp/AudioSink.h"
#include "include-pp/ppDataSource.h"
#include "include-pp/ppExtractor.h"
#include "include-pp/HTTPDataSource.h"
//#include "include-pp/IOMX.h"
//#include "include-pp/OMXClient.h"

//#include <media/stagefright/OMXClient.h>
//#include <media/stagefright/ColorConverter.h>
#include "include-pp/utils/threads.h"
#include "include-pp/utils/KeyedVector.h"

#define OS_ANDROID
namespace android {
#include "player/player.h"
}

namespace android {

struct AudioPlayer;
struct DataSource;
struct MediaBuffer;
struct MediaExtractor;
struct MediaSource;
struct Prefetcher;
struct TimeSource;

struct AwesomeRenderer : public RefBase {
    AwesomeRenderer() {}

    virtual void render(MediaBuffer *buffer) = 0;

	virtual ~AwesomeRenderer(){}
	virtual bool ownBuffer(){return false;}

private:
    AwesomeRenderer(const AwesomeRenderer &);
    AwesomeRenderer &operator=(const AwesomeRenderer &);
};

struct AwesomePlayer {
    AwesomePlayer();
    ~AwesomePlayer();

    //void setListener(const wp<MediaPlayerBase> &listener);
    void setListener(IPlayer* listener);

    status_t setDataSource(
            const char *uri,
            const KeyedVector<String8, String8> *headers = NULL);

    status_t setDataSource(int fd, int64_t offset, int64_t length);

    void reset();

    status_t prepare();
    status_t prepare_l();
    status_t prepareAsync();
    status_t prepareAsync_l();

    status_t play();
    status_t pause();

    bool isPlaying() const;

    //void setISurface(const sp<ISurface> &isurface);
    void setSurface(void* surface);
    //void setAudioSink(const sp<MediaPlayerBase::AudioSink> &audioSink);
    void setAudioSink(const sp<AudioSink> &audioSink);
    status_t setLooping(bool shouldLoop);

    status_t getDuration(int64_t *durationUs);
    status_t getPosition(int64_t *positionUs);

    status_t seekTo(int64_t timeUs);

    status_t getVideoDimensions(int32_t *width, int32_t *height) const;

    status_t suspend();
    status_t resume();

    // This is a mask of MediaExtractor::Flags.
    uint32_t flags() const;
	status_t startCompatibilityTest();
	void stopCompatibilityTest();

private:
    friend struct AwesomeEvent;

    enum {
        PLAYING             = 1,
        LOOPING             = 2,
        FIRST_FRAME         = 4,
        PREPARING           = 8,
        PREPARED            = 16,
        AT_EOS              = 32,
        PREPARE_CANCELLED   = 64,
    };

    mutable Mutex mLock;
    Mutex mMiscStateLock;

    //OMXClient mClient;
    //sp<IOMX> mOMX;
    TimedEventQueue mQueue;
    bool mQueueStarted;
    //wp<MediaPlayerBase> mListener;
    IPlayer* mListener;

    //sp<ISurface> mISurface;
    void* mSurface;
    //sp<MediaPlayerBase::AudioSink> mAudioSink;
    sp<AudioSink> mAudioSink;

    TimeSource *mTimeSource;

    String8 mUri;
    KeyedVector<String8, String8> mUriHeaders;

    sp<DataSource> mFileSource;

    sp<MediaSource> mVideoTrack;
    sp<MediaSource> mVideoSource;
    sp<AwesomeRenderer> mVideoRenderer;
    bool mVideoRendererIsPreview;

    sp<MediaSource> mAudioTrack;
    sp<MediaSource> mAudioSource;
    AudioPlayer *mAudioPlayer;
    int64_t mDurationUs;

    uint32_t mFlags;
    uint32_t mExtractorFlags;

    int32_t mVideoWidth, mVideoHeight;
    int32_t mDecodedWidth, mDecodedHeight;
    int64_t mTimeSourceDeltaUs;
    int64_t mVideoTimeUs;

    bool mSeeking;
    bool mSeekNotificationSent;
    int64_t mSeekTimeUs;
    int64_t mSeekingTimeUs;

	bool mReadingVideo;
	bool mFristReadingVideoDone;

    bool mWatchForAudioSeekComplete;
    bool mWatchForAudioEOS;

    sp<TimedEventQueue::Event> mVideoEvent;
    bool mVideoEventPending;
    sp<TimedEventQueue::Event> mStreamDoneEvent;
    bool mStreamDoneEventPending;
    sp<TimedEventQueue::Event> mBufferingEvent;
    bool mBufferingEventPending;
    sp<TimedEventQueue::Event> mSeekingEvent;
    bool mSeekingEventPending;
    sp<TimedEventQueue::Event> mCheckAudioStatusEvent;
    bool mAudioStatusEventPending;
    sp<TimedEventQueue::Event> mCheckSeekingStatusEvent;
    bool mSeekingStatusEventPending;

    sp<TimedEventQueue::Event> mAsyncPrepareEvent;
    Condition mPreparedCondition;
    bool mIsAsyncPrepare;
    status_t mPrepareResult;
    status_t mStreamDoneStatus;

	//bool mIsBuffering;

    void postVideoEvent_l(int64_t delayUs = -1);
    void postBufferingEvent_l();
    void postSeekingEvent_l();
    void postStreamDoneEvent_l(status_t status);
    void postCheckAudioStatusEvent_l();
    void postCheckSeekingStatusEvent_l();
    status_t play_l();

    MediaBuffer *mLastVideoBuffer;
    MediaBuffer *mVideoBuffer;
	KeyedVector<int64_t, MediaBuffer*> *mVideoBufferMap;

    sp<Prefetcher> mPrefetcher;
    sp<HTTPDataSource> mConnectingDataSource;
    sp<PPDataSource> mPPDataSource;
	sp<PPExtractor> mPPExtractor;
//	ColorConverter* mConverter;
    int64_t mVideoRenderTimeout;

    bool mRunningCompatibilityTest;
	bool mDeviceLoaded;
	

    struct SuspensionState {
        String8 mUri;
        KeyedVector<String8, String8> mUriHeaders;
        sp<DataSource> mFileSource;

        uint32_t mFlags;
        int64_t mPositionUs;

        void *mLastVideoFrame;
        size_t mLastVideoFrameSize;
        int32_t mColorFormat;
        int32_t mVideoWidth, mVideoHeight;
        int32_t mDecodedWidth, mDecodedHeight;

        SuspensionState()
            : mLastVideoFrame(NULL) {
        }

        ~SuspensionState() {
            if (mLastVideoFrame) {
                free(mLastVideoFrame);
                mLastVideoFrame = NULL;
            }
        }
    } *mSuspensionState;

    status_t setDataSource_l(
            const char *uri,
            const KeyedVector<String8, String8> *headers = NULL);

    status_t setDataSource_l(const sp<DataSource> &dataSource);
    status_t setDataSource_l(const sp<MediaExtractor> &extractor);
    void reset_l();
    status_t seekTo_l(int64_t timeUs);
    status_t pause_l();
    void initRenderer_l();
    //void seekAudioIfNecessary_l();

    void cancelPlayerEvents(bool keepBufferingGoing = false);

    void setAudioSource(sp<MediaSource> source);
    status_t initAudioDecoder();

    void setVideoSource(sp<MediaSource> source);
    status_t initVideoDecoder();

    void onStreamDone();

    void notifyListener_l(int msg, int ext1 = 0, int ext2 = 0);

    void onVideoEvent();
    void onBufferingUpdate();
    void onCheckAudioStatus();
    void onCheckSeekingStatus();
    void onPrepareAsyncEvent();
    void onSeekingEvent();
    void abortPrepare(status_t err);

    status_t finishSetDataSource_l();

    static bool ContinuePreparation(void *cookie);

    AwesomePlayer(const AwesomePlayer &);
    AwesomePlayer &operator=(const AwesomePlayer &);

	MediaBuffer* getOrderedVideoFrame();
	void checkBufferingStatus();
};

}  // namespace android

#endif  // AWESOME_PLAYER_H_

