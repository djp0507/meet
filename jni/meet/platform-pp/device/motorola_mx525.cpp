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
#define LOG_TAG "motorola_mx525"

#include "include-pp/OMXClient.h"
#include "include-pp/OMXCodec.h"
#include "include-pp/OMXCodec.h"
#include "include-pp/AudioSystem.h"
#include "include-pp/ppAudioTrack.h"
//#include "include-pp/OMXCodec_cm.h"
//#include "include-pp/AudioSystem_cm.h"
//#include "include-pp/ppAudioTrack_cm.h"

#include "device/motorola_mx525.h"

namespace android {

//extern const char *MODEL_NAME;
//extern const char *RELEASE_VERSION;
//extern const char *PRODUCT_VERSION;
extern GET_AUDIOTRACK_FUN CREATE_AUDIOTRACK_FUN;

class Motorola_MX525_IOMXRenderer : public IPPOMXRenderer
{
public:
	Motorola_MX525_IOMXRenderer(sp<IOMXRenderer>& target) 
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
	virtual ~Motorola_MX525_IOMXRenderer()
	{
	    LOGD("Destorying ~Motorola_MX525_IOMXRenderer");
		//IPCThreadState::self()->flushCommands();
	}

private:
	sp<IOMXRenderer> mTarget;
};

void Motorola_MX525::FlushCommands()
{
	IPCThreadState::self()->flushCommands();
}
Motorola_MX525::Motorola_MX525()
{
    mClient = new OMXClient();
    CHECK_EQ(mClient->connect(), OK);
}

Motorola_MX525::~Motorola_MX525()
{
    LOGD("Destorying ~Motorola_MX525");
	mClient->disconnect();
	delete mClient;
	mClient = NULL;
	//IPCThreadState::self()->flushCommands();
}

IPPOMXRenderer* Motorola_MX525::CreateOMXRenderer(
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
	return new Motorola_MX525_IOMXRenderer(target);
}

/*
sp<IOMXRenderer> Motorola_MX525::CreateOMXRenderer(
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

// static
sp<MediaSource> Motorola_MX525::CreateOMXCodec(
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
		//not supported. todo: need to find a way to render software decoded frame.
		if(codec == NULL)
		{
		    LOGE("trying software video codec");
			codec = OMXCodec_cm::Create(
	                omx,
	                meta,
	                createEncoder,
	                source,
	                matchComponentName,
	                1);
			if(codec != NULL)
			{
			    LOGE("Create software codec success");
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
	                OMXCodec::kPreferSoftwareCodecs);
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
	
	    if(codec == NULL)
	    {
		    LOGE("trying cyanogenmod audio hardware codec");
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
	
	if(codec == NULL)
	{
        LOGE("create %s codec failed", bVideo?"video":"audio");
	}
	return codec;
}



status_t Motorola_MX525::getOutputSamplingRate(int* samplingRate, int streamType)
{
	return AudioSystem::getOutputSamplingRate(samplingRate, streamType);
}

status_t Motorola_MX525::getOutputFrameCount(int* frameCount, int streamType)
{
	return AudioSystem::getOutputFrameCount(frameCount, streamType);
}

bool Motorola_MX525::canAudioHWDecode()
{
    return false;
}

bool Motorola_MX525::canAudioHWSBR()
{
    return false;
}
IPPAudioTrack* Motorola_MX525::createAudioTrack(
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


IPPAudioTrack* Motorola_MX525::createAudioTrack(
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

}
