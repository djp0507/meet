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
#define LOG_TAG "HTC_HTCIncredibleS2"

#include "include-pp/OMXClient.h"
#include "include-pp/OMXCodec.h"
#include "include-pp/AudioSystem.h"
#include "include-pp/ppAudioTrack.h"
#include "include-pp/OMXCodec.h"
//#include "include-pp/OMXCodec_cm.h"

#include "device/htc_htcincredibles2.h"

namespace android {

//extern const char *MODEL_NAME;
//extern const char *RELEASE_VERSION;
//extern const char *PRODUCT_VERSION;
extern GET_AUDIOTRACK_FUN CREATE_AUDIOTRACK_FUN;

class HTC_HTCIncredibleS2_IOMXRenderer : public IPPOMXRenderer
{
public:
	HTC_HTCIncredibleS2_IOMXRenderer(sp<IOMXRenderer>& target) 
		: mTarget(target)
	{
	}
	
	virtual int64_t getVideoRenderTimeout()
	{
		return 60000;
	}
    virtual void render(MediaBuffer* buffer)
	{
		/*
		LOGE("buffer->data():%p, buffer->range_length():%d", buffer->data(), buffer->range_length());
		
	    static int count=0;
		memset(buffer->data()+buffer->range_offset(),
			0,
			buffer->range_length());
		memset(buffer->data()+buffer->range_offset()+((count++)%25)*(buffer->range_length()/25),
			255,
			buffer->range_length()/25);
		*/
			
        void *id;
        if (buffer->meta_data()->findPointer(kKeyBufferID, &id)) {
			//LOGE("buffer_Id:%p", id);
            mTarget->render((IOMX::buffer_id)id);
            //mTarget->render((void*)id);
        }
	}
	virtual ~HTC_HTCIncredibleS2_IOMXRenderer()
	{
        //IPCThreadState::self()->flushCommands();
	}

private:
	sp<IOMXRenderer> mTarget;
};

void HTC_HTCIncredibleS2::FlushCommands()
{
	IPCThreadState::self()->flushCommands();
}

HTC_HTCIncredibleS2::HTC_HTCIncredibleS2()
{
    mClient = new OMXClient();
    CHECK_EQ(mClient->connect(), OK);
}

HTC_HTCIncredibleS2::~HTC_HTCIncredibleS2()
{
	mClient->disconnect();
	delete mClient;
	mClient = NULL;
	//IPCThreadState::self()->flushCommands();
}

IPPOMXRenderer* HTC_HTCIncredibleS2::CreateOMXRenderer(
            //const sp<IOMX> &omx,
            const sp<Surface> &surface,
            const char *componentName,
            OMX_COLOR_FORMATTYPE colorFormat,
            size_t encodedWidth, size_t encodedHeight,
            size_t displayWidth, size_t displayHeight)
{	
    LOGD("Start adjust encodedWidth: %d, encodedHeight: %d", encodedWidth, encodedHeight);
	//adjust width
	if(encodedWidth % 4 != 0)
	{
		LOGD("Adjust encodedWidth + 2");
		encodedWidth += 2;//to fix picture corrupt issue.
	}
	if(encodedWidth % 8 != 0)
	{
		LOGD("Adjust encodedWidth + 4");
		encodedWidth += 4;//to fix picture corrupt issue.
	}
	if(encodedWidth % 16 != 0)
	{
		LOGD("Adjust encodedWidth + 8");
		encodedWidth += 8;//to fix picture corrupt issue.
	}
	
	//adjust height
	if(encodedHeight % 4 != 0)
	{
		LOGD("Adjust encodedHeight +2");
		encodedHeight += 2;//to fix picture corrupt issue.
	}
	if(encodedHeight % 8 != 0)
	{
		LOGD("Adjust encodedHeight +4");
		encodedHeight+=4;//to fix picture corrupt issue.
	}
	if(encodedHeight % 16 != 0)
	{
		LOGD("Adjust encodedHeight +8");
		encodedHeight += 8;//to fix picture corrupt issue.
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
	return new HTC_HTCIncredibleS2_IOMXRenderer(target);
}

// static
sp<MediaSource> HTC_HTCIncredibleS2::CreateOMXCodec(
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
	                0);
			if(codec != NULL)
			{
			    LOGD("Create internal video codec success");
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



status_t HTC_HTCIncredibleS2::getOutputSamplingRate(int* samplingRate, int streamType)
{
	return AudioSystem::getOutputSamplingRate(samplingRate, streamType);
}

status_t HTC_HTCIncredibleS2::getOutputFrameCount(int* frameCount, int streamType)
{
	return AudioSystem::getOutputFrameCount(frameCount, streamType);
}

bool HTC_HTCIncredibleS2::canAudioHWDecode()
{
    return true;
}

bool HTC_HTCIncredibleS2::canAudioHWSBR()
{
    return true;
}
IPPAudioTrack* HTC_HTCIncredibleS2::createAudioTrack(
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


IPPAudioTrack* HTC_HTCIncredibleS2::createAudioTrack(
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
