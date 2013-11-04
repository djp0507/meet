/*
 * Copyright (C) 2010 The Android Open Source Project
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

#ifndef PP_EXTRACTOR_H_

#define PP_EXTRACTOR_H_

#include "include-pp/a14/frameworks/base/include/media/stagefright/MediaBuffer.h"
#include "include-pp/a14/frameworks/base/include/media/stagefright/MediaSource.h"
#include "include-pp/a14/frameworks/base/include/media/stagefright/MediaErrors.h"
#include "include-pp/a14/frameworks/base/include/media/stagefright/MetaData.h"
#include "include-pp/a14/frameworks/base/include/media/stagefright/MediaDebug.h"

#include "libppplayer/a14/PPMediaBufferGroup.h"
#include "libppplayer/a14/MediaDefs.h"
#include "libppplayer/a14/MediaExtractor.h"

//#include <media/stagefright/MediaDebug.h>
//#include <media/stagefright/MediaBuffer.h>
//#include <media/stagefright/MetaData.h>

namespace android {
	
struct AMessage;
class DataSource;
class String8;

struct PPExtractor : public MediaExtractor {
    PPExtractor(const sp<DataSource> &source);

    virtual size_t countTracks();
    virtual sp<MediaSource> getTrack(size_t index);
    virtual sp<MetaData> getTrackMetaData(size_t index, uint32_t flags = 0);

    virtual sp<MetaData> getMetaData();
    virtual status_t readVideoSample(MediaBuffer **out);
    virtual status_t readAudioSample(MediaBuffer **out);
    virtual status_t start(MetaData *params = NULL);
    virtual void stop();
	void seekTo(int64_t seekTimeUs);
	int64_t getCachedDurationUs();
	virtual uint32_t flags() const;
	bool isBuffering();
    bool reachedEndOfStream();

protected:
    virtual ~PPExtractor();
    static void *ThreadWrapper(void *me);
    void threadEntry();
    size_t constructNalus(void* dst, const void* src, size_t size);
    size_t parseNALSize(const uint8_t *data) const;

private:

    sp<DataSource> mDataSource; 
    bool mStarted;
    Mutex mLock;
    PPMediaBufferGroup *mVideoBufferGroup;
    PPMediaBufferGroup *mAudioBufferGroup;
    size_t mTrackCount;
    size_t mAudioTrackID;
    size_t mVideoTrackID;
    sp<MetaData> mAudioFormat;
    sp<MetaData> mVideoFormat;
    pthread_t mThread;
    size_t mNALLengthSize;// The number of bytes used to encode the length of a NAL unit.
    Condition mPauseCondition;
    Condition mRestartCondition;
    int64_t mSeekTimeUs;
    bool isSeeking;
	bool isBlocking;
	bool isStreamDone;
    int64_t mLastSampleTime;
	int64_t mDurationTimeUs;

    PPExtractor(const PPExtractor &);
    PPExtractor &operator=(const PPExtractor &);
};

bool SniffPP(
        const sp<DataSource> &source, String8 *mimeType, float *confidence, sp<AMessage> *meta);

}  // namespace android

#endif  // PP_EXTRACTOR_H_
