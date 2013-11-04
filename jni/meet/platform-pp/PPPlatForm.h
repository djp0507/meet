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

#ifndef PP_PLATFORM_H_

#define PP_PLATFORM_H_

//#include "include-pp/MediaBuffer.h"
//#include "include-pp/MediaSource.h"
#include "include-pp/IPPAudioTrack.h"
#include "include-pp/Surface.h"
#include "include-pp/sf/MetaData.h"

#include "include-pp/binder/IMemory.h"
#include "include-pp/binder/IPCThreadState.h"
#include <OMX_IVCommon.h>

namespace android {

class MediaBuffer;
struct MediaSource;

typedef void (*audio_error_callback)(status_t err);
typedef int audio_io_handle_t;
typedef void (*callback_t)(int event, void* user, void *info);

typedef IPPAudioTrack* (*GET_AUDIOTRACK_FUN)(
	        int streamType,
	        uint32_t sampleRate,
	        int format,
	        int channels,
            int frameCount,
	        uint32_t flags,
	        callback_t cbf,
	        void* user,
	        int notificationFrames,
	        int sessionId);


class IPPOMXRenderer
{
public:
    virtual void render(MediaBuffer* buffer) = 0;
	virtual bool ownBuffer()
	{
	    return false;
	}
	virtual int64_t getVideoRenderTimeout()
	{
		return 40000;
	}
	virtual ~IPPOMXRenderer(){}
};

class OMXWrapper
{
public:
    static bool Create();
    static void Destroy();
	static void FlushCommands();
};

class OMXRendererWrapper {
public:
	static IPPOMXRenderer* Create(
            //const sp<IOMX> &omx,
            void* surface,
            const char *componentName,
            OMX_COLOR_FORMATTYPE colorFormat,
            size_t encodedWidth, size_t encodedHeight,
            size_t displayWidth, size_t displayHeight);
};


class OMXCodecWrapper
{
public:
	enum CreationFlags {
        kPreferSoftwareCodecs    = 1,
        kIgnoreCodecSpecificData = 2,

        // The client wants to access the output buffer's video
        // data for example for thumbnail extraction.
        kClientNeedsFramebuffer  = 4,
    };
	
    static sp<MediaSource> Create(
            //const sp<IOMX> &omx,
            const sp<MetaData> &meta, bool createEncoder,
            const sp<MediaSource> &source,
            const char *matchComponentName = NULL,
            uint32_t flags = 0);
};


class AudioSystemWrapper
{
public:
    enum stream_type {
        DEFAULT          =-1,
        VOICE_CALL       = 0,
        SYSTEM           = 1,
        RING             = 2,
        MUSIC            = 3,
        ALARM            = 4,
        NOTIFICATION     = 5,
        BLUETOOTH_SCO    = 6,
        ENFORCED_AUDIBLE = 7, // Sounds that cannot be muted by user and must be routed to speaker
        DTMF             = 8,
        TTS              = 9,
#ifdef HAVE_FM_RADIO
        FM              = 10,
#endif
        NUM_STREAM_TYPES
    };
	
