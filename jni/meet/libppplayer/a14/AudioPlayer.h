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

#ifndef AUDIO_PLAYER_H_

#define AUDIO_PLAYER_H_

//#include "include-pp/MediaPlayerInterface.h"

#include "include-pp/a14/frameworks/base/include/media/stagefright/MediaBuffer.h"
#include "include-pp/a14/frameworks/base/include/media/stagefright/TimeSource.h"
#include "include-pp/a14/frameworks/base/include/utils/threads.h"
//#include "include-pp/a14/media/AudioTrack.h"
//#include "platform-pp/PPPlatForm.h"
//#include <media/stagefright/TimeSource.h>
//#include <media/stagefright/MediaBuffer.h>
#include "libppplayer/a14/AudioSink.h"

#define OS_ANDROID
namespace android {
#include "player/player.h"
}

namespace android {

class MediaSource;
//class AudioTrack;

class AudioPlayer : public TimeSource {
public:
    enum {
        REACHED_EOS,
        SEEK_COMPLETE
    };

    //AudioPlayer(const sp<MediaPlayerBase::AudioSink> &audioSink);
    AudioPlayer(const sp<AudioSink> &audioSink);
    virtual ~AudioPlayer();

    // Caller retains ownership of "source".
    void setSource(const sp<MediaSource> &source);

    // Return time in us.
    virtual int64_t getRealTimeUs();

    status_t start(bool sourceAlreadyStarted = false);

    void pause();
    void resume();

    void stop();

    // Returns the timestamp of the last buffer played (in us).
    int64_t getMediaTimeUs();

    // Returns true iff a mapping is established, i.e. the AudioPlayer
    // has played at least one frame of audio.
    bool getMediaTimeMapping(int64_t *realtime_us, int64_t *mediatime_us);

    status_t seekTo(int64_t time_us);

    bool isSeeking();
    bool reachedEOS(status_t *finalStatus);
	
    void setListener(IPlayer* listener);
	status_t startCompatibilityTest(bool sourceAlreadyStarted);
	void stopCompatibilityTest();

private:
    sp<MediaSource> mSource;
    //AudioTrack *mAudioTrack;

    MediaBuffer *mInputBuffer;

    int mSampleRate;
    int64_t mLatencyUs;
    size_t mFrameSize;

    Mutex mLock;
    int64_t mNumFramesPlayed;

    int64_t mPositionTimeMediaUs;
    int64_t mPositionTimeRealUs;

    bool mSeeking;
    bool mReachedEOS;
    status_t mFinalStatus;
    int64_t mSeekTimeUs;

    bool mStarted;
	bool mStopping;

    //sp<MediaPlayerBase::AudioSink> mAudioSink;
    sp<AudioSink> mAudioSink;

    static void AudioCallback(int event, void *user, void *info);
    void AudioCallback(int event, void *info);

    static size_t AudioSinkCallback(
            //MediaPlayerBase::AudioSink *audioSink,
            AudioSink *audioSink,
            void *data, size_t size, void *me);

    size_t fillBuffer(void *data, size_t size);

    int64_t getRealTimeUsLocked() const;

    AudioPlayer(const AudioPlayer &);
    AudioPlayer &operator=(const AudioPlayer &);
	status_t initAudioSink();

    void notifyListener_l(int msg, int ext1 = 0, int ext2 = 0);
	
    IPlayer* mListener;
    bool mRunningCompatibilityTest;
};

}  // namespace android

#endif  // AUDIO_PLAYER_H_
