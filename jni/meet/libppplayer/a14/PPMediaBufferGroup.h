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

#ifndef MEDIA_BUFFER_GROUP_H_

#define MEDIA_BUFFER_GROUP_H_

#include "include-pp/a14/frameworks/base/include/media/stagefright/MediaBuffer.h"
#include "include-pp/a14/frameworks/base/include/utils/Errors.h"
#include "include-pp/a14/frameworks/base/include/utils/threads.h"
//#include <media/stagefright/MediaBuffer.h>

namespace android {

class MediaBuffer;
class MetaData;

class PPMediaBufferGroup : public MediaBufferObserver {
public:
    PPMediaBufferGroup();
    PPMediaBufferGroup(size_t size, size_t playBufferingCount);
    ~PPMediaBufferGroup();

    // Blocks until a buffer is available and returns it to the caller,
    // the returned buffer will have a reference count of 1.
    status_t acquire_buffer(MediaBuffer **buffer);
    void add_buffer(MediaBuffer *buffer);
    status_t consume_buffer(MediaBuffer **buffer);

	//new apis for ppplayer.
    status_t push_buffer(uint8_t *data, size_t size, int64_t frameTime, bool isSync=false);
    status_t pop_buffer(MediaBuffer **buffer);
    void clear_buffer(int64_t seekTime);
    void clear_all_buffer();
    void notify_streamdone();
	void notify_stop();
	void notify_seekstart();
	void notify_seekdone();
	int64_t getCachedDurationUs(); //us
	bool isBuffering();

protected:
    virtual void signalBufferReturned(MediaBuffer *buffer);
    status_t acquire_buffer(MediaBuffer **buffer, size_t len);

private:
    friend class MediaBuffer;

    Mutex mLock;
    Condition mCondition;
    Condition mConditionBuffering;
    bool mStartBuffering;
    bool mIsStreamDone;
    bool mIsSeeking;
    bool mIsStopping;
    size_t mBufferCount;
    size_t mBufferingCount;
    size_t mPlayBufferingCount;

    uint8_t* mBuffer;
    size_t mSize;
    uint8_t* mReadingAddr, *mWritingAddr;
    MediaBuffer *mFirstBuffer, *mLastBuffer, *mReadingBuffer, *mWritingBuffer;

	int64_t mLastSampleTime;

    PPMediaBufferGroup(const PPMediaBufferGroup &);
    PPMediaBufferGroup &operator=(const PPMediaBufferGroup &);
};

}  // namespace android

#endif  // MEDIA_BUFFER_GROUP_H_
