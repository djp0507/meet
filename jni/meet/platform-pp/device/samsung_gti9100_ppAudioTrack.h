/*
 * Copyright (C) 2007 The Android Open Source Project
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

#ifndef ANDROID_SAMSUNG_GTI9100_PPAUDIOTRACK_H
#define ANDROID_SAMSUNG_GTI9100_PPAUDIOTRACK_H

#include "device/samsung_gti9100_AudioSystem.h"
#include "device/samsung_gti9100_AudioTrack.h"
#include "include-pp/IPPAudioTrack.h"

#include <stdint.h>
#include <sys/types.h>

namespace android {

// ----------------------------------------------------------------------------

class audio_track_cblk_t;

// ----------------------------------------------------------------------------

class Samsung_GTi9100_PPAudioTrack : public IPPAudioTrack
{
public:

    typedef void (*callback_t)(int event, void* user, void *info);

    Samsung_GTi9100_PPAudioTrack();

    Samsung_GTi9100_PPAudioTrack( int streamType,
                uint32_t sampleRate  = 0,
                int format           = 0,
                int channels         = 0,
                int frameCount       = 0,
                uint32_t flags       = 0,
                callback_t cbf       = 0,
                void* user           = 0,
                int notificationFrames = 0,
                int sessionId = 0);

    Samsung_GTi9100_PPAudioTrack( int streamType,
                uint32_t sampleRate = 0,
                int format          = 0,
                int channels        = 0,
                const sp<IMemory>& sharedBuffer = 0,
                uint32_t flags      = 0,
                callback_t cbf      = 0,
                void* user          = 0,
                int notificationFrames = 0,
                int sessionId = 0);

    virtual uint32_t    frameCount() const;
    virtual int         channelCount() const;
    virtual int         frameSize() const;
    virtual status_t    getPosition(uint32_t *position);
    virtual status_t    setVolume(float left, float right);
    virtual void        start();
    virtual uint32_t     latency() const;
    virtual ssize_t     write(const void* buffer, size_t size);
    virtual void        stop();
    virtual void        flush();
    virtual void        pause();
    virtual status_t    initCheck() const;
	
    ~Samsung_GTi9100_PPAudioTrack();



private:

    AudioTrack* m_p;



};


}; // namespace android

#endif // ANDROID_SAMSUNG_GTI9100_PPAUDIOTRACK_H