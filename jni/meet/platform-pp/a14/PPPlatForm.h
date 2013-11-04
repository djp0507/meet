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
//#include "include-pp/IPPAudioTrack.h"

#include "include-pp/a14/frameworks/base/include/surfaceflinger/Surface.h"
#include "include-pp/a14/frameworks/base/include/media/stagefright/MetaData.h"

#include "include-pp/a14/system/core/include/system/audio.h"
#include "include-pp/a14/frameworks/base/include/binder/IMemory.h"
#include "include-pp/a14/frameworks/base/include/binder/IPCThreadState.h"
#include "include-pp/a14/frameworks/base/include/media/stagefright/openmax/OMX_IVCommon.h"

namespace android {

class IPPAudioTrack;
class MediaBuffer;
struct MediaSource;

typedef void (*audio_error_callback)(status_t err);
typedef int audio_io_handle_t;
typedef void (*callback_t)(int event, void* user, void *info);
/*
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
*/

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
            void* surface,
            int32_t rotationDegrees);
};


class OMXCodecWrapper
{
public:
    static sp<MediaSource> Create(
            //const sp<IOMX> &omx,
            const sp<MetaData> &meta, bool createEncoder,
            const sp<MediaSource> &source,
            void* surface = NULL,
            const char *matchComponentName = NULL,
            uint32_t flags = 0);
};


class AudioSystemWrapper
{
public:
    static status_t getOutputSamplingRate(int* samplingRate, int stream = AUDIO_FORMAT_DEFAULT);
    static status_t getOutputFrameCount(int* frameCount, int stream = AUDIO_FORMAT_DEFAULT);
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
            const sp<Surface> &surface,
            int32_t rotationDegrees) = 0;
	
	virtual sp<MediaSource> CreateOMXCodec(
            //const sp<IOMX> &omx,
            const sp<MetaData> &meta, bool createEncoder,
            const sp<MediaSource> &source,
            const sp<Surface> &surface,
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
	
    virtual status_t getOutputSamplingRate(int* samplingRate, int stream = AUDIO_FORMAT_DEFAULT) = 0;
    virtual status_t getOutputFrameCount(int* frameCount, int stream = AUDIO_FORMAT_DEFAULT) = 0;
	virtual bool canAudioHWDecode() = 0;
	virtual bool canAudioHWSBR() = 0;
	virtual void FlushCommands() = 0;
	virtual ~IDevice(){}

};

}

#endif  // PP_PLATFORM_H_
