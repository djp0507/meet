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
#define LOG_TAG "xiaomi_mioneplus"

#include "device/xiaomi_mioneplus_IAudioFlinger.h"
#include "device/xiaomi_mioneplus_IAudioTrack.h"
//#include "device/xiaomi_mioneplus_AudioTrack.h"
//#include "device/xiaomi_mioneplus_ppAudioTrack.h"
#include "device/xiaomi_mioneplus_AudioSystem.h"
#include "include-pp/OMXClient.h"
#include "include-pp/OMXCodec.h"

#include "device/xiaomi_mioneplus.h"

namespace android {

//extern const char *MODEL_NAME;
//extern const char *RELEASE_VERSION;
//extern const char *PRODUCT_VERSION;
extern GET_AUDIOTRACK_FUN CREATE_AUDIOTRACK_FUN;


class Xiaomi_MIONEPlus_IOMXRenderer : public IPPOMXRenderer
{
public:
	Xiaomi_MIONEPlus_IOMXRenderer(sp<IOMXRenderer>& target) 
		: mTarget(target)
	{
	}
    virtual void render(MediaBuffer* buffer)
	{
        void *id;
        if (buffer->meta_data()->findPointer(kKeyBufferID, &id)) {
            mTarget->render((IOMX::buffer_id)id);
            //mTarget->render((void*)id);
        }
	}
	virtual int64_t getVideoRenderTimeout()
	{
		return 400000;
	}
	virtual ~Xiaomi_MIONEPlus_IOMXRenderer()
	{
		// The following hack is necessary to ensure that the OMX
        // component is completely released by the time we may try
        // to instantiate it again.

		//wp<IOMXRenderer> tmp = mTarget;
        //mTarget.clear();
        //while (tmp.promote() != NULL) {
		//    LOGE("waiting video render destorying");
        //    usleep(1000);
        //}
	    //IPCThreadState::self()->flushCommands();
	}

private:
	sp<IOMXRenderer> mTarget;
};


void Xiaomi_MIONEPlus::FlushCommands()
{
	IPCThreadState::self()->flushCommands();
}

Xiaomi_MIONEPlus::Xiaomi_MIONEPlus()
{
    mClient = new OMXClient();
    CHECK_EQ(mClient->connect(), OK);
}

Xiaomi_MIONEPlus::~Xiaomi_MIONEPlus()
{
	mClient->disconnect();
	delete mClient;
	mClient = NULL;
	//IPCThreadState::self()->flushCommands();
}

IPPOMXRenderer* Xiaomi_MIONEPlus::CreateOMXRenderer(
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
	return new Xiaomi_MIONEPlus_IOMXRenderer(target);
}

sp<MediaSource> Xiaomi_MIONEPlus::CreateOMXCodec(
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

	    if(codec == NULL)
	    {
		    LOGD("trying internal video hardware codec");
			codec = OMXCodec::Create(
	                mClient->interface(),//omx,
	                meta,
	                createEncoder,
	                source,
	                matchComponentName,
	                0);
			if(codec != NULL)
			{
			    LOGD("Create internal hardware codec success");
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
		
     /*	
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



status_t Xiaomi_MIONEPlus::getOutputSamplingRate(int* samplingRate, int streamType)
{
	// handle miui issue[MODEL_NAME: MI-ONE Plus]. IAudioFlinger.h interface has changes.
	//LOGE("force set miui sample rate");
	//*samplingRate=44100;
	//return NO_ERROR;
	return AudioSystem::getOutputSamplingRate(samplingRate, streamType);
}

status_t Xiaomi_MIONEPlus::getOutputFrameCount(int* frameCount, int streamType)
{
	// handle miui issue[MODEL_NAME: MI-ONE Plus]. IAudioFlinger.h interface has changes.
	//LOGE("force set miui frame count");
	//*frameCount=1200;
	//return NO_ERROR;
	return AudioSystem::getOutputFrameCount(frameCount, streamType);
}

bool Xiaomi_MIONEPlus::canAudioHWDecode()
{
    return false;
}

bool Xiaomi_MIONEPlus::canAudioHWSBR()
{
    return false;
}

IPPAudioTrack* Xiaomi_MIONEPlus::createAudioTrack(
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
    return new Xiaomi_MIONEPlus_PPAudioTrack(streamType,
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


IPPAudioTrack* Xiaomi_MIONEPlus::createAudioTrack(
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
    return new Xiaomi_MIONEPlus_PPAudioTrack(streamType,
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
