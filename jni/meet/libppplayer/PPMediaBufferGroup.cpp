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

#define LOG_TAG "PPMediaBufferGroup"
#include "include-pp/utils/Log.h"
//#include "include-pp/log.h"

#include "include-pp/MediaBuffer.h"
#include "include-pp/PPMediaBufferGroup.h"
#include "include-pp/MediaErrors.h"
#include "include-pp/sf/MetaData.h"
#include "include-pp/MediaDebug.h"

//#include <media/stagefright/MediaDebug.h>
//#include <media/stagefright/MediaBuffer.h>
//#include <media/stagefright/MetaData.h>

namespace android {

PPMediaBufferGroup::PPMediaBufferGroup()
    : mFirstBuffer(NULL),
      mLastBuffer(NULL),
      mReadingBuffer(NULL),
      mWritingBuffer(NULL),
      mStartBuffering(true),
      mIsStreamDone(false),
      mIsSeeking(false),
      mIsStopping(false),
      mBufferCount(0),
      mBufferingCount(0),
      mLastSampleTime(0),
      mBuffer(NULL),
      mSize(0),
      mReadingAddr(NULL),
      mWritingAddr(NULL)
{
      LOGD("memory allocation at %p", this);
}

PPMediaBufferGroup::PPMediaBufferGroup(size_t size, size_t playBufferingCount)
    : mFirstBuffer(NULL),
      mLastBuffer(NULL),
      mReadingBuffer(NULL),
      mWritingBuffer(NULL),
      mStartBuffering(true),
      mIsStreamDone(false),
      mIsSeeking(false),
      mIsStopping(false),
      mBufferCount(0),
      mBufferingCount(0),
      mLastSampleTime(0),
      mPlayBufferingCount(playBufferingCount),
      mBuffer(NULL),
      mSize(size),
      mReadingAddr(NULL),
      mWritingAddr(NULL)
{
	LOGD("memory allocation at %p", this);
	mBuffer=(uint8_t*)malloc(size);
	mWritingAddr = mBuffer + 1;
	mReadingAddr = mBuffer;	
}

PPMediaBufferGroup::~PPMediaBufferGroup() {
	LOGD("memory release at %p", this);
	clear_all_buffer();
	
	if(mReadingBuffer!=NULL)
	{
		MediaBuffer *begin = mReadingBuffer;
		MediaBuffer *next = mReadingBuffer->nextBuffer();
		if(begin==next)
		{
		        CHECK_EQ(begin->refcount(), 0);
		        begin->setObserver(NULL);
		        begin->release();
		}
		else
		{
			begin->setNextBuffer(NULL);
			for (MediaBuffer *buffer = next; buffer != NULL; buffer = buffer->nextBuffer())
			{
				CHECK_EQ(buffer->refcount(), 0);
				buffer->setObserver(NULL);
				buffer->release();
			}
		}
			
	}
	if(mBuffer)
	{
		delete mBuffer;
		mBuffer=NULL;
		mWritingAddr=NULL;
		mReadingAddr=NULL;
	}
}


void PPMediaBufferGroup::signalBufferReturned(MediaBuffer *buffer) {
	//LOGE("signalBufferReturned 1");
	//Mutex::Autolock autoLock(mLock);
	
	mReadingAddr=(uint8_t*)buffer->data()+buffer->size();
	buffer->setObserver(NULL);
	delete buffer;
	//LOGE(">>>>>>>>>> [%p][%p][%d] released buffer", this,getThreadId(), mBufferingCount);
	mCondition.signal();
	//LOGE("signalBufferReturned 2");
}

/*


PPMediaBufferGroup::~PPMediaBufferGroup() {
    MediaBuffer *next;
    for (MediaBuffer *buffer = mFirstBuffer; buffer != NULL;
         buffer = next) {
        next = buffer->nextBuffer();

        CHECK_EQ(buffer->refcount(), 0);

        buffer->setObserver(NULL);
        buffer->release();
    }
}

void PPMediaBufferGroup::add_buffer(MediaBuffer *buffer) {
    Mutex::Autolock autoLock(mLock);

    buffer->setObserver(this);

    if (mLastBuffer) {
        mLastBuffer->setNextBuffer(buffer);
    } else {
        mFirstBuffer = buffer;
	 mReadingBuffer = mFirstBuffer;
    }

    mLastBuffer = buffer;
}

status_t PPMediaBufferGroup::acquire_buffer(MediaBuffer **out) {
    Mutex::Autolock autoLock(mLock);

    for (;;) {
        for (MediaBuffer *buffer = mFirstBuffer;
             buffer != NULL; buffer = buffer->nextBuffer()) {
            if (buffer->refcount() == 0) {
                buffer->add_ref();
                buffer->reset();

                *out = buffer;
		  mWritingBuffer=buffer;
		LOGE(">>>>>>>>>> [%d][%d]buffer acquired",this, getThreadId());
                goto exit;
            }
        }

        // All buffers are in use. Block until one of them is returned to us.
        LOGE(">>>>>>>>>> [%d][%d] mStartBuffering to false", this,getThreadId());
        mStartBuffering=false;
		mConditionBuffering.signal();
        LOGE("######## buffer is full, waiting");
        mCondition.wait(mLock);
    }

exit:
    return OK;
}
*/


void PPMediaBufferGroup::add_buffer(MediaBuffer *buffer) {
	if(buffer==NULL) return;

    Mutex::Autolock autoLock(mLock);
	buffer->setObserver(this);

	if (mWritingBuffer)
	{
		buffer->setNextBuffer(mWritingBuffer->nextBuffer());
		mWritingBuffer->setNextBuffer(buffer);
	}
	else
	{
		mWritingBuffer=buffer;
		mReadingBuffer=buffer;
		mWritingBuffer->setNextBuffer(mWritingBuffer);
	}
	mBufferCount++;
}

status_t PPMediaBufferGroup::acquire_buffer(MediaBuffer **out)
{
	Mutex::Autolock autoLock(mLock);

acquiring:
	
	if(mWritingBuffer != NULL)
	{
		MediaBuffer *buffer = mWritingBuffer;
		if(buffer->refcount() == 0)
		{
			buffer->add_ref();
			buffer->reset();

	       	// LOGE(">>>>>>>>>> [%d][%d][%d] acquired buffer", this,getThreadId(), mBufferingCount);
			*out = buffer;
			mWritingBuffer=buffer->nextBuffer();
			mBufferingCount++;
			if(mBufferingCount > (mBufferCount/2) && mStartBuffering)
			{
			     // LOGE(">>>>>>>>>> [%d][%d][%d] mStartBuffering to false", this,getThreadId(), mBufferingCount);
			     mStartBuffering = false;
				 mConditionBuffering.signal();
			}
		}
		else
		{
			// LOGE(">>>>>>>>>> [%d][%d][%d] All buffers are in use, waiting", this,getThreadId(), mBufferingCount);
			// All buffers are in use. Block until one of them is returned to us.
			mCondition.wait(mLock);
			goto acquiring;
		}
	}
	return OK;
}

status_t PPMediaBufferGroup::consume_buffer(MediaBuffer **out) {
	if(mReadingBuffer==NULL) return ERROR_IO;

       Mutex::Autolock autoLock(mLock);
	//LOGE(">>>>>>>>>> [%d][%d][%d]start consuming buffer",this, getThreadId(),mBufferingCount);
	
waiting:
	while(mStartBuffering || mReadingBuffer->refcount()==0)
	{
		//LOGE(">>>>>>>>>> [%d][%d][%d]waiting buffering to finish",this, getThreadId(),mBufferingCount);
		//LOGE("mReadingBuffer->refcount()?: %d",(mReadingBuffer->refcount()==0));
		//LOGE("mStartBuffering?: %d",mStartBuffering);
		
		mConditionBuffering.waitRelative(mLock,100000000);//0.1sec
	}
	if(mReadingBuffer->nextBuffer()==mWritingBuffer)
	{
        	//LOGE(">>>>>>>>>> [%d][%d][%d] mStartBuffering to true", this,getThreadId(),mBufferingCount);
		mStartBuffering=true;
		goto waiting;

	}
	//LOGE(">>>>>>>>>> [%d][%d][%d] consumed buffer",this, getThreadId(),mBufferingCount);
	
	*out=mReadingBuffer;
	
	mReadingBuffer = mReadingBuffer->nextBuffer();
//	if(mReadingBuffer==NULL) mReadingBuffer=mFirstBuffer;
	//LOGE("########buffer consumed");
	mBufferingCount--;
	return OK;
}


status_t PPMediaBufferGroup::acquire_buffer(MediaBuffer **out, size_t len)
{
	if((len+2) >mSize) return ERROR_IO;

acquiring:

	if(mIsSeeking || mIsStopping) return ERROR_IO;

	MediaBuffer *buffer = NULL;
	LOGD("MediaBuffer size:%d", sizeof(MediaBuffer));
	if(mWritingAddr > mReadingAddr)
	{
		if(mWritingAddr+len+1<mBuffer+mSize)
		{
			//LOGE(">>>>>>>>>> [%d][%d][%d] find new buffer at end", this,getThreadId(), mBufferingCount);
			//LOGE("mWritingAddr:%d,len:%d,mBuffer:%d,mSize:%d",mWritingAddr,len,mBuffer,mSize);
			buffer = new MediaBuffer(mWritingAddr, len);
			//mWritingAddr+=(len+1);
		}
		else if(mBuffer+len+1 < mReadingAddr)
		{
			//LOGE(">>>>>>>>>> [%d][%d][%d] find new buffer at front", this,getThreadId(), mBufferingCount);
			//LOGE("mWritingAddr:%d,len:%d,mBuffer:%d,mSize:%d",mWritingAddr,len,mBuffer,mSize);
			buffer = new MediaBuffer(mBuffer, len);
			//mWritingAddr=(mBuffer+len+1);
		}
		else
		{
	        //LOGE(">>>>>>>>>> [%d][%d][%d] All buffers are in use, waiting", this,getThreadId(), mBufferingCount);
	        // All buffers are in use. Block until one of them is returned to us.
	        mCondition.wait(mLock);
			goto acquiring;
		}
	}
	else if(mWritingAddr < mReadingAddr)
	{
		if(mWritingAddr+len+1 < mReadingAddr)
		{
			//LOGE(">>>>>>>>>> [%d][%d][%d] find new buffer at middle", this,getThreadId(), mBufferingCount);
			//LOGE("mWritingAddr:%d,len:%d,mBuffer:%d,mSize:%d",mWritingAddr,len,mBuffer,mSize);
			buffer = new MediaBuffer(mWritingAddr, len);
			//mWritingAddr+=(len+1);
		}
		else
		{
	        //LOGE(">>>>>>>>>> [%d][%d][%d] All buffers are in use, waiting", this,getThreadId(), mBufferingCount);
	        // All buffers are in use. Block until one of them is returned to us.
	        mCondition.wait(mLock);
			goto acquiring;
		}
	}
	else
	{
		//LOGE("this should never happen");
		return ERROR_IO;
	}
	if(buffer==NULL) return ERROR_IO;
	
	buffer->reset();
	*out=buffer;		
	return OK;
}

status_t PPMediaBufferGroup::push_buffer(uint8_t *data, size_t size, int64_t frameTime, bool isSync)
{
	if(data==NULL || size<=0) return ERROR_IO;
	Mutex::Autolock autoLock(mLock);

	MediaBuffer* buffer = NULL;
	acquire_buffer(&buffer, size);
	
	if(buffer==NULL) return ERROR_IO;

	buffer->meta_data()->clear();
	buffer->meta_data()->setInt64(kKeyTime, frameTime);
	buffer->meta_data()->setInt32(kKeyIsSyncFrame, isSync);
	
	memcpy(buffer->data(), data, size);
	
	buffer->add_ref();
	buffer->setObserver(this);
	
	if (mLastBuffer) {
		mLastBuffer->setNextBuffer(buffer);
	} else {
		mFirstBuffer = buffer;
	}
	
	mLastBuffer = buffer;
	
	mBufferingCount++;
	
	LOGD(">>>>>>>>>> [this:%p][%d]add buffer with len: %d, sampleTime: %lld(ms), isSync: %d",this, mBufferingCount, buffer->range_length(), frameTime/1000, isSync);
	if(mBufferingCount >= mPlayBufferingCount && mStartBuffering)
	{
	     //LOGE(">>>>>>>>>> [this:%p][%d] mStartBuffering to false", this, mBufferingCount);
	     mStartBuffering = false;
		 mConditionBuffering.signal();
	}

	mWritingAddr = (uint8_t*)buffer->data()+buffer->range_length() + 1;
	mLastSampleTime = frameTime;
	return OK;
}

status_t PPMediaBufferGroup::pop_buffer(MediaBuffer **out) {
    //LOGE("pop_buffer 1");
    Mutex::Autolock autoLock(mLock);
	//LOGE(">>>>>>>>>> [this:%p][%d]start reading frame buffer",this, mBufferingCount);
	//int64_t timeout = 10000000000;//10sec
	//int64_t waitTimeUnit = 100000000;//0.1sec
	
waiting:
    //LOGE("pop_buffer 2");
	while(mStartBuffering && !mIsStreamDone && !mIsStopping /* && timeout>=0*/)
	{
		//LOGE(">>>>>>>>>> [%p][%p][%d]waiting buffer to finish", this, getThreadId(), mBufferingCount);
		mConditionBuffering.waitRelative(mLock, 100000000);//0.1sec
		//timeout-=waitTimeUnit;
	}
	
	if(mIsStopping) return ERROR_END_OF_STREAM;
	
    // LOGE("pop_buffer 4");
	/*
	if(timeout < 0)
	{
	    LOGE(">>>>>>>>>> [%p][%p][%d] timeout, mPlayBufferingCount %d", this, getThreadId(),mBufferingCount,mPlayBufferingCount);
		*out=new MediaBuffer(0);
		(*out)->meta_data()->clear();
		(*out)->meta_data()->setInt64(kKeyTime, mLastSampleTime);
		return OK;
	}
	*/	
	
	if(mFirstBuffer == NULL)
	{
		//LOGE("pop_buffer 4.1");
		if(mIsStreamDone) return ERROR_END_OF_STREAM;
		//LOGE("pop_buffer 4.2");
		LOGD(">>>>>>>>>> [this: %p][%d] start buffering", this, mBufferingCount);
		mStartBuffering = true;
		/*
		mPlayBufferingCount+=250;
		if(mPlayBufferingCount>=500)
			mPlayBufferingCount=500;
		*/
		goto waiting;
		//LOGE("pop_buffer 4.3");
	}
	else
	{
		//LOGE("pop_buffer 5.1");
		*out = mFirstBuffer;
		//LOGE("pop_buffer 5.2");
		mFirstBuffer = mFirstBuffer->nextBuffer();
		//LOGE("pop_buffer 5.3");
		mBufferingCount--;
		
		//LOGE(">>>>>>>>>> [this:%p][%d] found buffer",this, mBufferingCount);
		if(mFirstBuffer == NULL)
		{
		    //LOGE("pop_buffer 5.3.1");
			mLastBuffer = NULL;

			mStartBuffering = true;
		} 
		//else if (mFirstBuffer == mLastBuffer) {
		//	mStartBuffering = true;
		//}
		//LOGE("pop_buffer 5.4");
		return OK;
	}
}

void PPMediaBufferGroup::clear_buffer(int64_t seekTime)
{
	clear_all_buffer();
	//comment bellow as current pptv online video has long time gop and our memory buffer is with small size, 
	//keep cached memory frames does not make much help.
	/*
	Mutex::Autolock autoLock(mLock);

	while(mFirstBuffer!=NULL)
	{
	    int64_t timestampUs=0;
		mFirstBuffer->meta_data()->findInt64(kKeyTime, &timestampUs);
		int64_t timeDiff = seekTime-timestampUs;
		timeDiff = timeDiff>0?timeDiff:-timeDiff;

	    int32_t isSync = 0;
		mFirstBuffer->meta_data()->findInt32(kKeyIsSyncFrame, &isSync);
		LOGE("clear_buffer at %lld", timeDiff);
		if(timeDiff<=5000000 && isSync)//as pptv online video has gop:250fs
		{
			LOGE("clear_buffer,key frame, need to keep it %lld", timeDiff);
			return;
		}
		
		MediaBuffer *buffer = mFirstBuffer;
		mFirstBuffer=mFirstBuffer->nextBuffer();
		LOGE("clear_buffer start");
		//buffer->release();
		{
			mReadingAddr=(uint8_t*)buffer->data()+buffer->size();
			delete buffer;
		    mCondition.signal();
		}
		LOGE("clear_buffer end");
	}
	
	mFirstBuffer = NULL;
	mLastBuffer = NULL;
	mStartBuffering = true;
	//mIsStreamDone = false;
	mBufferingCount = 0;
	mWritingAddr = mBuffer+1;
	mReadingAddr = mBuffer;
	//mCondition.signal();
	*/
}

void PPMediaBufferGroup::clear_all_buffer()
{
	LOGD("clear_all_buffer begin");
	Mutex::Autolock autoLock(mLock);

	while(mFirstBuffer!=NULL)
	{		
		MediaBuffer *buffer = mFirstBuffer;
		mFirstBuffer=mFirstBuffer->nextBuffer();
		buffer->setObserver(NULL);
		delete buffer;
	}
	
	mFirstBuffer = NULL;
	mLastBuffer = NULL;
	mStartBuffering = true;
	//mIsStreamDone = false;
	mBufferingCount = 0;
	mWritingAddr = mBuffer+1;
	mReadingAddr = mBuffer;
	mCondition.signal();
	LOGD("clear_all_buffer end");
}


void PPMediaBufferGroup::notify_streamdone()
{
	LOGD("notify_streamdone begin");
	//Mutex::Autolock autoLock(mLock);
	mIsStreamDone=true;
	mStartBuffering=false;
	mConditionBuffering.signal();
	//mCondition.signal();
	LOGD("notify_streamdone begin");
}

void PPMediaBufferGroup::notify_stop()
{
	LOGD("notify_stop begin");
	//Mutex::Autolock autoLock(mLock);
	mIsStopping=true;
	mStartBuffering=false;
	mConditionBuffering.signal();
	mCondition.signal();
	LOGD("notify_stop end");
}

void PPMediaBufferGroup::notify_seekstart()
{
	LOGD("notify_seekstart begin");
	Mutex::Autolock autoLock(mLock);
	mIsSeeking=true;
	mIsStreamDone=false;
	mCondition.signal();
	LOGD("notify_seekstart end");
}

void PPMediaBufferGroup::notify_seekdone()
{
    LOGD("notify_seekdone begin");
	Mutex::Autolock autoLock(mLock);
	mIsSeeking=false;
    LOGD("notify_seekdone end");
}

int64_t PPMediaBufferGroup::getCachedDurationUs() //us
{
//	Mutex::Autolock autoLock(mLock);
	int64_t duration = 0;
	if(mFirstBuffer && mLastBuffer)
	{
		int64_t firstFrameTime;
		int64_t lastFrameTime;
		mFirstBuffer->meta_data()->findInt64(kKeyTime, &firstFrameTime);
		mLastBuffer->meta_data()->findInt64(kKeyTime, &lastFrameTime);
		duration = lastFrameTime - firstFrameTime + 40 * 1000l;
	}
	//LOGE("getCachedDurationUs %lld", duration);
	return duration;
}

bool PPMediaBufferGroup::isBuffering()
{
	return mStartBuffering;
}

}  // namespace android
