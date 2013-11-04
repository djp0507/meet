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
#define LOG_TAG "PPPlatForm"
#include "PPPlatForm.h"

#include "include-pp/MediaSource.h"
#include "include-pp/utils/Log.h"

#include "include-pp/PPBox_Util.h"
#include "platform/platforminfo.h"

extern "C" {
#include <dlfcn.h>
}

namespace android {

//extern const char *MODEL_NAME;
//extern const char *BOARD_NAME;
//extern const char *CHIP_NAME;
//extern const char *MANUFACTURE_NAME;
//extern const char *APP_PATH;
//extern const char *RELEASE_VERSION;

extern PlatformInfo* gPlatformInfo;

static IDevice* gDevice = NULL;

typedef IDevice* (*GET_DEVICE_FUN)(
			GET_AUDIOTRACK_FUN,
			PlatformInfo*);

static IDevice* getDevice()
{
    if (gDevice == NULL)
    {
		//LOGI("MODEL_NAME: %s", MODEL_NAME);

		GET_AUDIOTRACK_FUN createAudioTrackFun = NULL;
		
		if(!strncmp(gPlatformInfo->release_version, "2.2", strlen("2.2")))
		{
			const char libOSName[] = "lib/libos_a8.so";
			char* dllPath = (char*)malloc(strlen(gPlatformInfo->app_path) + strlen(libOSName) + 1);
			strcpy(dllPath, gPlatformInfo->app_path);
			strcpy(dllPath + strlen(gPlatformInfo->app_path), libOSName);
			
	        LOGI("Lib path: %s", dllPath);
	        
	    	void* deviceHandle = dlopen(dllPath, RTLD_GLOBAL | RTLD_NOW);

			if(deviceHandle != NULL)
			{
				LOGI("Load lib %s success", dllPath);
		    	createAudioTrackFun = (GET_AUDIOTRACK_FUN)dlsym(deviceHandle, "createAudioTrack");
			}
			else
			{
				LOGE("Load lib %s error: %s", dllPath, dlerror());
			}
			free(dllPath);
		}
		else if(!strncmp(gPlatformInfo->release_version, "2.3", strlen("2.3")))
		{
			const char libOSName[] = "lib/libos_a9.so";
			char* dllPath = (char*)malloc(strlen(gPlatformInfo->app_path) + strlen(libOSName) + 1);
			strcpy(dllPath, gPlatformInfo->app_path);
			strcpy(dllPath + strlen(gPlatformInfo->app_path), libOSName);
			
	        LOGI("Lib path: %s", dllPath);
	        
	    	void* deviceHandle = dlopen(dllPath, RTLD_GLOBAL | RTLD_NOW);

			if(deviceHandle != NULL)
			{
				LOGI("Load lib %s success", dllPath);
				if(!strncmp(gPlatformInfo->model_name, "MI-ONE", strlen("MI-ONE")))
				{
			    	createAudioTrackFun = (GET_AUDIOTRACK_FUN)dlsym(deviceHandle, "createAudioTrack_Xiaomi");
					if(createAudioTrackFun != NULL)
					{
						LOGI("dlsym createAudioTrack_Xiaomi success");
					}
				}
				else
				{
			    	createAudioTrackFun = (GET_AUDIOTRACK_FUN)dlsym(deviceHandle, "createAudioTrack");
				}
			}
			else
			{
				LOGE("Load lib %s error: %s", dllPath, dlerror());
			}
			free(dllPath);
		}
		else
		{
			return NULL;
		}

		if(createAudioTrackFun != NULL)
		{
			LOGI("Load sym createAudioTrackFun success");
		}
		else
		{
			LOGE("Load sym createAudioTrackFun error: %s", dlerror());
			return NULL;
		}
		
		const char libName[] = "lib/libdevice_internal.so";
		char* dllPath = (char*)malloc(strlen(gPlatformInfo->app_path) + strlen(libName) + 1);
		strcpy(dllPath, gPlatformInfo->app_path);
		strcpy(dllPath + strlen(gPlatformInfo->app_path), libName);
		
        LOGI("Lib path: %s", dllPath);
        
    	void* deviceHandle = dlopen(dllPath, RTLD_GLOBAL | RTLD_NOW);

        //try default devices.
		if(deviceHandle != NULL)
		{
			LOGI("Load lib %s success", dllPath);
	    	GET_DEVICE_FUN createDeviceFun = (GET_DEVICE_FUN)dlsym(deviceHandle, "createDevice");
			if(createDeviceFun != NULL)
			{
				LOGI("Load sym createDevice success");
		    	gDevice = createDeviceFun(createAudioTrackFun, gPlatformInfo);
			}
			else
			{
				LOGE("Load sym createDevice error: %s", dlerror());
			}
		}
		else
		{
			LOGE("Load lib %s error: %s", dllPath, dlerror());
		}
		free(dllPath);

        //try cm devices.
		if(gDevice == NULL)
		{
			const char libCMName[] = "lib/libdevice_cm.so";
			char* dllCMPath = (char*)malloc(strlen(gPlatformInfo->app_path) + strlen(libCMName) + 1 );
			strcpy(dllCMPath, gPlatformInfo->app_path);
			strcpy(dllCMPath + strlen(gPlatformInfo->app_path), libCMName);

			LOGI("Lib path: %s", dllCMPath);
			
	    	void* cmDeviceHandle = dlopen(dllCMPath, RTLD_GLOBAL | RTLD_NOW);

			if(cmDeviceHandle != NULL)
			{
				LOGI("Load lib %s success", dllCMPath);
		    	GET_DEVICE_FUN createDeviceFun = (GET_DEVICE_FUN)dlsym(cmDeviceHandle, "createDevice");
				if(createDeviceFun != NULL)
				{
					LOGI("Load sym createDevice success");
			    	gDevice = createDeviceFun(createAudioTrackFun, gPlatformInfo);
				}
				else
				{
					LOGE("Load sym createDevice error: %s", dlerror());
				}
			}
			else
			{
				LOGE("Load lib %s error: %s", dllCMPath, dlerror());
			}
			
			free(dllCMPath);
			
		}
		
		if(gDevice == NULL)
			LOGE("Failed to create device");
    }
	return gDevice;
}

bool OMXWrapper::Create()
{
    gDevice = getDevice();
	return (gDevice!=NULL)? true:false;
}

void OMXWrapper::Destroy()
{
    if(gDevice)
    {
		delete gDevice;
		gDevice = NULL;
    }
}

void OMXWrapper::FlushCommands()
{
    if(gDevice)
    {
		gDevice->FlushCommands();
    }
}

IPPOMXRenderer* OMXRendererWrapper::Create(
            //const sp<IOMX> &omx,
            void* surface,
            const char *componentName,
            OMX_COLOR_FORMATTYPE colorFormat,
            size_t encodedWidth, size_t encodedHeight,
            size_t displayWidth, size_t displayHeight)
{
	LOGD("Create()");
    const sp<Surface> mySurface((Surface*)surface);
    return getDevice()->CreateOMXRenderer(
					            //omx,
					            mySurface,
					            componentName,
					            colorFormat,
					            encodedWidth,
					            encodedHeight,
					            displayWidth,
					            displayHeight);
}
	
sp<MediaSource> OMXCodecWrapper::Create(
            //const sp<IOMX> &omx,
            const sp<MetaData> &meta, bool createEncoder,
            const sp<MediaSource> &source,
            const char *matchComponentName,
            uint32_t flags)
{
	LOGD("Create()");
    return getDevice()->CreateOMXCodec(
	        //omx,
	        meta,
	        createEncoder,
	        source,
	        matchComponentName,
	        flags);
}

IPPAudioTrack* AudioSystemWrapper::createAudioTrack(
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
	LOGD("createAudioTrack()");
    return getDevice()->createAudioTrack(
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

IPPAudioTrack* AudioSystemWrapper::createAudioTrack(
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
	LOGD("createAudioTrack()");
    return getDevice()->createAudioTrack(
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
	
status_t AudioSystemWrapper::getOutputSamplingRate(int* samplingRate, int stream)
{
	LOGD("getOutputSamplingRate()");
    return getDevice()->getOutputSamplingRate(samplingRate, stream);
}
	
status_t AudioSystemWrapper::getOutputFrameCount(int* frameCount, int stream)
{
	LOGD("getOutputFrameCount()");
    return getDevice()->getOutputFrameCount(frameCount, stream);
}
	
bool AudioSystemWrapper::canAudioHWDecode()
{
	LOGD("canAudioHWDecode()");
    return getDevice()->canAudioHWDecode();
}
	
bool AudioSystemWrapper::canAudioHWSBR()
{
	LOGD("canAudioHWSBR()");
    return getDevice()->canAudioHWSBR();
}

}
