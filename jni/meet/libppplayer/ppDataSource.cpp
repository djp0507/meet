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

#define LOG_TAG "ppDataSource"

#include "include-pp/ppDataSource.h"

#include "include-pp/PPBox_Util.h"
#include "platform/platforminfo.h"

namespace android {
extern PlatformInfo* gPlatformInfo;
PPDataSource* PPDataSource::mInstance = NULL;
PPDataSource* PPDataSource::getInstance()
{
	//todo: handle multithread sync
	if(mInstance == NULL)
	{
		mInstance = new PPDataSource();
	}
	else
	{
		mInstance->init();
	}
	return mInstance;
}

void PPDataSource::releaseInstance()
{
	//LOGE("PPDataSource releaseInstance");
	//todo: handle multithread sync
	if(mInstance != NULL)
	{
		delete mInstance;
		mInstance=NULL;
		//LOGE("=========> calling PPBOX_StopP2PEngine");
		//PPBOX_StopP2PEngine();
	}
}

PPDataSource::PPDataSource()
{
	LOGD("PPDataSource constructor");
	mState = DISCONNECTED;
	init();
}

PPDataSource::~PPDataSource() {
	LOGD("PPDataSource descontructor");
	uninit();
}

int64_t getRealTimeUs() {
    struct timeval tv;
    gettimeofday(&tv, NULL);

    return (int64_t)tv.tv_sec * 1000000ll + tv.tv_usec;
}

static bool mClosing;
static Mutex mLock;
static Condition mCondition;
static PP_int32 gRet;
static bool gFinish;
static void open_call_back(
        PP_int32 ec)
{
    gRet = ec;
	gFinish = true;
	mCondition.signal();
}

status_t PPDataSource::openStream(const char *url)
{
    //initialize globle status.
	gFinish = false;
	mClosing = false;
	gRet = ppbox_not_open;
			
    int64_t timeout = 60*1000000;//60sec
	status_t ret = ERROR_NOT_CONNECTED;
	if(url!=NULL)
	{
		size_t n = strlen(url);
		if(n>0)
		{
			LOGI("=========> calling PPBOX_Open %s", url);
			//url = "ppfile-mp4:///data/data/video_m.mp4";
			//PP_int32 ec = PPBOX_Open(url);
			
			//PPBOX_AsyncOpen(url, open_call_back);
			((PPBoxHandle*)gPlatformInfo->ppbox)->asyncOpen(url, open_call_back);

			int64_t beginTime = getRealTimeUs();
			int64_t leftTime = timeout;
			while (!gFinish && leftTime>0)
			{
			    Mutex::Autolock autoLock(mLock);
				mCondition.waitRelative(mLock, 100000000);//0.1sec
				if(mClosing) break;
				leftTime = timeout - (getRealTimeUs()-beginTime);
	        }
			
			if(gRet == ppbox_success)
			{
				LOGI("Open stream success");
				mState = CONNECTED;
				ret = OK;
			}
			else if(leftTime <= 0)
			{
				LOGE("timeout, Open stream failed");
				LOGE("=========> calling PPBOX_Close");
				//PPBOX_Close();
				((PPBoxHandle*)gPlatformInfo->ppbox)->close();
			}
			else if(mClosing)
			{
				LOGD("got stop command, Open stream failed");
				LOGD("=========> calling PPBOX_Close");
				//PPBOX_Close();
				((PPBoxHandle*)gPlatformInfo->ppbox)->close();
			}
			else
			{
				LOGE("Open stream failed with ret:%d", gRet);
				LOGE("=========> calling PPBOX_Close");
				//PPBOX_Close();
				((PPBoxHandle*)gPlatformInfo->ppbox)->close();
			}			
		}
	}
	return ret;
}

void PPDataSource::closeStream()
{
    mClosing = true;
	if(mState==CONNECTED)
	{
		LOGD("=========> calling PPBOX_Close");
		//PPBOX_Close();
		((PPBoxHandle*)gPlatformInfo->ppbox)->close();
		mState = CONNECTING;
	}
}

status_t PPDataSource::initCheck() const {
	return (mState != DISCONNECTED) ? (status_t)OK : ERROR_NOT_CONNECTED;
}

ssize_t PPDataSource::readAt(off_t offset, void *data, size_t size) {
	//LOGE("PPDataSource reading data with threadID: %d", getThreadId());
	//todo.
	ssize_t len = 5;
	memcpy(data, (void*)"PPVOD",len);
	return len;
}

status_t PPDataSource::getSize(off_t *size) {
    //todo.
    //LOGE("PPDataSource getSize");
    return ERROR_NOT_CONNECTED;
}

status_t PPDataSource::init()
{
	//LOGE("PPDataSource init");
	status_t ret = OK;
	mState = CONNECTING;
	/*
	if(mState==DISCONNECTED ||mState==CONNECTING)
	{
		LOGE("=========> calling PPBOX_StartP2PEngine");
		PP_int32 ec = PPBOX_StartP2PEngine("12", "161", "08ae1acd062ea3ab65924e07717d5994");
		if (ppbox_success != ec) {
			LOGE("Start p2p engine failed with code:%d",ec);
			ret = ERROR_NOT_CONNECTED;
		}
		else
		{
			LOGE("Start p2p engine success");
			mState = CONNECTING;
		}
	}
	*/
	return ret;
}
 
void PPDataSource::uninit() {
	//LOGE("PPDataSource uninit");
	closeStream();
}

}  // namespace android
