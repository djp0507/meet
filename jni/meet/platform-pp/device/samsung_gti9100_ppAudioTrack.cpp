/* //device/extlibs/pv/android/Samsung_GTi9100_PPAudioTrack.cpp
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
#define LOG_TAG "Samsung_GTi9100_PPAudioTrack"

#define OMAP_ENHANCEMENT
#define TARGET_OMAP4
#define NPA_BUFFERS
#define ARM_4K_PAGE_SIZE 4096

#include "device/samsung_gti9100_ppAudioTrack.h"

namespace android {

Samsung_GTi9100_PPAudioTrack::Samsung_GTi9100_PPAudioTrack()
{
    m_p = NULL;
}

Samsung_GTi9100_PPAudioTrack::Samsung_GTi9100_PPAudioTrack(
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
    LOGE("Constructing");
    m_p = new AudioTrack(
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


Samsung_GTi9100_PPAudioTrack::Samsung_GTi9100_PPAudioTrack(
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
    m_p = new AudioTrack(
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

Samsung_GTi9100_PPAudioTrack::~Samsung_GTi9100_PPAudioTrack()
{
}

uint32_t Samsung_GTi9100_PPAudioTrack::frameCount() const
{
    return m_p->frameCount();
}
int Samsung_GTi9100_PPAudioTrack::channelCount() const
{
    return m_p->channelCount();
}
int Samsung_GTi9100_PPAudioTrack::frameSize() const
{
    return m_p->frameSize();
}
status_t Samsung_GTi9100_PPAudioTrack::getPosition(uint32_t *position)
{
    return m_p->getPosition(position);
}
status_t Samsung_GTi9100_PPAudioTrack::setVolume(float left, float right)
{
    return m_p->setVolume(left, right);
}
void Samsung_GTi9100_PPAudioTrack::start()
{
    m_p->start();
}
uint32_t Samsung_GTi9100_PPAudioTrack::latency() const
{
    return m_p->latency();
}
ssize_t Samsung_GTi9100_PPAudioTrack::write(const void* buffer, size_t size)
{
    return m_p->write(buffer, size);
}
void Samsung_GTi9100_PPAudioTrack::stop()
{
    m_p->stop();
}
void Samsung_GTi9100_PPAudioTrack::flush()
{
    m_p->flush();
}
void Samsung_GTi9100_PPAudioTrack::pause()
{
    m_p->pause();
}
status_t Samsung_GTi9100_PPAudioTrack::initCheck() const
{
    return m_p->initCheck();
}

}; // namespace android