	// Channel mask definitions must be kept in sync with JAVA values in /media/java/android/media/AudioFormat.java
    enum audio_channels {
        // output channels
        CHANNEL_OUT_FRONT_LEFT = 0x4,
        CHANNEL_OUT_FRONT_RIGHT = 0x8,
        CHANNEL_OUT_FRONT_CENTER = 0x10,
        CHANNEL_OUT_LOW_FREQUENCY = 0x20,
        CHANNEL_OUT_BACK_LEFT = 0x40,
        CHANNEL_OUT_BACK_RIGHT = 0x80,
        CHANNEL_OUT_FRONT_LEFT_OF_CENTER = 0x100,
        CHANNEL_OUT_FRONT_RIGHT_OF_CENTER = 0x200,
        CHANNEL_OUT_BACK_CENTER = 0x400,
        CHANNEL_OUT_MONO = CHANNEL_OUT_FRONT_LEFT,
        CHANNEL_OUT_STEREO = (CHANNEL_OUT_FRONT_LEFT | CHANNEL_OUT_FRONT_RIGHT),
        CHANNEL_OUT_QUAD = (CHANNEL_OUT_FRONT_LEFT | CHANNEL_OUT_FRONT_RIGHT |
                CHANNEL_OUT_BACK_LEFT | CHANNEL_OUT_BACK_RIGHT),
        CHANNEL_OUT_SURROUND = (CHANNEL_OUT_FRONT_LEFT | CHANNEL_OUT_FRONT_RIGHT |
                CHANNEL_OUT_FRONT_CENTER | CHANNEL_OUT_BACK_CENTER),
        CHANNEL_OUT_5POINT1 = (CHANNEL_OUT_FRONT_LEFT | CHANNEL_OUT_FRONT_RIGHT |
                CHANNEL_OUT_FRONT_CENTER | CHANNEL_OUT_LOW_FREQUENCY | CHANNEL_OUT_BACK_LEFT | CHANNEL_OUT_BACK_RIGHT),
        CHANNEL_OUT_7POINT1 = (CHANNEL_OUT_FRONT_LEFT | CHANNEL_OUT_FRONT_RIGHT |
                CHANNEL_OUT_FRONT_CENTER | CHANNEL_OUT_LOW_FREQUENCY | CHANNEL_OUT_BACK_LEFT | CHANNEL_OUT_BACK_RIGHT |
                CHANNEL_OUT_FRONT_LEFT_OF_CENTER | CHANNEL_OUT_FRONT_RIGHT_OF_CENTER),
        CHANNEL_OUT_ALL = (CHANNEL_OUT_FRONT_LEFT | CHANNEL_OUT_FRONT_RIGHT |
                CHANNEL_OUT_FRONT_CENTER | CHANNEL_OUT_LOW_FREQUENCY | CHANNEL_OUT_BACK_LEFT | CHANNEL_OUT_BACK_RIGHT |
                CHANNEL_OUT_FRONT_LEFT_OF_CENTER | CHANNEL_OUT_FRONT_RIGHT_OF_CENTER | CHANNEL_OUT_BACK_CENTER),

        // input channels
        CHANNEL_IN_LEFT = 0x4,
        CHANNEL_IN_RIGHT = 0x8,
        CHANNEL_IN_FRONT = 0x10,
        CHANNEL_IN_BACK = 0x20,
        CHANNEL_IN_LEFT_PROCESSED = 0x40,
        CHANNEL_IN_RIGHT_PROCESSED = 0x80,
        CHANNEL_IN_FRONT_PROCESSED = 0x100,
        CHANNEL_IN_BACK_PROCESSED = 0x200,
        CHANNEL_IN_PRESSURE = 0x400,
        CHANNEL_IN_X_AXIS = 0x800,
        CHANNEL_IN_Y_AXIS = 0x1000,
        CHANNEL_IN_Z_AXIS = 0x2000,
        CHANNEL_IN_VOICE_UPLINK = 0x4000,
        CHANNEL_IN_VOICE_DNLINK = 0x8000,
#ifdef OMAP_ENHANCEMENT
        CHANNEL_IN_VOICE_UPLINK_DNLINK = 0x10000,
#endif
        CHANNEL_IN_MONO = CHANNEL_IN_FRONT,
        CHANNEL_IN_STEREO = (CHANNEL_IN_LEFT | CHANNEL_IN_RIGHT),
        CHANNEL_IN_ALL = (CHANNEL_IN_LEFT | CHANNEL_IN_RIGHT | CHANNEL_IN_FRONT | CHANNEL_IN_BACK|
                CHANNEL_IN_LEFT_PROCESSED | CHANNEL_IN_RIGHT_PROCESSED | CHANNEL_IN_FRONT_PROCESSED | CHANNEL_IN_BACK_PROCESSED|
                CHANNEL_IN_PRESSURE | CHANNEL_IN_X_AXIS | CHANNEL_IN_Y_AXIS | CHANNEL_IN_Z_AXIS |
#ifdef OMAP_ENHANCEMENT
                CHANNEL_IN_VOICE_UPLINK | CHANNEL_IN_VOICE_DNLINK | CHANNEL_IN_VOICE_UPLINK_DNLINK)
#else
                CHANNEL_IN_VOICE_UPLINK | CHANNEL_IN_VOICE_DNLINK )
#endif
    };
	
