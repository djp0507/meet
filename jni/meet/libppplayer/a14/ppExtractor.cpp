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
#define LOG_TAG "PPExtractor"

#include "platform-pp/a14/PPPlatForm.h"

#include "libppplayer/a14/MediaDefs.h"
#include "libppplayer/a14/ppDataSource.h"
#include "libppplayer/a14/ppExtractor.h"

#include "include-pp/a14/frameworks/base/include/media/stagefright/foundation/AMessage.h"
#include "include-pp/a14/frameworks/base/include/media/stagefright/Utils.h"
#include "include-pp/a14/frameworks/base/include/utils/Log.h"
#include "include-pp/a14/frameworks/base/include/utils/String8.h"
#include "include-pp/a14/frameworks/base/include/utils/threads.h"
#include "include-pp/a14/system/core/include/cutils/properties.h"

#include <sys/prctl.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <arpa/inet.h>
#include <ctype.h>

#include "include-pp/PPBox_Util.h"
#include "platform/platforminfo.h"

namespace android {

extern PlatformInfo* gPlatformInfo;
extern void parse_sps(uint8_t * sps,size_t sps_size,uint8_t *aprofile,uint8_t *alevel,uint32_t *num_ref_frames,uint8_t *interlaced);

const int32_t video_buffer_play_count = 1; // the sample buffer count within
const int32_t audio_buffer_play_count = 200*2; // the sample buffer count within
const int32_t video_buffer_size = 1024*1024*6; // the max video frame buffer size.
const int32_t audio_buffer_size = 1024*512*2; //the max audio frame buffer size.
const int32_t max_video_frame_size = 1024*100;
const int32_t max_audio_frame_size = 1024;

struct PPMediaSource : public MediaSource {
	PPMediaSource();
    PPMediaSource(size_t index, const sp<MetaData> &format, sp<PPExtractor> extractor);

    virtual sp<MetaData> getFormat();

    virtual status_t start(MetaData *params = NULL);
    virtual status_t stop();

    virtual status_t read(
            MediaBuffer **buffer, const ReadOptions *options = NULL);

protected:
    virtual ~PPMediaSource();

private:
    Mutex mLock;
    size_t mTrackIndex;
    sp<PPExtractor> mExtractor;
    bool mStarted;
    bool mIsAVC;
    bool mIsAAC;
    sp<MetaData> mFormat;

