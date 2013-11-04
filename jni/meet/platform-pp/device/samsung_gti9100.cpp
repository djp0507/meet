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
#define OMAP_ENHANCEMENT
#define TARGET_OMAP4
#define NPA_BUFFERS
#define ARM_4K_PAGE_SIZE 4096

#include "device/samsung_gti9100_IOMX.h"
#include "device/samsung_gti9100_MediaSource.h"
#include "device/samsung_gti9100_IAudioFlinger.h"
#include "device/samsung_gti9100_OMXClient.h"
#include "device/samsung_gti9100_OMXCodec.h"
#include "device/samsung_gti9100_AudioSystem.h"
#include "device/samsung_gti9100_ppAudioTrack.h"

#include "device/samsung_gti9100.h"

#include "include-pp/MediaDebug.h"
#include "include-pp/utils/Vector.h"

#include "../../libppplayer/TIVideoConfigParser.cpp"

#define LOG_TAG "samsung_gti9100"

namespace android {

//extern const char *MODEL_NAME;
//extern const char *RELEASE_VERSION;
//extern const char *PRODUCT_VERSION;
extern GET_AUDIOTRACK_FUN CREATE_AUDIOTRACK_FUN;


class Samsung_GTi9100_IOMXRenderer : public IPPOMXRenderer
{
public:
	Samsung_GTi9100_IOMXRenderer(sp<IOMXRenderer>& target);

    virtual void render(MediaBuffer* buffer)
	{
		mBuffersWithRenderer.push(buffer);
		
        void *id;
        if (buffer->meta_data()->findPointer(kKeyBufferID, &id)) {
			LOGD("buffer_Id: %p", id);
            mTarget->render((IOMX::buffer_id)id);
        }

		if ((!mBufferReleaseCallbackSet) && (mBuffersWithRenderer.size()))
		{
	        mBuffersWithRenderer[0]->release();
	        mBuffersWithRenderer.pop();
	    }
	}
	
	virtual bool ownBuffer()
	{
	    return true;
	}
	
	void releaseRenderedBuffer(const sp<IMemory>& mem)
	{
	    LOGD("releaseRenderedBuffer");
	    bool buffer_released = false;
	    unsigned int i = 0;

	    for(i = 0; i < mBuffersWithRenderer.size(); i++){
	        if (mBuffersWithRenderer[i]->data() == mem->pointer()){
	            mBuffersWithRenderer[i]->release();
	            mBuffersWithRenderer.removeAt(i);
	            buffer_released = true;
	            break;
	        }
	    }

	    if (buffer_released == false)
	        LOGE("Something wrong... Overlay returned wrong buffer address(%p). This message is harmless if you just did a seek.", mem->pointer());
	}
	