	enum pcm_sub_format {
        PCM_SUB_16_BIT          = 0x1, // must be 1 for backward compatibility
        PCM_SUB_8_BIT           = 0x2, // must be 2 for backward compatibility
    };
	
	enum audio_format {
        INVALID_FORMAT      = -1,
        FORMAT_DEFAULT      = 0,
        PCM                 = 0x00000000, // must be 0 for backward compatibility
        MP3                 = 0x01000000,
        AMR_NB              = 0x02000000,
        AMR_WB              = 0x03000000,
        AAC                 = 0x04000000,
        HE_AAC_V1           = 0x05000000,
        HE_AAC_V2           = 0x06000000,
        VORBIS              = 0x07000000,
        MAIN_FORMAT_MASK    = 0xFF000000,
        SUB_FORMAT_MASK     = 0x00FFFFFF,
        // Aliases
        PCM_16_BIT          = (PCM|PCM_SUB_16_BIT),
        PCM_8_BIT          = (PCM|PCM_SUB_8_BIT)
    };
	
    static status_t getOutputSamplingRate(int* samplingRate, int stream = DEFAULT);
    static status_t getOutputFrameCount(int* frameCount, int stream = DEFAULT);
	static bool canAudioHWDecode();
	static bool canAudioHWSBR();

	
	typedef void (*callback_t)(int event, void* user, void *info);
    static IPPAudioTrack* createAudioTrack( int streamType,
                                    uint32_t sampleRate  = 0,
                                    int format           = 0,
                                    int channels         = 0,
                                    int frameCount       = 0,
                                    uint32_t flags       = 0,
                                    callback_t cbf       = 0,
                                    void* user           = 0,
                                    int notificationFrames = 0,
                                    int sessionId = 0);

    static IPPAudioTrack* createAudioTrack( int streamType,
                                    uint32_t sampleRate = 0,
                                    int format          = 0,
                                    int channels        = 0,
                                    const sp<IMemory>& sharedBuffer = 0,
                                    uint32_t flags      = 0,
                                    callback_t cbf      = 0,
                                    void* user          = 0,
                                    int notificationFrames = 0,
                                    int sessionId = 0);

	
};

class IDevice
{
    public:	
	//virtual sp<IOMX> CreateOMX();
	virtual IPPOMXRenderer* CreateOMXRenderer(
            //const sp<IOMX> &omx,
            const sp<Surface> &surface,
            const char *componentName,
            OMX_COLOR_FORMATTYPE colorFormat,
            size_t encodedWidth, size_t encodedHeight,
            size_t displayWidth, size_t displayHeight) = 0;
	
	virtual sp<MediaSource> CreateOMXCodec(
            //const sp<IOMX> &omx,
            const sp<MetaData> &meta, bool createEncoder,
            const sp<MediaSource> &source,
            const char *matchComponentName = NULL,
            uint32_t flags = 0) = 0;

    virtual IPPAudioTrack* createAudioTrack( int streamType,
            uint32_t sampleRate  = 0,
            int format           = 0,
            int channels         = 0,
            int frameCount       = 0,
            uint32_t flags       = 0,
            callback_t cbf       = 0,
            void* user           = 0,
            int notificationFrames = 0,
            int sessionId = 0) = 0;

    virtual IPPAudioTrack* createAudioTrack( int streamType,
            uint32_t sampleRate = 0,
            int format          = 0,
            int channels        = 0,
            const sp<IMemory>& sharedBuffer = 0,
            uint32_t flags      = 0,
            callback_t cbf      = 0,
            void* user          = 0,
            int notificationFrames = 0,
            int sessionId = 0) = 0;
	
    virtual status_t getOutputSamplingRate(int* samplingRate, int stream = AudioSystemWrapper::DEFAULT) = 0;
    virtual status_t getOutputFrameCount(int* frameCount, int stream = AudioSystemWrapper::DEFAULT) = 0;
	virtual bool canAudioHWDecode() = 0;
	virtual bool canAudioHWSBR() = 0;
	virtual void FlushCommands() = 0;
	virtual ~IDevice(){}

};

}

#endif  // PP_PLATFORM_H_