    PPMediaSource(const PPMediaSource &);
    PPMediaSource &operator=(const PPMediaSource &);
};

PPMediaSource::PPMediaSource()
{
      LOGD("memory allocation at %p", this);
}

PPMediaSource::PPMediaSource(size_t index, const sp<MetaData> &format, sp<PPExtractor> extractor)
    : mTrackIndex(index),
      mExtractor(extractor),
      mStarted(false),
      mFormat(format),
      mIsAVC(false),
      mIsAAC(false)
{
      LOGD("memory allocation at %p", this);

	const char *mime;
	bool success = mFormat->findCString(kKeyMIMEType, &mime);
	CHECK(success);
	mIsAVC = !strcasecmp(mime, MEDIA_MIMETYPE_VIDEO_AVC);
	mIsAAC = !strcasecmp(mime, MEDIA_MIMETYPE_AUDIO_AAC);

}

PPMediaSource::~PPMediaSource() 
{
	LOGD("memory release at %p", this);
	
	if (mStarted) {
		stop();
	}
}

sp<MetaData> PPMediaSource::getFormat() {
	LOGD("PPMediaSource getFormat");
	return mFormat;
}

status_t PPMediaSource::start(MetaData *params)
{
	//Mutex::Autolock autoLock(mLock);

	if (!mStarted) {
		mExtractor->start();
		mStarted = true;
	}
	return OK;
}

status_t PPMediaSource::stop()
{
	//Mutex::Autolock autoLock(mLock);
    if(!mStarted) return OK;
    
    LOGD("++++++++PPMediaSource Start stopping, thread id: %p", getThreadId());
    
	mExtractor->stop();
	mStarted = false;

    LOGD("++++++++PPMediaSource End stopping, thread id: %p", getThreadId());
	return OK;
}

status_t PPMediaSource::read(MediaBuffer **out, const ReadOptions *options) 
{
	LOGD("PPMediaSource::read 1");
    //Mutex::Autolock autoLock(mLock);
	CHECK(mStarted);
	status_t ret = ERROR_IO;
	if(mIsAVC)//video
	{
	// LOGE("PPMediaSource::read 2.1");
		int64_t seekTimeUs;
		ReadOptions::SeekMode mode;
	// LOGE("PPMediaSource::read 2.2");
		if (options && options->getSeekTo(&seekTimeUs, &mode)) {
	// LOGE("PPMediaSource::read 2.2.1");
			//as video/audio share one ppextractor instance. so only 
			//have video media source to do seek which will cause ppextractor to do seek.
			mExtractor->seekTo(seekTimeUs);
	// LOGE("PPMediaSource::read 2.2.2");
		}
	// LOGE("PPMediaSource::read 2.3");
		
		ret = mExtractor->readVideoSample(out);
	// LOGE("PPMediaSource::read 2.4");
		if(ret==OK)
		{
			//LOGE("read video sample with size = %d",(*out)->range_length());
		}
		else if(ret==ERROR_END_OF_STREAM)
			LOGE("no more data can be read");
		else
			LOGE("read video sample failed");
			
	}
	else if(mIsAAC)//audio
	{
	    //audio does not seek.
	//LOGE("PPMediaSource::read 3.1");
		ret = mExtractor->readAudioSample(out);
	//LOGE("PPMediaSource::read 3.2");
		if(ret==OK)
		{
			//LOGE("read audio sample with size = %d",(*out)->range_length());
		}
		else if(ret==ERROR_END_OF_STREAM)
			LOGE("no more data can be read");
		else
			LOGE("read audio sample failed");
	}
	else
	{
	}
	//LOGE("PPMediaSource::read 2.5");
	return ret;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////

/* test, can be deleted.
void setMediaSource(const sp<MediaSource> &codec)
{
    LOGE("before set target");
    sp<MediaSource> target = codec;
    LOGE("end set target");
}
// static
sp<MediaSource>* createMediaSource() {
    LOGE("Start instance create test");
    sp<MediaSource>* source = new sp<MediaSource>(new PPMediaSource());
    LOGE("after instance create test");
	return source;
}
*/
PPExtractor::PPExtractor(const sp<DataSource> &source)
    : mDataSource(source),
    mTrackCount(0),
    mStarted(false),
    mAudioTrackID(-1),
    mVideoTrackID(-1),
    mVideoBufferGroup(NULL),
    mAudioBufferGroup(NULL),
    mSeekTimeUs(0),
    mLastSampleTime(0),
    isSeeking(false),
    isBlocking(false),
    isStreamDone(false),
    mDurationTimeUs(0)
{
	LOGD("PPExtractor constructor");
/* test, can be deleted.
	{
		sp<MediaSource> *source1=NULL;
		LOGE("before call createMediaSource");
		source1 = createMediaSource();
		LOGE("set target");
		setMediaSource(*source1);
		LOGE("after call createMediaSource");
		sp<MediaSource> spSource = *source1;
		LOGE("after init new sp");
		delete source1;
		LOGE("after delete source ptr");
	}
	LOGE("exit test environment");
	
*/

	mTrackCount = countTracks();
	
	if(mTrackCount>0)
	{
		for(size_t i=0;i<mTrackCount;i++)
		{
			sp<MetaData> meta = getTrackMetaData(i);
			if(meta.get()!=NULL)
			{
				const char *mime;
	        	CHECK(meta->findCString(kKeyMIMEType, &mime));
					
				if (!strncasecmp(mime, "video/", 6))
				{
					mVideoTrackID=i;
					mVideoFormat=meta;
					
					uint32_t type;
					const void *data;
					size_t size;
					//sp<MetaData> format=getTrackMetaData(i);
					CHECK(meta->findData(kKeyAVCC, &type, &data, &size));

					const uint8_t *ptr = (const uint8_t *)data;

					CHECK(size >= 7);
					CHECK_EQ(ptr[0], 1);  // configurationVersion == 1

					// The number of bytes used to encode the length of a NAL unit.
					mNALLengthSize = 1 + (ptr[4] & 3);
					LOGD("NALU: mNALLengthSize: %d", mNALLengthSize);
				}
				else if(!strncasecmp(mime, "audio/", 6))
				{
					mAudioTrackID=i;
					mAudioFormat=meta;
				}
				else
				{
					//todo
				}
				
				if(mDurationTimeUs==0) {
					mDurationTimeUs = ((PPBoxHandle*)gPlatformInfo->ppbox)->getDuration();
					//mDurationTimeUs = PPBOX_GetDuration();
				}
			}
		}
	}
}

PPExtractor::~PPExtractor() { 
    LOGD("PPExtractor deconstructor");
}

size_t PPExtractor::countTracks() {
	if(mTrackCount <= 0)
	{
		LOGD("=========> calling PPBOX_GetStreamCount");
		//mTrackCount =  PPBOX_GetStreamCount();
		mTrackCount = ((PPBoxHandle*)gPlatformInfo->ppbox)->getStreamCount();
		LOGD("return count: %d", mTrackCount);
	}
	return mTrackCount;
}

sp<MediaSource> PPExtractor::getTrack(size_t index) {
    if (index >= mTrackCount) {
		LOGE("Requested track index %d is invalid", index);
        return NULL;
    }
    //sp<MetaData> format = getTrackMetaData(index);
    if(index == mAudioTrackID)
	{
	    return new PPMediaSource(index, mAudioFormat, this);
	}
	else if(index == mVideoTrackID)
	{
	    return new PPMediaSource(index, mVideoFormat, this);
	}
	else
	{
		//TODO
	}
	
	LOGE("Unknown track ID");
	return NULL;
}

bool parseAOTs(PPBOX_StreamInfoEx* stream_info, uint32_t* sampleRate, uint32_t* channelCount)
{
	if(stream_info == NULL) return false;
	
	static uint32_t kSamplingRate[] = {
            96000, 88200, 64000, 48000, 44100, 32000, 24000, 22050,
            16000, 12000, 11025, 8000, 7350
        };
	static uint32_t kChannelCount[] = {
            0, 1, 2
        };

	const uint8_t* audioConfig=stream_info->format_buffer;
	LOGD("========AOTs========");
	for(uint8_t i=0;i<stream_info->format_size;i++)
	{
		LOGD("[%d]",*(audioConfig+i));
	}
	LOGD("========AOTs========");
	
    uint8_t audioObjectType = ((*audioConfig)>>3);
	LOGD("audioObjectType: %d", audioObjectType);
	if(audioObjectType==31)
	{
		audioObjectType=(((*audioConfig)&0x7)<<3)+((*(audioConfig+1))>>5);
		LOGE("audioObjectType new: %d not supported.", audioObjectType);
		return false;
	}

	uint8_t samplingFrequencyIndex = (((*audioConfig)&0x7)<<1)+((*(audioConfig+1))>>7);
	LOGD("samplingFrequencyIndex: %d", samplingFrequencyIndex);
	if(samplingFrequencyIndex==15)
	{
		LOGE("samplingFrequencyIndex is not supported.");
		return false;
	}

	uint8_t channelConfiguration = (((*(audioConfig+1))&0x78)>>3);
	LOGD("channelConfiguration: %d", channelConfiguration);
	*channelCount = kChannelCount[channelConfiguration];

	if(AudioSystemWrapper::canAudioHWDecode())
	{
	    if(!AudioSystemWrapper::canAudioHWSBR())
	    {
			// the device does not support SBR.
	        *sampleRate = kSamplingRate[samplingFrequencyIndex];
			return true;
	    }
	}

	bool gotSampleRate=false;
	if(audioObjectType==5)
	{
		kSamplingRate[samplingFrequencyIndex]*2;
		gotSampleRate=true;
	}
	else if(stream_info->format_size>=4)
	{
		LOGD("1");
		const uint16_t syncExtensionType=((*(audioConfig+2))<<3)+((*(audioConfig+3))>>5);
		LOGD("syncExtensionType: %d", syncExtensionType);
		if(syncExtensionType==0x2b7)
		{
			LOGD("2");
			uint8_t extensionAudioObjectType=(*(audioConfig+3))&0x1f;
			LOGD("extensionAudioObjectType: %d", extensionAudioObjectType);
			if(extensionAudioObjectType==5)
			{
				LOGD("3");
				uint8_t sbrPresentFlag=(*(audioConfig+4))>>7;
				LOGD("sbrPresentFlag:%d", sbrPresentFlag);
				if(sbrPresentFlag == 1)
				{
					LOGD("4");
					uint8_t extensionSamplingFrequencyIndex=((*(audioConfig+4))&0x7f)>>3;
					LOGD("extensionSamplingFrequencyIndex: %d", extensionSamplingFrequencyIndex);
					if (extensionSamplingFrequencyIndex > 12) {
						return ERROR_MALFORMED;
					}
					LOGD("got extension sample rate");
					*sampleRate = kSamplingRate[extensionSamplingFrequencyIndex];
					gotSampleRate=true;
				}
			}
		}
	}

	if(!gotSampleRate)
		*sampleRate = kSamplingRate[samplingFrequencyIndex];
	return true;
}

sp<MetaData> PPExtractor::getTrackMetaData(size_t index, uint32_t flags)
{
	LOGD("getTrackMetaData");
	if (index >= mTrackCount)
	{
		LOGE("The requested track id : %d is not valid", index);
		return NULL;
	}

	if(mAudioTrackID!=-1 && mAudioTrackID ==index && mAudioFormat.get()!=NULL)
	{
		return mAudioFormat;
	}
	if(mVideoTrackID!=-1 && mVideoTrackID ==index && mVideoFormat.get()!=NULL)
	{
		return mVideoFormat;
	}

	sp<MetaData> meta = new MetaData; 
	LOGD("=========> calling PPBOX_GetStreamInfoEx");
	PPBOX_StreamInfoEx stream_info;
	//PPBOX_GetStreamInfoEx(index, &stream_info);
	((PPBoxHandle*)gPlatformInfo->ppbox)->getStreamInfoEx(index, &stream_info);
	if (stream_info.type == ppbox_video)
	{
		if(stream_info.sub_type == ppbox_video_avc)
		{
    		meta->setCString(kKeyMIMEType, MEDIA_MIMETYPE_VIDEO_AVC);
			meta->setData(kKeyAVCC, kTypeAVCC, stream_info.format_buffer, stream_info.format_size);
			meta->setInt32(kKeyWidth, stream_info.video_format.width);
            meta->setInt32(kKeyHeight, stream_info.video_format.height);
			//int64_t durationTimeUs = PPBOX_GetDuration();
			int64_t durationTimeUs = ((PPBoxHandle*)gPlatformInfo->ppbox)->getDuration();
			durationTimeUs *= 1000;
            meta->setInt64(kKeyDuration, durationTimeUs);
            //meta->setInt32(kKeyMaxInputSize, max_video_frame_size + 10 * 2);


			LOGI("kKeyMIMEType: %s", MEDIA_MIMETYPE_VIDEO_AVC);
			LOGI("kKeyWidth: %d", stream_info.video_format.width);
			LOGI("kKeyHeight: %d", stream_info.video_format.height);
			//LOGI("kKeyDuration: %lld, PPBOX_GetDuration(): %d", durationTimeUs, PPBox_GetDuration());
			LOGI("kKeyDuration: %lld, PPBOX_GetDuration(): %d", durationTimeUs, ((PPBoxHandle*)gPlatformInfo->ppbox)->getDuration());
			LOGI("kKeyMaxInputSize: %d", max_video_frame_size + 10 * 2);
			LOGI("format_type: %d", stream_info.format_type);
			LOGI("format_size: %d", stream_info.format_size);
			
			PP_uchar const * p = stream_info.format_buffer;
			
			{
			    //uint8_t profile,level,interlaced;
			    //uint32_t num_ref_frames;
				//parse_sps((uint8_t*)p, stream_info.format_size, &profile, &level, &num_ref_frames, &interlaced);
				//LOGE("profile:%u, level:%u, num_ref_frames:%u, interlaced:%d", profile, level, num_ref_frames, interlaced);
			}
			
			int32_t profile = (int)*p++;
			meta->setInt32(kKeyVideoProfile, profile);
			int32_t level = (int)*p++;
			meta->setInt32(kKeyVideoLevel, level);

			LOGI("Configuration Version: %d", (int)*p++);
			LOGI("Profile: %d", profile);
			LOGI("Profile Compatibility: %d", (int)*p++);
			LOGI("Level: %d", level);
			LOGI("NALU Length Size: %d", 1 + ((*p++) & 3));
			//meta->setInt32(kKeyNumRefFrames, 50);//default value.
		}
		else
		{
			LOGE("video sub_type not supported");
			//todo
		}
	}
	else if(stream_info.type == ppbox_audio) {
		if(stream_info.sub_type == ppbox_audio_aac)
		{
			uint32_t sampleRate, channelCount;
			bool result = parseAOTs(&stream_info, &sampleRate, &channelCount);
			//channelCount=stream_info.audio_format.channel_count;
			if(result==false)
			{
				LOGE("Parse AOTs failed.");
				return NULL;
			}
			//int64_t durationTimeUs = PPBOX_GetDuration();
			int64_t durationTimeUs = ((PPBoxHandle*)gPlatformInfo->ppbox)->getDuration();
			durationTimeUs *= 1000;
			meta->setCString(kKeyMIMEType, MEDIA_MIMETYPE_AUDIO_AAC);
    		meta->setInt32(kKeySampleRate, sampleRate);
        	meta->setInt32(kKeyChannelCount, channelCount);
    		meta->setInt64(kKeyDuration, durationTimeUs);
    		//meta->setInt32(kKeyMaxInputSize, max_audio_frame_size + 10 * 2);
			
			LOGI("Sample Rate: %d", sampleRate);//stream_info.audio_format.sample_rate);
			LOGI("Channel Count: %d", channelCount);
			LOGI("kKeyDuration: %lld", durationTimeUs);
			LOGI("Sample Size: %d", stream_info.audio_format.sample_size);
			LOGI("format_type: %d", stream_info.format_type);
			LOGI("format_size: %d", stream_info.format_size);
			LOGI("Sub Type: %d", stream_info.sub_type);
			
			//pack a pesudo descriptor header.
			uint8_t esds_size = 0x27-12;
			uint8_t* esds_data = new uint8_t[esds_size];
			memset(esds_data, 0, esds_size);

			uint8_t pos=0;
			uint8_t* destag = (uint8_t*)(esds_data+pos);
			*destag=0x3;	 // ES_DescrTag
			pos+=1;

			uint8_t* lenfield = (uint8_t*)(esds_data+pos);
			*lenfield=0x19;	 // Length field
			pos+=1;

			uint16_t* esid = (uint16_t*)(esds_data+pos);
			*esid=0x1;//es_id
			pos+=2;

			uint8_t* streamflag = (uint8_t*)(esds_data+pos);
			*streamflag=0x0;	 // OCRstreamFlag
			pos+=1;

			uint8_t* decconfig = (uint8_t*)(esds_data+pos);
			*decconfig=0x4;	 // DecoderConfigDescriptor 
			pos+=1;

			uint8_t* lenfield2 = (uint8_t*)(esds_data+pos);
			*lenfield2=0x11;	 // Length Field 
			pos+=1;

			uint8_t* objecttypeindication = (uint8_t*)(esds_data+pos);
			*objecttypeindication=0x40;	 // objectTypeIndication  
			pos+=1;

			uint8_t* streamtype = (uint8_t*)(esds_data+pos);
			*streamtype=0x15;	 // streamType    
			pos+=1;
			
			pos+=1; //0
			uint16_t* bufferSizeDB = (uint16_t*)(esds_data+pos);
			*bufferSizeDB=htonl(0x1f8);	 //bufferSizeDB       
			pos+=2;

			uint32_t* maxBitrate = (uint32_t*)(esds_data+pos);
			*maxBitrate=htonl(0x8728);	 //maxBitrate         
			pos+=4;

			uint32_t* avgBitrate = (uint32_t*)(esds_data+pos);
			*avgBitrate=htonl(0x73e8);	 //avgBitrate           
			pos+=4;

			uint8_t* decSpecificInfotag = (uint8_t*)(esds_data+pos);
			*decSpecificInfotag=0x5;	 // DecSpecificInfotag    
			pos+=1;

			uint8_t* lengthfield3 = (uint8_t*)(esds_data+pos);
			*lengthfield3=0x2;	 // lenght field    
			pos+=1;

			uint8_t* setup1 = (uint8_t*)(esds_data+pos);
			*setup1=stream_info.format_buffer[0];//0x11;	 //setup       
			pos+=1;

			uint8_t* setup2 = (uint8_t*)(esds_data+pos);
			*setup2=stream_info.format_buffer[1];//0x90;	 //setup       
			pos+=1;
			//for(int len=0;len<stream_info.format_size;len++)
			//{
			//	LOGE("audio setup data:[%d]",stream_info.format_buffer[len]);
			//}
		
			uint8_t* slConfigDescrTag = (uint8_t*)(esds_data+pos);
			*slConfigDescrTag=0x6;	 // SLConfigDescrTag
			pos+=1;

			uint8_t* lengthfield4 = (uint8_t*)(esds_data+pos);
			*lengthfield4=0x1;	 // lenght field    
			pos+=1;

			uint8_t* predefined  = (uint8_t*)(esds_data+pos);
			*predefined =0x2;	 // predefined 
			pos+=1;

			meta->setData(kKeyESDS, kTypeESDS, esds_data, esds_size);

			delete esds_data; 			
			
		}
		else if(stream_info.sub_type == ppbox_audio_mp3)
		{
			LOGI("audio sub_type is mp3, not supported currently");
    			//todo
		}
		else if(stream_info.sub_type == ppbox_audio_wma)
		{
			LOGI("audio sub_type is wma, not supported currently");
    			//todo
		}
		else
		{
			LOGI("audio sub_type is unkown, not supported");
			//todo
		}
	}
	else
	{
		//todo.
	}

    return meta;
}

sp<MetaData> PPExtractor::getMetaData() {
	LOGD("PPExtractor getMetaData");
	sp<MetaData> meta = new MetaData;
	meta->setCString(kKeyMIMEType, MEDIA_MIMETYPE_CONTAINER_PP);

	return meta;
}

status_t PPExtractor::readVideoSample(MediaBuffer **out)
{
    status_t ret = ERROR_IO;
	//LOGE("readVideoSample 1");
    if(mVideoBufferGroup)
    {
	//LOGE("readVideoSample 1.1");
		ret = mVideoBufferGroup->pop_buffer(out);
	//LOGE("readVideoSample 1.2");
    }
	return ret;
}

status_t PPExtractor::readAudioSample(MediaBuffer **out)
{
    status_t ret = ERROR_IO;
	//LOGE("readAudioSample 1");
    if(mAudioBufferGroup)
    {
	//LOGE("readAudioSample 1");
		ret = mAudioBufferGroup->pop_buffer(out);
	//LOGE("readAudioSample 1");
    }
	return ret;
}

// static
void *PPExtractor::ThreadWrapper(void *me)
{
    static_cast<PPExtractor *>(me)->threadEntry();
    return NULL;
}

void PPExtractor::threadEntry()
{
	while(mStarted)
	{
		if(isSeeking)
		{
			//seeking: step2
			Mutex::Autolock autoLock(mLock);
		    LOGD("Start seekto: %lld", mSeekTimeUs);
		    PP_uint32 start_time = mSeekTimeUs / 1000;

			if(start_time >= mDurationTimeUs)
	    	{
				LOGD("Seek to stream end");
				if(mVideoBufferGroup)
				{
					mVideoBufferGroup->notify_streamdone();
				}
				if(mAudioBufferGroup)
				{
					mAudioBufferGroup->notify_streamdone();
				}
				isStreamDone=true;
	    	}
			else
			{
			    //PP_int32 ec = PPBOX_Seek(start_time);
			    int64_t ec = ((PPBoxHandle*)gPlatformInfo->ppbox)->seek(start_time);
			    if (ppbox_success == ec || ppbox_would_block == ec) {
			        LOGD("seekto success");
			    } else {
			        //LOGE("seekto failed with error: %s", PPBOX_GetLastErrorMsg());
			        LOGE("seekto failed with error: %s", ((PPBoxHandle*)gPlatformInfo->ppbox)->getLastErrorMsg());
			    }
				LOGD("End seekto");
				isStreamDone=false;
			}
			
			LOGD("Start clean cache");
		    if(mVideoBufferGroup)
		    {
				mVideoBufferGroup->clear_buffer(mSeekTimeUs);
				mVideoBufferGroup->notify_seekdone();
		    }
		    if(mAudioBufferGroup)
		    {
				mAudioBufferGroup->clear_buffer(mSeekTimeUs);
				mAudioBufferGroup->notify_seekdone();
		    }
			LOGD("End clean cache");
		
			isSeeking=false;
			mPauseCondition.signal();
		}

		if(isStreamDone)
		{
			usleep(100000);//0.1sec
			LOGD("stream is done, waiting ...");
			continue;
			
		}
		
		LOGD("=========> calling PPBOX_ReadSampleEx2");
		PPBOX_SampleEx2 sample;
		//LOGE("Begin readsample");
		//PP_int32 ec = PPBOX_ReadSampleEx2(&sample);
		PP_int32 ec = ((PPBoxHandle*)gPlatformInfo->ppbox)->readSampleEx2(&sample);
		//LOGE("End readsample");
		if (ec == ppbox_success && sample.stream_index==mVideoTrackID)
		{
			LOGV("start add video sample, sample.is_sync:%d, kKeyTime: %lld, sample.buffer_length:%d", sample.is_sync, sample.start_time, sample.buffer_length);
			mLastSampleTime = sample.start_time;
			//isBlocking = false;
			uint8_t* packet = new uint8_t[sample.buffer_length+100];//100 to avoid 2 bytes nalsize case.
			if(packet != NULL)
			{
				size_t length = constructNalus(packet, sample.buffer, sample.buffer_length);
				LOGV("sonstructed nalu length:%d", length);
				if(length > 0)
				{
					if(mVideoBufferGroup)
					{
						mVideoBufferGroup->push_buffer(packet, length, sample.start_time, sample.is_sync);
						//LOGE("######## adding video sample");
					}
				}
				delete packet;
			}
			//LOGE("######## end add video sample");
			
			/*
			LOGE("######## start acquire video buffer");
			status_t ret = ERROR_IO;
			status_t err = mVideoBufferGroup->acquire_buffer(&packet, sample.buffer_length);
			if (err != OK) {
				if(packet!=NULL)
				{
					packet->release();
					packet = NULL;
				}
				continue;
			}

			if(packet!=NULL)
			{
				size_t length = constructNalus(packet, sample.buffer, sample.buffer_length);
				if(length > 0)
				{
				    packet->set_range(0, length);
					
					packet->meta_data()->clear();
					//int64_t frameTime = sample.start_time-mSeekTimeUs;
					packet->meta_data()->setInt64(kKeyTime, sample.start_time);
					LOGE("video sample kKeyTime: %lld", sample.start_time);
					ret = OK;
					mVideoBufferGroup->push_buffer(packet);
					//LOGE("PPMediaSource got video frame from track: %d from thread: %d", mVideoTrackID, getThreadId());

					//LOGE("xxxxxxxx video sample length: %d", length);
					//LOGE("xxxxxxxx video sample kKeyTime: %d", sample.start_time);

				}
			}
			if(ret!=OK)
			{
				if (packet != NULL) {
					packet->release();
					packet = NULL;
				}
			}
			*/
		}
		else if (ec == ppbox_success && sample.stream_index==mAudioTrackID)
		{
			//LOGE("######## start add audio sample, kKeyTime: %lld", sample.start_time);
			if(mAudioBufferGroup)
			{
				mAudioBufferGroup->push_buffer((uint8_t*)sample.buffer, sample.buffer_length, sample.start_time, sample.is_sync);
				//LOGE("######## adding audio sample");
			}

			//LOGE("######## end add audio sample");
			/*
			status_t ret = ERROR_IO;
			MediaBuffer *packet = NULL;
			LOGE("######## start acquire audio buffer");
			status_t err = mAudioBufferGroup->acquire_buffer(&packet, sample.buffer_length);
			if (err != OK) {
				if(packet!=NULL)
				{
					packet->release();
					packet = NULL;
				}
				continue;
			}
			
			if(packet!=NULL)
			{
				memcpy(packet->data(), sample.buffer, sample.buffer_length);
			    packet->set_range(0, sample.buffer_length);
				packet->meta_data()->clear();
				packet->meta_data()->setInt64(kKeyTime, sample.start_time);
				LOGE("audio sample kKeyTime: %lld", sample.start_time);
				ret = OK;
				mAudioBufferGroup->push_buffer(packet);
					//LOGE("PPMediaSource got audio frame from track: %d from thread: %d", mAudioTrackID, getThreadId());
					//LOGE("xxxxxxxx audio sample length: %d", sample.buffer_length);
					//LOGE("xxxxxxxx audio sample kKeyTime: %d", sample.start_time);
			}
			
			if(ret!=OK)
			{
				if (packet != NULL) {
					packet->release();
					packet = NULL;
				}
			}
			*/
		}
		else if (ec == ppbox_would_block)
		{
			LOGD("block getting data from track: %d", sample.stream_index);
			/*
			static uint32_t blockedCount=0;
			if(isBlocking)
				blockedCount++;
			else
				blockedCount=0;
			if(blockedCount>150)//15sec
			{
				isSeeking=true;
				mSeekTimeUs=mLastSampleTime+6*1000000;
				LOGE("block time reaches 15sec, now try seek to %lld", mSeekTimeUs);
				blockedCount=0;
			}
			else
			{
				usleep(100000);//0.1sec
			}
			isBlocking=true;
			*/
			usleep(100000);//0.1sec
			continue;

		}
		else if (ec == ppbox_stream_end)
		{
			LOGD("reach stream end");
			//PPBOX_Close();
			LOGD("ppbox_stream_end 1");
			if(mVideoBufferGroup)
			{
				mVideoBufferGroup->notify_streamdone();
				LOGD("ppbox_stream_end 1.1");
			}
			if(mAudioBufferGroup)
			{
				mAudioBufferGroup->notify_streamdone();
				LOGD("ppbox_stream_end 1.2");
			}
				LOGD("ppbox_stream_end 2");
			isStreamDone=true;
			continue;
			//todo:
		} 
		else
		{
			//LOGE("PPbox engine Unknow error: %d, %s",ec, PPBOX_GetLastErrorMsg());
			LOGE("PPbox engine Unknow error: %d, %s", ec, ((PPBoxHandle*)gPlatformInfo->ppbox)->getLastErrorMsg);
			if(mVideoBufferGroup)
			{
				mVideoBufferGroup->notify_streamdone();
			}
			if(mAudioBufferGroup)
			{
				mAudioBufferGroup->notify_streamdone();
			}
			isStreamDone=true;
			continue;
			//sleep(1);
			//break;
		}
	}

	mStarted=false;
}

status_t PPExtractor::start(MetaData *params) {
	LOGD("PPExtractor start");
	Mutex::Autolock autoLock(mLock);
	
	if (mStarted) {
		return OK;
	}
	if(mTrackCount>0)
	{		
		for(size_t i=0;i<mTrackCount;i++)
		{
			if(i==mVideoTrackID)//video
			{
				if(mVideoBufferGroup)
			    {
					//mVideoBufferGroup->clear_all_buffer();
					delete mVideoBufferGroup;
					mVideoBufferGroup=NULL;
			    }
				mVideoBufferGroup=new PPMediaBufferGroup(video_buffer_size, video_buffer_play_count);
			}
			else if(i==mAudioTrackID)//audio
			{
				if(mAudioBufferGroup)
			    {
					//mAudioBufferGroup->clear_all_buffer();
					delete mAudioBufferGroup;
					mAudioBufferGroup=NULL;
			    }
				mAudioBufferGroup=new PPMediaBufferGroup(audio_buffer_size, audio_buffer_play_count);
			}
			else //todo: like handle subtitle.
			{
			}
		}

		mStarted = true;
		//start thread to read data from ppbox.
		pthread_attr_t attr;
		pthread_attr_init(&attr);
		pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
		pthread_create(&mThread, &attr, ThreadWrapper, this);
		LOGD("start to running ppbox reading data thread");

		pthread_attr_destroy(&attr);
	}

	return OK;
}


void PPExtractor::stop()
{
	//Mutex::Autolock autoLock(mLock);
    if (!mStarted) {
        return;
    }
    
    LOGD("++++++++Start stopping, this: %p, thread id: %p", this, getThreadId());
    mStarted = false;
	
	if(mVideoBufferGroup)
    {
		mVideoBufferGroup->notify_stop();
    }
    if(mAudioBufferGroup)
    {
		mAudioBufferGroup->notify_stop();
	}

    void *dummy;
    pthread_join(mThread, &dummy);
	
	if(mVideoBufferGroup)
    {
		//mVideoBufferGroup->clear_all_buffer();
		//mVideoBufferGroup->notify_streamdone();
		delete mVideoBufferGroup;
		mVideoBufferGroup=NULL;
    }
    if(mAudioBufferGroup)
    {
		//mAudioBufferGroup->clear_all_buffer();
		//mAudioBufferGroup->notify_streamdone();
		delete mAudioBufferGroup;
		mAudioBufferGroup=NULL;
    }
	
    LOGD("++++++++End stopping, this: %p, thread id: %p", this, getThreadId());
	
}

void PPExtractor::seekTo(int64_t seekTimeUs)
{
	//int64_t delta = seekTimeUs-mSeekTimeUs;
	//delta = (delta>=0)?delta:(-delta);

	//if(delta > 2000000)//2sec
	//{
		LOGD("Assigned seeking task");
		mSeekTimeUs=seekTimeUs;
		{
			//seeking: step1
			Mutex::Autolock autoLock(mLock);
		    isSeeking=true;
			
			if(mVideoBufferGroup)
		    {
				mVideoBufferGroup->notify_seekstart();
		    }
		    if(mAudioBufferGroup)
		    {
				mAudioBufferGroup->notify_seekstart();
			}
	
		    mPauseCondition.wait(mLock);
		}
		LOGD("Executed seeking task");
	//}
}

int64_t PPExtractor::getCachedDurationUs() //us
{
	int64_t duration = 0;
	if(mVideoBufferGroup)
    {
		duration = mVideoBufferGroup->getCachedDurationUs();
		LOGD("getCachedDurationUs %lld", duration);
    }
	return duration;
}

size_t PPExtractor::parseNALSize(const uint8_t *data) const {
    switch (mNALLengthSize) {
        case 1:
            return *data;
        case 2:
            return U16_AT(data);
        case 3:
            return ((size_t)data[0] << 16) | U16_AT(&data[1]);
        case 4:
            return U32_AT(data);
    }

    // This cannot happen, mNALLengthSize springs to life by adding 1 to
    // a 2-bit integer.
    CHECK(!"Should not be here.");

    return 0;
}

size_t PPExtractor::constructNalus(void* dst, const void* src, size_t size) {
	uint8_t *dstData = (uint8_t *)dst;
	const uint8_t *srcData = (const uint8_t *)src;
	size_t srcOffset = 0;
	size_t dstOffset = 0;

	size_t totalWrittenLen = 0;
    bool start_frame = true;
	while (srcOffset < size)
	{
		CHECK(srcOffset + mNALLengthSize <= size);
		size_t nalLength = parseNALSize(&srcData[srcOffset]);
		srcOffset += mNALLengthSize;

		if (srcOffset + nalLength > size) {
			return -1;
		}

		if (nalLength == 0) {
			continue;
		}
		if(mDurationTimeUs==0) //broadcast
		{
		    uint8_t naluType = (srcData[srcOffset] & 31);
			if (naluType==6 || 
				naluType==7 ||
				naluType==8 ||
				naluType==9)
			{
				srcOffset += nalLength;
				LOGV("NALU:Ignore nalu type:%d", naluType);
				continue;
			}
		}

		if(start_frame)
		{
			CHECK(dstOffset + 4 <= size);

			dstData[dstOffset++] = 0;
			dstData[dstOffset++] = 0;
			dstData[dstOffset++] = 0;
			dstData[dstOffset++] = 1;
			start_frame = false;
			totalWrittenLen+=4;
			//LOGE("NALU: At start frame, nal size is:%d, forbidden_zero_bit:%d, nal_ref_idc:%d, nal_unit_type:%d",
			//	nalLength,
			//	(srcData[srcOffset]>>7) & 1,
			//	(srcData[srcOffset]>>5) & 3,
			//	srcData[srcOffset] & 31);
		}
		else
		{
			CHECK(dstOffset + 3 <= size);

			dstData[dstOffset++] = 0;
			dstData[dstOffset++] = 0;
			dstData[dstOffset++] = 1;
			totalWrittenLen+=3;
			//LOGE("NALU: append nalu, nal size is:%d, forbidden_zero_bit:%d, nal_ref_idc:%d, nal_unit_type:%d",
			//	nalLength,
			//	(srcData[srcOffset]>>7) & 1,
			//	(srcData[srcOffset]>>5) & 3,
			//	srcData[srcOffset] & 31);
		}
		memcpy(&dstData[dstOffset], &srcData[srcOffset], nalLength);
		srcOffset += nalLength;
		dstOffset += nalLength;
		totalWrittenLen+=nalLength;
	}

	return totalWrittenLen;
}

uint32_t PPExtractor::flags() const 
{
    LOGD("flags");
	//PP_uint32 durationTimeUs = PPBOX_GetDuration();
	PP_uint32 durationTimeUs = ((PPBoxHandle*)gPlatformInfo->ppbox)->getDuration();
	if(durationTimeUs==0)
	{
	    LOGE("cannot seek");
	    return 0;
	}
	else
	{
	    LOGD("can seek");
	    return CAN_SEEK_BACKWARD | CAN_SEEK_FORWARD | CAN_PAUSE;
	}
}

bool PPExtractor::isBuffering()
{
    if(mAudioBufferGroup==NULL || mVideoBufferGroup==NULL) return false;
    
    if(mAudioBufferGroup->isBuffering() ||mVideoBufferGroup->isBuffering())
    {
        LOGD("got buffering");
        return true;
    }
    return false;
}

bool PPExtractor::reachedEndOfStream()
{
	return isStreamDone;
}

bool SniffPP(const sp<DataSource> &source, String8 *mimeType, float *confidence, sp<AMessage> *meta) 
{
	//LOGE("PPExtractor SniffPP with threadID: %d", getThreadId());
	char tmp[5];
	if (source->readAt(0, tmp, 5) < 5 || memcmp(tmp, "PPVOD", 5)) {
		return false;
	}

	mimeType->setTo(MEDIA_MIMETYPE_CONTAINER_PP);
	*confidence = 0.2f;

	return true;
}

}  // namespace android
