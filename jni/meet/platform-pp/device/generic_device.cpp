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
#define LOG_TAG "generic_device"

#include "include-pp/OMXClient.h"
#include "include-pp/OMXCodec.h"
#include "include-pp/AudioSystem.h"
#include "include-pp/ppAudioTrack.h"
#include "include-pp/OMXCodec_cm.h"

#include "device/generic_device.h"

extern "C"
{
#include <dlfcn.h>
}
namespace android {

//extern const char *MODEL_NAME;
//extern const char *RELEASE_VERSION;
//extern const char *PRODUCT_VERSION;
extern GET_AUDIOTRACK_FUN CREATE_AUDIOTRACK_FUN;

class Generic_Device_IOMXRenderer : public IPPOMXRenderer
{
public:
	Generic_Device_IOMXRenderer(sp<IOMXRenderer>& target) 
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
	virtual ~Generic_Device_IOMXRenderer()
	{
		LOGD("Destorying Generic_Device_IOMXRenderer");
	}

private:
	sp<IOMXRenderer> mTarget;
};

void Generic_Device::FlushCommands()
{
	IPCThreadState::self()->flushCommands();
}

Generic_Device::Generic_Device()
{
    mClient = new OMXClient();
    CHECK_EQ(mClient->connect(), OK);
}

Generic_Device::~Generic_Device()
{
	mClient->disconnect();
	delete mClient;
	mClient = NULL;
}

IPPOMXRenderer* Generic_Device::CreateOMXRenderer(
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
	return new Generic_Device_IOMXRenderer(target);
}
	
sp<MediaSource> Generic_Device::CreateOMXCodec(
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

    /*
    if(MIUI_ROM)
	{
		LOGE("start load lib");
	    void *libHandle = dlopen("/data/data/com.pplive.androidphone/libstagefright_cm.so", RTLD_NOW);

	    if (libHandle) {

			LOGE("load lib success");
	        typedef sp<MediaSource> *(*createCodec)(
	                const IOMX* omx,
			        const MetaData* meta,
			        bool createEncoder,
			        const MediaSource* source,
			        const char *matchComponentName,
			        uint32_t flags);

	        createCodec funcCreateCodec =
	            (createCodec)dlsym(
	                    libHandle,
	                    "_Z11createCodecPN7android4IOMXEPNS_8MetaDataEbPNS_11MediaSourceEPKcj");
			
	        if (funcCreateCodec) {
				LOGE("find symbol success:%p", funcCreateCodec);
	            sp<MediaSource>* pCodec  = (*funcCreateCodec)(
		                omx.get(),
		                meta.get(),
		                createEncoder,
		                source.get(),
		                matchComponentName,
		                flags);
				codec = *pCodec;
				delete pCodec; //need to release sp<mediasource>* memory, codec will not be released.
	        } 
			else
			{
		        LOGE("Cannot find symbol");
	        }
	    }
		else
		{
		    LOGE("load lib failed");
		}
	}
*/
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
	    if(codec == NULL)
	    {
		    LOGD("trying cyanogenmod video hardware codec");
			codec = OMXCodec_cm::Create(
	                mClient->interface(),//omx,
	                meta,
	                createEncoder,
	                source,
	                matchComponentName,
	                0);
			if(codec != NULL)
			{
			    LOGD("Create cyanogenmod hardware codec success");
			}
	    }
		/*//not supported. todo: need to find a way to render software decoded frame.
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
		    LOGD("trying software audio codec");
			codec = OMXCodec_cm::Create(
	                mClient->interface(),//omx,
	                meta,
	                createEncoder,
	                source,
	                matchComponentName,
	                OMXCodec_cm::kPreferSoftwareCodecs);
			if(codec != NULL)
			{
			    LOGD("Create software codec success");
			}
		}
	
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
	
	    if(codec == NULL)
	    {
		    LOGD("trying cyanogenmod audio hardware codec");
			codec = OMXCodec_cm::Create(
	                mClient->interface(),//omx,
	                meta,
	                createEncoder,
	                source,
	                matchComponentName,
	                0);
			if(codec != NULL)
			{
			    LOGD("Create cyanogenmod hardware codec success");
			}
	    }
	}
	
	if(codec == NULL)
	{
        LOGE("create %s codec failed", bVideo?"video":"audio");
	}
	return codec;
}

IPPAudioTrack* Generic_Device::createAudioTrack(
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

IPPAudioTrack* Generic_Device::createAudioTrack(
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
	
status_t Generic_Device::getOutputSamplingRate(int* samplingRate, int stream)
{
    return AudioSystem::getOutputSamplingRate(samplingRate, stream);
}
	
status_t Generic_Device::getOutputFrameCount(int* frameCount, int stream)
{
    return AudioSystem::getOutputFrameCount(frameCount, stream);
}
	
bool Generic_Device::canAudioHWDecode()
{
    return false;
}
	
bool Generic_Device::canAudioHWSBR()
{
    return false;
}

}
