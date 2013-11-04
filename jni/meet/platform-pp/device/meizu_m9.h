/*
 * Copyright (C) 2009 The Android Open Source Project
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

#ifndef PP_MEIZU_M9_H_

#define PP_MEIZU_M9_H_

#include "platform-pp/PPPlatForm.h"

namespace android {

class OMXClient;
class Meizu_M9 : public IDevice
{
public:
	Meizu_M9();
	~Meizu_M9();
	
	//virtual sp<IOMX> CreateOMX();
	
	virtual IPPOMXRenderer* CreateOMXRenderer(
            //const sp<IOMX> &omx,
            const sp<Surface> &surface,
            const char *componentName,
            OMX_COLOR_FORMATTYPE colorFormat,
            size_t encodedWidth, size_t encodedHeight,
            size_t displayWidth, size_t displayHeight);
	
	virtual sp<MediaSource> CreateOMXCodec(
            //const sp<IOMX> &omx,
            const sp<MetaData> &meta, bool createEncoder,
            const sp<MediaSource> &source,
            const char *matchComponentName = NULL,
            uint32_t flags = 0);

    virtual IPPAudioTrack* createAudioTrack( int streamType,
            uint32_t sampleRate  = 0,
            int format           = 0,
            int channels         = 0,
            int frameCount       = 0,
            uint32_t flags       = 0,
            callback_t cbf       = 0,
            void* user           = 0,
            int notificationFrames = 0,
            int sessionId = 0);

    virtual IPPAudioTrack* createAudioTrack( int streamType,
            uint32_t sampleRate = 0,
            int format          = 0,
            int channels        = 0,
            const sp<IMemory>& sharedBuffer = 0,
            uint32_t flags      = 0,
            callback_t cbf      = 0,
            void* user          = 0,
            int notificationFrames = 0,
            int sessionId = 0);
	
    virtual status_t getOutputSamplingRate(int* samplingRate, int stream = AudioSystemWrapper::DEFAULT);
    virtual status_t getOutputFrameCount(int* frameCount, int stream = AudioSystemWrapper::DEFAULT);
	virtual bool canAudioHWDecode();
	virtual bool canAudioHWSBR();
	virtual void FlushCommands();
private:

	OMXClient* mClient;
};

}

#endif  // PP_MEIZU_M9_H_