	~Samsung_GTi9100_IOMXRenderer()
	{
		if (mBuffersWithRenderer.size()) 
		{
            unsigned int i;
            unsigned int sz = mBuffersWithRenderer.size();

            for(i = 0; i < sz; i++){
                mBuffersWithRenderer[i]->release();
            }

            for(i = 0; i < sz; i++){
                mBuffersWithRenderer.pop();
            }
        }
		//IPCThreadState::self()->flushCommands();
	}
	
private:
	sp<IOMXRenderer> mTarget;
    Vector<MediaBuffer*> mBuffersWithRenderer;
	bool mBufferReleaseCallbackSet;
	
};

static void releaseRenderedBufferCallback(const sp<IMemory>& mem, void *cookie){
    Samsung_GTi9100_IOMXRenderer *ap = static_cast<Samsung_GTi9100_IOMXRenderer *>(cookie);
    ap->releaseRenderedBuffer(mem);
}

Samsung_GTi9100_IOMXRenderer::Samsung_GTi9100_IOMXRenderer(sp<IOMXRenderer>& target) 
		: mTarget(target)
{
	mBufferReleaseCallbackSet = target->setCallback(releaseRenderedBufferCallback, this);
	target->setCallback(releaseRenderedBufferCallback, this);
	LOGD("mBufferReleaseCallbackSet:%d", mBufferReleaseCallbackSet);
    target->requestRendererClone(false);
	LOGD("Finish constructor");
}

//////////////////////////////////////////////////////////////////////////////////////////

void Samsung_GTi9100::FlushCommands()
{
	IPCThreadState::self()->flushCommands();
}

Samsung_GTi9100::Samsung_GTi9100()
{
    LOGD("Destorying Samsung_GTi9100");
    mClient = new OMXClient();
    CHECK_EQ(mClient->connect(), OK);
	mRender = NULL;
}

Samsung_GTi9100::~Samsung_GTi9100()
{
	mClient->disconnect();
	delete mClient;
	mClient = NULL;
	//IPCThreadState::self()->flushCommands();
}

IPPOMXRenderer* Samsung_GTi9100::CreateOMXRenderer(
            //const sp<IOMX> &omx,
            const sp<Surface> &surface,
            const char *componentName,
            OMX_COLOR_FORMATTYPE colorFormat,
            size_t encodedWidth_ignore, size_t encodedHeight_ignore,
            size_t displayWidth_ignore, size_t displayHeight_ignore)
{
    //if(mRender != NULL) return mRender;

	//update width/height here.
	sp<MetaData> meta = mVideoCodec->getFormat();
	
	int32_t encodedWidth, encodedHeight;
    int32_t displayWidth, displayHeight;
    
    CHECK(meta->findInt32(kKeyWidth, &encodedWidth));
    CHECK(meta->findInt32(kKeyHeight, &encodedHeight));

    CHECK(meta->findInt32(kKeyWidth, &displayWidth));
    CHECK(meta->findInt32(kKeyHeight, &displayHeight));
 
    if(!(meta->findInt32(kKeyPaddedWidth, &encodedWidth))) {
       CHECK(meta->findInt32(kKeyWidth, &encodedWidth));
    }
    if(!(meta->findInt32(kKeyPaddedHeight, &encodedHeight))) {
       CHECK(meta->findInt32(kKeyHeight, &encodedHeight));
    }
    
    LOGD("decodedWidth/decodedHeight: %dx%d", encodedWidth, encodedHeight);
    LOGD("mVideoWidth/mVideoHeight: %dx%d", displayWidth, displayHeight);
	
    int32_t outputBufferCnt = mVideoCodec->getNumofOutputBuffers();
    
    LOGD("mVideoCodec->getNumofOutputBuffers(): %d", outputBufferCnt);
    
	sp<IOMX> pOMX = mClient->interface();
	LOGD("pOMX: %p", pOMX.get());
    const sp<ISurface> isurface = surface->getISurface();
    sp<IOMXRenderer> target = pOMX->createRenderer(
								isurface, 
		                        componentName,
		                        colorFormat,
		                        encodedWidth, 
		                        encodedHeight,
		                        displayWidth, 
		                        displayHeight,
		                        0,
		                        0,
		                        outputBufferCnt);
	LOGD("target x: %p", target.get());
	if(target == NULL) return NULL;
	
    if (target != NULL) {
		LOGD("1.1 mExtBufferAddresses.capacity():%u", (target->getBuffers()).capacity());
		LOGD("1.1 mExtBufferAddresses.size():%u", (target->getBuffers()).size());
        // Share overlay buffers with video decoder.
        //bool portReconfig = false;
        //if(mRender != NULL) portReconfig = true;	
        //mVideoCodec->setBuffers(target->getBuffers(), true);//portReconfig);

		mRender = new Samsung_GTi9100_IOMXRenderer(target);
		LOGD("new Samsung_GTi9100_IOMXRenderer");
		return mRender;
    }
		LOGD("2");
	return NULL;
}

sp<MediaSource> Samsung_GTi9100::CreateOMXCodec(
        //const sp<IOMX> &omx,
        const sp<MetaData> &meta, bool createEncoder,
        const sp<MediaSource> &source,
        const char *matchComponentName,
        uint32_t flags)
{
	const char *mime;
	CHECK(meta->findCString(kKeyMIMEType, &mime));

	int32_t profile = -1;

	meta->findInt32(kKeyVideoProfile, & profile);

	LOGI("Profile: %d", profile);
	
	bool bVideo = (!strncasecmp(mime, "video/", 6));
	
	//sp<MediaSource> codec=NULL;

    if(bVideo) //video
    {		
	    if(mVideoCodec == NULL)
	    {
		    LOGD("trying internal video hardware codec");
			//codec = Samsung_GTi9100_OMXCodec::Create(

			LOGI("Begin updateMetaData()");
			updateMetaData(meta);
			LOGI("End updateMetaData()");
			
			mVideoCodec = OMXCodec::Create(
	                mClient->interface(),//omx,
	                meta,
	                createEncoder,
	                source,
	                matchComponentName,
	                0);
	    }
		LOGD("Create video codec %s", (mVideoCodec!=NULL)?"success":"failed");		
		return mVideoCodec;
    }
	else //audio
	{
	    if(mAudioCodec == NULL)
	    {
		    LOGD("trying internal audio hardware codec");
			//codec = Samsung_GTi9100_OMXCodec::Create(
			mAudioCodec = OMXCodec::Create(
	                mClient->interface(),//omx,
	                meta,
	                createEncoder,
	                source,
	                matchComponentName,
	                0);
	    }
		
		if(mAudioCodec == NULL)
		{
		    LOGD("trying software audio codec");
			//codec = Samsung_GTi9100_OMXCodec::Create(
			mAudioCodec = OMXCodec::Create(
	                mClient->interface(),//omx,
	                meta,
	                createEncoder,
	                source,
	                matchComponentName,
	                OMXCodec::kPreferSoftwareCodecs);
		}

		LOGD("Create audio codec %s", (mAudioCodec!=NULL)?"success":"failed");
		return mAudioCodec;
	}
}

status_t Samsung_GTi9100::getOutputSamplingRate(int* samplingRate, int streamType)
{
	return AudioSystem::getOutputSamplingRate(samplingRate, streamType);
}

status_t Samsung_GTi9100::getOutputFrameCount(int* frameCount, int streamType)
{
	return AudioSystem::getOutputFrameCount(frameCount, streamType);
}

bool Samsung_GTi9100::canAudioHWDecode()
{
    return false;
}

bool Samsung_GTi9100::canAudioHWSBR()
{
    return false;
}
IPPAudioTrack* Samsung_GTi9100::createAudioTrack(
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
    LOGD("createAudioTrack");
    return new Samsung_GTi9100_PPAudioTrack(streamType,
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


IPPAudioTrack* Samsung_GTi9100::createAudioTrack(
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
    return new Samsung_GTi9100_PPAudioTrack(streamType,
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

}
