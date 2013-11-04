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
#define LOG_TAG "HUAWEI_U8800"

#include "include-pp/OMXClient.h"
#include "include-pp/OMXCodec.h"
#include "include-pp/AudioSystem.h"
#include "include-pp/ppAudioTrack.h"

#include "device/huawei_u8800.h"

extern "C"
{
#include <dlfcn.h>
}
namespace android {

//extern const char *MODEL_NAME;
//extern const char *RELEASE_VERSION;
//extern const char *PRODUCT_VERSION;
extern GET_AUDIOTRACK_FUN CREATE_AUDIOTRACK_FUN;

class HUAWEI_U8800_IOMXRenderer : public IPPOMXRenderer
{
public:
	HUAWEI_U8800_IOMXRenderer(sp<IOMXRenderer>& target) 
		: mTarget(target)
	{
	}
    virtual void render(MediaBuffer* buffer)
	{
        void *id;
        if (buffer->meta_data()->findPointer(kKeyBufferID, &id)) {
            //mTarget->render((IOMX::buffer_id)id);
            mTarget->render((void*)id);
        }
	}
	virtual ~HUAWEI_U8800_IOMXRenderer()
	{
        //IPCThreadState::self()->flushCommands();
	}

private:
	sp<IOMXRenderer> mTarget;
};

void HUAWEI_U8800::FlushCommands()
{
	IPCThreadState::self()->flushCommands();
}

HUAWEI_U8800::HUAWEI_U8800()
{
    mClient = new OMXClient();
    CHECK_EQ(mClient->connect(), OK);
}

HUAWEI_U8800::~HUAWEI_U8800()
{
	mClient->disconnect();
	delete mClient;
	mClient = NULL;
	//IPCThreadState::self()->flushCommands();
}

IPPOMXRenderer* HUAWEI_U8800::CreateOMXRenderer(
            //const sp<IOMX> &omx,
            const sp<Surface> &surface,
            const char *componentName,
            OMX_COLOR_FORMATTYPE colorFormat,
            size_t encodedWidth, size_t encodedHeight,
            size_t displayWidth, size_t displayHeight)
{	
    LOGD("Start adjust encodedWidth:%d, encodedHeight:%d", encodedWidth, encodedHeight);
	//adjust width
	if(encodedWidth%4 != 0)
	{
		LOGD("Adjust encodedWidth +2");
		encodedWidth+=2;//to fix picture corrupt issue.
	}
	if(encodedWidth%8 != 0)
	{
		LOGD("Adjust encodedWidth +4");
		encodedWidth+=4;//to fix picture corrupt issue.
	}
	if(encodedWidth%16 != 0)
	{
		LOGD("Adjust encodedWidth +8");
		encodedWidth+=8;//to fix picture corrupt issue.
	}
	
	//adjust height
	if(encodedHeight%4 != 0)
	{
		LOGD("Adjust encodedHeight +2");
		encodedHeight+=2;//to fix picture corrupt issue.
	}
	if(encodedHeight%8 != 0)
	{
		LOGD("Adjust encodedHeight +4");
		encodedHeight+=4;//to fix picture corrupt issue.
	}
	if(encodedHeight%16 != 0)
	{
		LOGD("Adjust encodedHeight +8");
		encodedHeight+=8;//to fix picture corrupt issue.
	}

    const sp<ISurface> isurface = surface->getISurface();
    sp<IOMXRenderer> target = (mClient->interface())->createRenderer(
								isurface, 
		                        componentName,
		                        colorFormat,
		                        encodedWidth, 
		                        encodedHeight,
		                        displayWidth, 
		                        displayHeight,
		                        0);
	if(target == NULL) return NULL;
	return new HUAWEI_U8800_IOMXRenderer(target);
}
	
sp<MediaSource> HUAWEI_U8800::CreateOMXCodec(
            //const sp<IOMX> &omx,
            const sp<MetaData> &meta, bool createEncoder,
            const sp<MediaSource> &source,
            const char *matchComponentName,
            uint32_t flags)
{
	const char *mime;
	CHECK(meta->findCString(kKeyMIMEType, &mime));
	bool bVideo = (!strncasecmp(mime, "video/", 6));
	
	sp<MediaSource> codec=NULL;

    if(bVideo) //video
    {
	    if(codec == NULL)
		{
		    LOGD("trying internal video codec");
			codec = OMXCodec::Create(
	                mClient->interface(),//omx,
	                meta,
	                createEncoder,
	                source,
	                matchComponentName,
	                flags);
			if(codec != NULL)
			{
			    LOGD("Create internal codec success");
			}
		}
    }
	else //audio
	{
	    if(codec == NULL)
		{
		    LOGD("trying internal audio codec");
			codec = OMXCodec::Create(
	                mClient->interface(),//omx,
	                meta,
	                createEncoder,
	                source,
	                matchComponentName,
	                0);
			if(codec != NULL)
			{
			    LOGD("Create internal codec success");
			}
		}
	}
	
	if(codec == NULL)
	{
        LOGE("create %s codec failed", bVideo?"video":"audio");
	}
	return codec;
}

IPPAudioTrack* HUAWEI_U8800::createAudioTrack(
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
    /*
    return new PPAudioTrack(streamType,
		sampleRate,
        format,
        channels,
        frameCount,
        flags,
        cbf,
        user,
        notificationFrames,
        sessionId);
        */
	if(CREATE_AUDIOTRACK_FUN != NULL)
	{
		return CREATE_AUDIOTRACK_FUN(
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
	return NULL;
}

IPPAudioTrack* HUAWEI_U8800::createAudioTrack(
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
    /*
    return new PPAudioTrack(streamType,
        sampleRate,
        format,
        channels,
        sharedBuffer,
        flags,
        cbf,
        user,
        notificationFrames,
        sessionId);
        */
    return NULL;
}
	
status_t HUAWEI_U8800::getOutputSamplingRate(int* samplingRate, int stream)
{
    return AudioSystem::getOutputSamplingRate(samplingRate, stream);
}
	
status_t HUAWEI_U8800::getOutputFrameCount(int* frameCount, int stream)
{
    return AudioSystem::getOutputFrameCount(frameCount, stream);
}
	
bool HUAWEI_U8800::canAudioHWDecode()
{
    return true;
}
	
bool HUAWEI_U8800::canAudioHWSBR()
{
    return true;
}

}
