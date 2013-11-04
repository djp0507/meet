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

#include "platform-pp/a14/IPPAudioTrack.h"

#include "include-pp/a14/frameworks/base/include/media/stagefright/OMXClient.h"
#include "include-pp/a14/frameworks/base/include/media/stagefright/OMXCodec.h"
#include "include-pp/a14/frameworks/base/include/media/AudioSystem.h"
#include "include-pp/a14/frameworks/base/include/media/AudioTrack.h"

#include "samsung_gti9000.h"

namespace android {

class Samsung_GTi9000_PPAudioTrack : public IPPAudioTrack {
private:
	AudioTrack* mTrack;

public:
	typedef void (*callback_t)(int event, void* user, void *info);

	Samsung_GTi9000_PPAudioTrack() 
	{
		mTrack = NULL;
	}

	Samsung_GTi9000_PPAudioTrack(
		int streamType,
		uint32_t sampleRate  = 0,
		int format           = 0,
		int channels         = 0,
		int frameCount       = 0,
		uint32_t flags       = 0,
		callback_t cbf       = 0,
		void* user           = 0,
		int notificationFrames = 0,
		int sessionId = 0)
	{
		mTrack = new AudioTrack(
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

	Samsung_GTi9000_PPAudioTrack( 
        int streamType,
        uint32_t sampleRate = 0,
        int format          = 0,
        int channels        = 0,
        const sp<IMemory>& sharedBuffer = 0,
        uint32_t flags      = 0,
        callback_t cbf      = 0,
        void* user          = 0,
        int notificationFrames = 0,
        int sessionId = 0)
    {
	    mTrack = new AudioTrack(
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

    ~Samsung_GTi9000_PPAudioTrack() {}
    
	uint32_t frameCount() const 
	{
		return mTrack->frameCount();
	}
	
	int channelCount() const
	{
		return mTrack->channelCount();
	}
	
	int frameSize() const
	{
		return mTrack->frameSize();
	}
	
	status_t getPosition(uint32_t *position)
	{
		return mTrack->getPosition(position);
	}
	
	status_t setVolume(float left, float right)
	{
		return mTrack->setVolume(left, right);
	}
	
	void start()
	{
		mTrack->start();
	}
	
	uint32_t latency() const 
	{
		return mTrack->latency();
	}
	
	ssize_t write(const void* buffer, size_t size)
	{
		return mTrack->write(buffer, size);
	}
	
	void stop()
	{
		mTrack->stop();
	}
	
	void flush()
	{
		mTrack->flush();
	}
	
	void pause()
	{
		mTrack->pause();
	}
	
	status_t initCheck() const
	{
		return mTrack->initCheck();
	}
	
};


class Samsung_GTi9000_IOMXRenderer : public IPPOMXRenderer
{
public:
	Samsung_GTi9000_IOMXRenderer(const sp<Surface> &surface,
                                            int32_t rotationDegrees) 
		: mNativeWindow(surface)
	{
        applyRotation(rotationDegrees);
	}
    virtual void render(MediaBuffer* buffer)
	{
		int64_t timeUs;
		buffer->meta_data()->findInt64(kKeyTime, &timeUs);
		native_window_set_buffers_timestamp(mNativeWindow.get(), timeUs * 1000);
		status_t err = mNativeWindow->queueBuffer(
		mNativeWindow.get(), buffer->graphicBuffer().get());
		if (err != 0) {
			LOGE("queueBuffer failed with error %s (%d)", strerror(-err), -err);
			return;
		}

		sp<MetaData> metaData = buffer->meta_data();
		metaData->setInt32(kKeyRendered, 1);

	}
	virtual ~Samsung_GTi9000_IOMXRenderer()
	{
        //IPCThreadState::self()->flushCommands();
	}

private:
	sp<ANativeWindow> mNativeWindow;

    void applyRotation(int32_t rotationDegrees) {
        uint32_t transform;
        switch (rotationDegrees) {
            case 0: transform = 0; break;
            case 90: transform = HAL_TRANSFORM_ROT_90; break;
            case 180: transform = HAL_TRANSFORM_ROT_180; break;
            case 270: transform = HAL_TRANSFORM_ROT_270; break;
            default: transform = 0; break;
        }

        if (transform) {
            if(native_window_set_buffers_transform(mNativeWindow.get(), transform) !=0)
            {
                LOGE("native_window_set_buffers_transform failed");
            }
        }
    }
};

void Samsung_GTi9000::FlushCommands()
{
    LOGI("FlushCommands");
	IPCThreadState::self()->flushCommands();
}

Samsung_GTi9000::Samsung_GTi9000()
{
    mClient = new OMXClient();
    if (mClient->connect() != OK) {
		LOGE("OMX connect failed.");
    } else {
		LOGI("OMX connect success.");
    }
}

Samsung_GTi9000::~Samsung_GTi9000()
{
	mClient->disconnect();
	delete mClient;
	mClient = NULL;
	//IPCThreadState::self()->flushCommands();
}

IPPOMXRenderer* Samsung_GTi9000::CreateOMXRenderer(
            const sp<Surface> &surface,
            int32_t rotationDegrees)
{	

	return new Samsung_GTi9000_IOMXRenderer(surface, rotationDegrees);
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
            const sp<Surface> &surface,
            const char *matchComponentName,
            uint32_t flags)
{
	const char *mime;
	meta->findCString(kKeyMIMEType, &mime);
	bool bVideo = (!strncasecmp(mime, "video/", 6));
	
	sp<MediaSource> codec = NULL;

	LOGD("surface: %p", surface.get());

/*	sp<ISurfaceTexture> new_st;
	new_st = surface->getSurfaceTexture();

	const sp<ANativeWindow> nativeWindow = new SurfaceTextureClient(new_st);
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
	                flags, surface);
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
    return new Samsung_GTi9000_PPAudioTrack(
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
