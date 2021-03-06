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
#define LOG_TAG "samsung_gti9000"

#include "include-pp/OMXClient.h"
#include "include-pp/OMXCodec.h"
//#include "include-pp/AudioSystem_cm.h"
#include "include-pp/AudioSystem.h"
//#include "include-pp/OMXCodec_cm.h"
#include "include-pp/OMXCodec.h"
//#include "include-pp/ppAudioTrack_cm.h"
#include "include-pp/ppAudioTrack.h"

#include "device/samsung_gti9000.h"

namespace android {

//extern const char *MODEL_NAME;
//extern const char *RELEASE_VERSION;
//extern const char *PRODUCT_VERSION;
extern GET_AUDIOTRACK_FUN CREATE_AUDIOTRACK_FUN;


class Samsung_GTi9000_IOMXRenderer : public IPPOMXRenderer
{
public:
	Samsung_GTi9000_IOMXRenderer(sp<IOMXRenderer>& target) 
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
	virtual ~Samsung_GTi9000_IOMXRenderer()
	{
        //IPCThreadState::self()->flushCommands();
	}

private:
	sp<IOMXRenderer> mTarget;
};

void Samsung_GTi9000::FlushCommands()
{
    LOGI("FlushCommands");
	IPCThreadState::self()->flushCommands();
}

Samsung_GTi9000::Samsung_GTi9000()
{
    mClient = new OMXClient();
    CHECK_EQ(mClient->connect(), OK);
}

Samsung_GTi9000::~Samsung_GTi9000()
{
	mClient->disconnect();
	delete mClient;
	mClient = NULL;
	//IPCThreadState::self()->flushCommands();
}

IPPOMXRenderer* Samsung_GTi9000::CreateOMXRenderer(
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
	return new Samsung_GTi9000_IOMXRenderer(target);
}

/*
sp<IOMXRenderer> Samsung_GTi9000::CreateOMXRenderer(
            //const sp<IOMX> &omx,
            const sp<ISurface> &surface,
            const char *componentName,
            OMX_COLOR_FORMATTYPE colorFormat,
            size_t encodedWidth, size_t encodedHeight,
            size_t displayWidth, size_t displayHeight)
{
	return (mClient->interface())->createRenderer(surface, 
                        componentName,
                        colorFormat,
                        encodedWidth, 
                        encodedHeight,
                        displayWidth, 
                        displayHeight,
                        0);
}
*/
	
sp<MediaSource> Samsung_GTi9000::CreateOMXCodec(
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
		/*
		if(codec == NULL)
		{
			LOGE("trying cyanogenmod video hardware codec");
			codec = OMXCodec_cm::Create(
			mClient->interface(),//omx,
			meta,
			createEncoder,
			source,
			matchComponentName,
			0);
			if(codec != NULL)
			{
			    LOGE("Create cyanogenmod hardware codec success");
			}
		}
	    */
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
		/*
		if(codec == NULL)
		{
			LOGE("trying software audio codec");
			codec = OMXCodec_cm::Create(
			mClient->interface(),//omx,
			meta,
			createEncoder,
			source,
			matchComponentName,
			OMXCodec_cm::kPreferSoftwareCodecs);
			if(codec != NULL)
			{
			    LOGE("Create software codec success");
			}
		}
		*/
	}
	
	if(codec == NULL)
	{
        LOGE("create %s codec failed", bVideo?"video":"audio");
	}
	return codec;
}

IPPAudioTrack* Samsung_GTi9000::createAudioTrack(
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
					sessionId)
	*/;
	return CREATE_AUDIOTRACK_FUN ? 
			CREATE_AUDIOTRACK_FUN(
					streamType,
					sampleRate,
			        format,
			        channels,
			        frameCount,
			        flags,
			        cbf,
			        user,
			        notificationFrames,
			        sessionId) : NULL;
			        
}

IPPAudioTrack* Samsung_GTi9000::createAudioTrack(
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
	
status_t Samsung_GTi9000::getOutputSamplingRate(int* samplingRate, int stream)
{
    //return AudioSystem_cm::getOutputSamplingRate(samplingRate, stream);
    return AudioSystem::getOutputSamplingRate(samplingRate, stream);
}
	
status_t Samsung_GTi9000::getOutputFrameCount(int* frameCount, int stream)
{
    //return AudioSystem_cm::getOutputFrameCount(frameCount, stream);
    return AudioSystem::getOutputFrameCount(frameCount, stream);
}
	
bool Samsung_GTi9000::canAudioHWDecode()
{
    return true;
}
	
bool Samsung_GTi9000::canAudioHWSBR()
{
    return true;
}

}
