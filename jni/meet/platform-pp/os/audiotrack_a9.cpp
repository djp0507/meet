/* //device/extlibs/pv/android/AudioTrack_A9.cpp
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
#define LOG_TAG "AudioTrack_A9"

#include "include-pp/AudioTrack.h"
#include "os/audiotrack_a9.h"

namespace android {


AudioTrack_A9::AudioTrack_A9(
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
{
    m_p = new AudioTrack(
                streamType,
                sampleRate,
                format,
                channels,
                frameCount,
                flags,
                cbf,
                user,
                notificationFrames);
}

AudioTrack_A9::~AudioTrack_A9()
{
}

uint32_t AudioTrack_A9::frameCount() const
{
    return m_p->frameCount();
}
int AudioTrack_A9::channelCount() const
{
    return m_p->channelCount();
}
int AudioTrack_A9::frameSize() const
{
    return m_p->frameSize();
}
status_t AudioTrack_A9::getPosition(uint32_t *position)
{
    return m_p->getPosition(position);
}
status_t AudioTrack_A9::setVolume(float left, float right)
{
    return m_p->setVolume(left, right);
}
void AudioTrack_A9::start()
{
    m_p->start();
}
uint32_t AudioTrack_A9::latency() const
{
    return m_p->latency();
}
ssize_t AudioTrack_A9::write(const void* buffer, size_t size)
{
    return m_p->write(buffer, size);
}
void AudioTrack_A9::stop()
{
    m_p->stop();
}
void AudioTrack_A9::flush()
{
    m_p->flush();
}
void AudioTrack_A9::pause()
{
    m_p->pause();
}
status_t AudioTrack_A9::initCheck() const
{
    return m_p->initCheck();
}

}; // namespace android

