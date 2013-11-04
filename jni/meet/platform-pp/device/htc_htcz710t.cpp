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
#define LOG_TAG "htc_htcz710t"

#include "include-pp/OMXClient.h"
#include "include-pp/OMXCodec.h"
#include "include-pp/AudioSystem.h"
#include "include-pp/OMXCodec.h"
#include "include-pp/ppAudioTrack.h"
//#include "include-pp/AudioSystem_cm.h"
//#include "include-pp/OMXCodec_cm.h"
//#include "include-pp/ppAudioTrack_cm.h"

#include "device/htc_htcz710t.h"

namespace android {

//extern const char *MODEL_NAME;
//extern const char *RELEASE_VERSION;
//extern const char *PRODUCT_VERSION;
extern GET_AUDIOTRACK_FUN CREATE_AUDIOTRACK_FUN;

class HTC_HTCZ710T_IOMXRenderer : public IPPOMXRenderer
{
public:
	HTC_HTCZ710T_IOMXRenderer(sp<IOMXRenderer>& target) 
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
	virtual ~HTC_HTCZ710T_IOMXRenderer()
	{
        //IPCThreadState::self()->flushCommands();
	}

private:
	sp<IOMXRenderer> mTarget;
};

void HTC_HTCZ710T::FlushCommands()
{
	IPCThreadState::self()->flushCommands();
}

HTC_HTCZ710T::HTC_HTCZ710T()
{
    LOGD("Constructor");
    mClient = new OMXClient();
    LOGD("Constructor 1");
    CHECK_EQ(mClient->connect(), OK);
    LOGD("Constructor 2");
}

HTC_HTCZ710T::~HTC_HTCZ710T()
{
	mClient->disconnect();
	delete mClient;
	mClient = NULL;
	//IPCThreadState::self()->flushCommands();
}

IPPOMXRenderer* HTC_HTCZ710T::CreateOMXRenderer(
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
	return new HTC_HTCZ710T_IOMXRenderer(target);
}

/*
sp<IOMXRenderer> HTC_HTCZ710T::CreateOMXRenderer(
            //const sp<IOMX> &omx,
            const sp<ISurface> &surface,
            const char *componentName,
            OMX_COLOR_FORMATTYPE colorFormat,
            size_t encodedWidth, size_t encodedHeight,
            size_t displayWidth, size_t displayHeight)
{	
	return (mClient.interface())->createRenderer(surface, 
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
sp<MediaSource> HTC_HTCZ710T::CreateOMXCodec(
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
		    LOGD("trying software audio codec");
			codec = OMXCodec::Create(
	                mClient->interface(),//omx,
	                meta,
	                createEncoder,
	                source,
	                matchComponentName,
	                OMXCodec::kPreferSoftwareCodecs);
			if(codec != NULL)
			{
			    LOGD("Create software codec success");
			}
		}
	/*
	    if(codec == NULL)
		{
		    LOGE("trying internal audio codec");
			codec = OMXCodec::Create(
	                mClient->interface(),//omx,
	                meta,
	                createEncoder,
	                source,
	                matchComponentName,
	                0);
			if(codec != NULL)
			{
			    LOGE("Create internal codec success");
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


status_t HTC_HTCZ710T::getOutputSamplingRate(int* samplingRate, int streamType)
{
    LOGV("getOutputSamplingRate 1");
	status_t ret = AudioSystem::getOutputSamplingRate(samplingRate, streamType);
    LOGV("getOutputSamplingRate 2:%d", *samplingRate);
	return ret;
}

status_t HTC_HTCZ710T::getOutputFrameCount(int* frameCount, int streamType)
{
    LOGV("getOutputFrameCount 1");
	return AudioSystem::getOutputFrameCount(frameCount, streamType);
    LOGV("getOutputFrameCount 2:%d", *frameCount);
}

bool HTC_HTCZ710T::canAudioHWDecode()
{
    return true;
}

bool HTC_HTCZ710T::canAudioHWSBR()
{
    return true;
}
IPPAudioTrack* HTC_HTCZ710T::createAudioTrack(
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


IPPAudioTrack* HTC_HTCZ710T::createAudioTrack(
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
