/* //device/extlibs/pv/android/PPAudioTrack.cpp
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
#define LOG_TAG "PPAudioTrack_cm"
#include "include-pp/ppAudioTrack_cm.h"

namespace android {

PPAudioTrack_cm::PPAudioTrack_cm()
{
    m_p = NULL;
}

PPAudioTrack_cm::PPAudioTrack_cm(
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
    m_p = new AudioTrack_cm(
                streamType,
                sampleRate,
                format,
                channels,
                frameCount,
                flags,
                cbf,
                user,
                notificationFrames,
                sessionId);
}


PPAudioTrack_cm::PPAudioTrack_cm(
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
{
    m_p = new AudioTrack_cm(
                streamType,
                sampleRate,
                format,
                channels,
                sharedBuffer,
                flags,
                cbf,
                user,
                notificationFrames,
                sessionId);
}

PPAudioTrack_cm::~PPAudioTrack_cm()
{
}

uint32_t PPAudioTrack_cm::frameCount() const
{
    return m_p->frameCount();
}
int PPAudioTrack_cm::channelCount() const
{
    return m_p->channelCount();
}
int PPAudioTrack_cm::frameSize() const
{
    return m_p->frameSize();
}
status_t PPAudioTrack_cm::getPosition(uint32_t *position)
{
    return m_p->getPosition(position);
}
status_t PPAudioTrack_cm::setVolume(float left, float right)
{
    return m_p->setVolume(left, right);
}
void PPAudioTrack_cm::start()
{
    m_p->start();
}
uint32_t PPAudioTrack_cm::latency() const
{
    return m_p->latency();
}
ssize_t PPAudioTrack_cm::write(const void* buffer, size_t size)
{
    return m_p->write(buffer, size);
}
void PPAudioTrack_cm::stop()
{
    m_p->stop();
}
void PPAudioTrack_cm::flush()
{
    m_p->flush();
}
void PPAudioTrack_cm::pause()
{
    m_p->pause();
}
status_t PPAudioTrack_cm::initCheck() const
{
    return m_p->initCheck();
}

}; // namespace android

