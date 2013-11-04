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

#ifndef PP_PIPE_EXTRACTOR_H_

#define PP_PIPE_EXTRACTOR_H_

#include "include-pp/MediaDefs.h"
#include "include-pp/MediaExtractor.h"
#include "include-pp/MediaSource.h"
#include "include-pp/MediaErrors.h"
#include "include-pp/sf/MetaData.h"
#include "include-pp/MediaBuffer.h"
#include "include-pp/MediaDebug.h"

//#include <media/stagefright/MediaBuffer.h>
//#include <media/stagefright/MediaBufferGroup.h>
//#include <media/stagefright/MediaDebug.h>
//#include <media/stagefright/MetaData.h>
#include <utils/List.h>

namespace android {

class DataSource;
class String8;

enum TRACK_TYPE
{
	AUDIO_TRACK,
	VIDEO_TRACK,
};

struct PPPipeExtractor : public MediaExtractor {
    PPPipeExtractor(int fd=0);

    virtual sp<MediaSource> getAudioTrack();
    virtual sp<MediaSource> getVideoTrack();

    virtual void readVideoSample(MediaBuffer **out);
    virtual void readAudioSample(MediaBuffer **out);
    virtual status_t start(MetaData *params = NULL);
    virtual void stop();
    void setReadFD(int fd){mFd=fd;}
    void readCodecSpecificData();

    virtual ~PPPipeExtractor();
protected:
    static void *ThreadWrapper(void *me);
    void threadEntry();

    void parseCodecSpecificData();

    //just for pass compile.
    virtual size_t countTracks(){return 0;}
    virtual sp<MediaSource> getTrack(size_t index){return NULL;}
    virtual sp<MetaData> getTrackMetaData(
            size_t index, uint32_t flags = 0){return NULL;}

private:

    int mFd;
    bool mStarted;
    Mutex mLock;
    //MediaBufferGroup *mVideoBufferGroup;
    //MediaBufferGroup *mAudioBufferGroup;
    List<MediaBuffer *> mVideoBufferGroup;
    List<MediaBuffer *> mAudioBufferGroup;
    Condition mVideoBufferGroupCondition;
    Condition mAudioBufferGroupCondition;

    MediaBuffer* mAudioCodecData;
    MediaBuffer* mVideoCodecData;

	
    size_t mTrackCount;
    size_t mAudioTrackID;
    size_t mVideoTrackID;
    pthread_t mThread;
    size_t mNALLengthSize;// The number of bytes used to encode the length of a NAL unit.

    PPPipeExtractor(const PPPipeExtractor &);
    PPPipeExtractor &operator=(const PPPipeExtractor &);
	uint8_t* binarysearch(uint8_t* src, uint32_t src_size, uint8_t* dst, uint32_t dst_size);
	uint8_t* findNextVideo(uint8_t* start, uint32_t size);
};


}  // namespace android

#endif  // PP_PIPE_EXTRACTOR_H_