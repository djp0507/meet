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

//#define LOG_NDEBUG 0
#define LOG_TAG "PPPlatForm"
#include "PPPlatForm.h"

#include "platform-pp/a14/samsung_gti9000.h"
#include "platform-pp/a14/samsung_gti9300.h"

#include "include-pp/PPBox_Util.h"
#include "platform/platforminfo.h"

#include "include-pp/a14/frameworks/base/include/media/stagefright/MediaSource.h"
#include "include-pp/a14/frameworks/base/include/utils/Log.h"
#include "include-pp/a14/frameworks/base/include/media/stagefright/OMXCodec.h"


extern "C" {
#include <dlfcn.h>
}

namespace android {

extern PlatformInfo* gPlatformInfo;
static IDevice* gDevice = NULL;

static IDevice* getDevice()
{
    if(gPlatformInfo == NULL)
    {
        LOGE("gPlatformInfo is NULL");
        return NULL;
    }

    if (gDevice == NULL)
    {
        if (!strncmp(gPlatformInfo->model_name, "GT-I9300", strlen("GT-I9300"))) 
        {	
        	gDevice = new Samsung_GTi9300();
        }
        else if(!strncmp(gPlatformInfo->board_name, "s5pc110", strlen("s5pc110")))
    	{
    	    gDevice = new Samsung_GTi9000();
        }
        else
        {
    	    gDevice = new Samsung_GTi9000();
        }
    }
    
	return gDevice;
}

bool OMXWrapper::Create()
{
    gDevice = getDevice();
	return (gDevice != NULL)? true : false;
}

void OMXWrapper::Destroy()
{
    if(gDevice)
    {
		delete gDevice;
		gDevice = NULL;
    }
}

void OMXWrapper::FlushCommands()
{
    if(gDevice)
    {
		gDevice->FlushCommands();
    }
}

IPPOMXRenderer* OMXRendererWrapper::Create(
            void* surface,
            int32_t rotationDegrees)
{
    const sp<Surface> mySurface((Surface*)surface);
    return getDevice()->CreateOMXRenderer(
					            mySurface,
					            rotationDegrees);
}
	
sp<MediaSource> OMXCodecWrapper::Create(
            //const sp<IOMX> &omx,
            const sp<MetaData> &meta, bool createEncoder,
            const sp<MediaSource> &source,
            void* surface,
            const char *matchComponentName,
            uint32_t flags)
{
    const sp<Surface> mySurface((Surface*)surface);
    return getDevice()->CreateOMXCodec(
	        //omx,
	        meta,
	        createEncoder,
	        source,
	        mySurface,
	        matchComponentName,
	        flags);
}

IPPAudioTrack* AudioSystemWrapper::createAudioTrack(
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
	LOGE("createAudioTrack()");
    return getDevice()->createAudioTrack(
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

IPPAudioTrack* AudioSystemWrapper::createAudioTrack(
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
	LOGE("createAudioTrack()");
    return getDevice()->createAudioTrack(
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
	
status_t AudioSystemWrapper::getOutputSamplingRate(int* samplingRate, int stream)
{
	LOGD("getOutputSamplingRate()");
    return getDevice()->getOutputSamplingRate(samplingRate, stream);
}
	
status_t AudioSystemWrapper::getOutputFrameCount(int* frameCount, int stream)
{
	LOGD("getOutputFrameCount()");
    return getDevice()->getOutputFrameCount(frameCount, stream);
}
	
bool AudioSystemWrapper::canAudioHWDecode()
{
	LOGD("canAudioHWDecode()");
    return getDevice()->canAudioHWDecode();
}
	
bool AudioSystemWrapper::canAudioHWSBR()
{
	LOGD("canAudioHWSBR()");
    return getDevice()->canAudioHWSBR();
}

}
