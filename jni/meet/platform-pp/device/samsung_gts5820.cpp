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
#define LOG_TAG "samsung_gts5820"

#include "include-pp/OMXClient.h"
#include "include-pp/OMXCodec_cm.h"
#include "include-pp/OMXCodec.h"
#include "include-pp/AudioSystem.h"
#include "include-pp/ppAudioTrack.h"

#include "device/samsung_gts5820.h"

namespace android {

//extern const char *MODEL_NAME;
//extern const char *RELEASE_VERSION;
//extern const char *PRODUCT_VERSION;

extern GET_AUDIOTRACK_FUN CREATE_AUDIOTRACK_FUN;


class Samsung_GTS5820_IOMXRenderer : public IPPOMXRenderer
{
public:
	Samsung_GTS5820_IOMXRenderer(sp<IOMXRenderer>& target) 
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
	virtual ~Samsung_GTS5820_IOMXRenderer()
	{
        //IPCThreadState::self()->flushCommands();
	}

private:
	sp<IOMXRenderer> mTarget;
};

void Samsung_GTS5820::FlushCommands()
{
    LOGI("FlushCommands");
	IPCThreadState::self()->flushCommands();
}

Samsung_GTS5820::Samsung_GTS5820()
{
    mClient = new OMXClient();
    CHECK_EQ(mClient->connect(), OK);
}

Samsung_GTS5820::~Samsung_GTS5820()
{
	mClient->disconnect();
	delete mClient;
	mClient = NULL;
	//IPCThreadState::self()->flushCommands();
}

IPPOMXRenderer* Samsung_GTS5820::CreateOMXRenderer(
            //const sp<IOMX> &omx,
            const sp<Surface> &surface,
            const char *componentName,
            OMX_COLOR_FORMATTYPE colorFormat,
            size_t encodedWidth, size_t encodedHeight,
            size_t displayWidth, size_t displayHeight)
{	

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
	return new Samsung_GTS5820_IOMXRenderer(target);
}

sp<MediaSource> Samsung_GTS5820::CreateOMXCodec(
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
			//LOGD("trying internal hardware codec");
		    LOGD("trying cyanogenmod video hardware codec");
			codec = OMXCodec_cm::Create(
	                mClient->interface(),//omx,
	                meta,
	                createEncoder,
	                source,
	                "OMX.PV.avcdec",
	                flags);
			if(codec != NULL)
			{
				//LOGD("Create internal video codec success");
			    LOGD("Create cyanogenmod video hardware codec success");
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
        LOGE("create %s codec failed", bVideo? "video" : "audio");
	}
	return codec;
}

IPPAudioTrack* Samsung_GTS5820::createAudioTrack(
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

IPPAudioTrack* Samsung_GTS5820::createAudioTrack(
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
    return NULL;
}
	
status_t Samsung_GTS5820::getOutputSamplingRate(int* samplingRate, int stream)
{
    //return AudioSystem_cm::getOutputSamplingRate(samplingRate, stream);
    return AudioSystem::getOutputSamplingRate(samplingRate, stream);
}
	
status_t Samsung_GTS5820::getOutputFrameCount(int* frameCount, int stream)
{
    //return AudioSystem_cm::getOutputFrameCount(frameCount, stream);
    return AudioSystem::getOutputFrameCount(frameCount, stream);
}
	
bool Samsung_GTS5820::canAudioHWDecode()
{
    return true;
}
	
bool Samsung_GTS5820::canAudioHWSBR()
{
    return true;
}

}
