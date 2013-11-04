#define LOG_TAG "NuPPMediaExtractor"

// Do not modfiy here
#include "include-pp/a16/MetaData.h"
#include "include-pp/sf/include/ESDS.h"

#include "include-pp/a16/NuPPMediaExtractor.h"
#include "include-pp/a16/ABuffer.h"
#include "include-pp/a16/AMessage.h"

#include "include-pp/ppDataSource.h"
#include "include-pp/ppExtractor.h"

#include "include-pp/Utils.h"

namespace android {

status_t convertMetaDataToMessage(
		const sp<MetaData> &meta, sp<AMessage> *format) {
	format->clear();

	const char *mime;
	CHECK(meta->findCString(kKeyMIMEType, &mime));

	sp<AMessage> msg = new AMessage;
	msg->setString("mime", mime);

	int64_t durationUs;
	if (meta->findInt64(kKeyDuration, &durationUs)) {
		msg->setInt64("durationUs", durationUs);
	}

	if (!strncasecmp("video/", mime, 6)) {
		int32_t width, height;
		CHECK(meta->findInt32(kKeyWidth, &width));
		CHECK(meta->findInt32(kKeyHeight, &height));

		msg->setInt32("width", width);
		msg->setInt32("height", height);
	} else if (!strncasecmp("audio/", mime, 6)) {
		int32_t numChannels, sampleRate;
		CHECK(meta->findInt32(kKeyChannelCount, &numChannels));
		CHECK(meta->findInt32(kKeySampleRate, &sampleRate));

		msg->setInt32("channel-count", numChannels);
		msg->setInt32("sample-rate", sampleRate);

		int32_t channelMask;
		if (meta->findInt32(kKeyChannelMask, &channelMask)) {
			msg->setInt32("channel-mask", channelMask);
		}

		int32_t delay = 0;
		if (meta->findInt32(kKeyEncoderDelay, &delay)) {
			msg->setInt32("encoder-delay", delay);
		}
		int32_t padding = 0;
		if (meta->findInt32(kKeyEncoderPadding, &padding)) {
			msg->setInt32("encoder-padding", padding);
		}

		int32_t isADTS;
		if (meta->findInt32(kKeyIsADTS, &isADTS)) {
			msg->setInt32("is-adts", true);
		}
	}

	int32_t maxInputSize;
	if (meta->findInt32(kKeyMaxInputSize, &maxInputSize)) {
		msg->setInt32("max-input-size", maxInputSize);
	}

	uint32_t type;
	const void *data;
	size_t size;
	if (meta->findData(kKeyAVCC, &type, &data, &size)) {
		// Parse the AVCDecoderConfigurationRecord

		const uint8_t *ptr = (const uint8_t *)data;

		CHECK(size >= 7);
		CHECK_EQ((unsigned)ptr[0], 1u);  // configurationVersion == 1
		uint8_t profile = ptr[1];
		uint8_t level = ptr[3];

		// There is decodable content out there that fails the following
		// assertion, let's be lenient for now...
		// CHECK((ptr[4] >> 2) == 0x3f);  // reserved

		size_t lengthSize = 1 + (ptr[4] & 3);

		// commented out check below as H264_QVGA_500_NO_AUDIO.3gp
		// violates it...
		// CHECK((ptr[5] >> 5) == 7);  // reserved

		size_t numSeqParameterSets = ptr[5] & 31;

		ptr += 6;
		size -= 6;

		sp<ABuffer> buffer = new ABuffer(1024);
		buffer->setRange(0, 0);

		for (size_t i = 0; i < numSeqParameterSets; ++i) {
			CHECK(size >= 2);
			size_t length = U16_AT(ptr);

			ptr += 2;
			size -= 2;

			CHECK(size >= length);

			memcpy(buffer->data() + buffer->size(), "\x00\x00\x00\x01", 4);
			memcpy(buffer->data() + buffer->size() + 4, ptr, length);
			buffer->setRange(0, buffer->size() + 4 + length);

			ptr += length;
			size -= length;
		}

		buffer->meta()->setInt32("csd", true);
		buffer->meta()->setInt64("timeUs", 0);

		msg->setBuffer("csd-0", buffer);

		buffer = new ABuffer(1024);
		buffer->setRange(0, 0);

		CHECK(size >= 1);
		size_t numPictureParameterSets = *ptr;
		++ptr;
		--size;

		for (size_t i = 0; i < numPictureParameterSets; ++i) {
			CHECK(size >= 2);
			size_t length = U16_AT(ptr);

			ptr += 2;
			size -= 2;

			CHECK(size >= length);

			memcpy(buffer->data() + buffer->size(), "\x00\x00\x00\x01", 4);
			memcpy(buffer->data() + buffer->size() + 4, ptr, length);
			buffer->setRange(0, buffer->size() + 4 + length);

			ptr += length;
			size -= length;
		}

		buffer->meta()->setInt32("csd", true);
		buffer->meta()->setInt64("timeUs", 0);
		msg->setBuffer("csd-1", buffer);
	} else if (meta->findData(kKeyESDS, &type, &data, &size)) {
		ESDS esds((const char *)data, size);
		CHECK_EQ(esds.InitCheck(), (status_t)OK);

		const void *codec_specific_data;
		size_t codec_specific_data_size;
		esds.getCodecSpecificInfo(
				&codec_specific_data, &codec_specific_data_size);

		sp<ABuffer> buffer = new ABuffer(codec_specific_data_size);

		memcpy(buffer->data(), codec_specific_data,
				codec_specific_data_size);

		buffer->meta()->setInt32("csd", true);
		buffer->meta()->setInt64("timeUs", 0);
		msg->setBuffer("csd-0", buffer);
	} else if (meta->findData(kKeyVorbisInfo, &type, &data, &size)) {
		sp<ABuffer> buffer = new ABuffer(size);
		memcpy(buffer->data(), data, size);

		buffer->meta()->setInt32("csd", true);
		buffer->meta()->setInt64("timeUs", 0);
		msg->setBuffer("csd-0", buffer);

		if (!meta->findData(kKeyVorbisBooks, &type, &data, &size)) {
			return -EINVAL;
		}

		buffer = new ABuffer(size);
		memcpy(buffer->data(), data, size);

		buffer->meta()->setInt32("csd", true);
		buffer->meta()->setInt64("timeUs", 0);
		msg->setBuffer("csd-1", buffer);
	}

	*format = msg;

	return OK;
}


//////////////////////////////////////////////////////////////////

NuPPMediaExtractor::NuPPMediaExtractor()
	  : mTotalBitrate(-1ll),
		mDurationUs(-1ll){
}

NuPPMediaExtractor::~NuPPMediaExtractor() {
	releaseTrackSamples();

	for (size_t i = 0; i < mSelectedTracks.size(); ++i) {
		TrackInfo *info = &mSelectedTracks.editItemAt(i);

		CHECK_EQ((status_t)OK, info->mSource->stop());
	}

	mSelectedTracks.clear();

	if (mImpl != NULL) {
		mImpl->stop();
	}
}

status_t NuPPMediaExtractor::setDataSource(
	const char * path, 
	const KeyedVector < String8,String8 > * headers) {
	Mutex::Autolock autoLock(mLock);

	if (mImpl != NULL) {
		return -EINVAL;
	}

	sp<PPDataSource> dataSource = new PPDataSource;

	status_t err = dataSource->openStream(path);

	if (err != OK) {
		return -ENOENT;
	}

	mImpl = new PPExtractor(dataSource);
	mImpl->start();

	mDataSource = dataSource;

	updateDurationAndBitrate();
	
	return OK;
}

void NuPPMediaExtractor::updateDurationAndBitrate() {
	mTotalBitrate = 0ll;
	mDurationUs = -1ll;

	for (size_t i = 0; i < mImpl->countTracks(); ++i) {
		sp<MetaData> meta = mImpl->getTrackMetaData(i);

		int32_t bitrate;
		if (!meta->findInt32(kKeyBitRate, &bitrate)) {
			const char *mime;
			CHECK(meta->findCString(kKeyMIMEType, &mime));
			LOGV("track of type '%s' does not publish bitrate", mime);

			mTotalBitrate = -1ll;
		} else if (mTotalBitrate >= 0ll) {
			mTotalBitrate += bitrate;
		}

		int64_t durationUs;
		if (meta->findInt64(kKeyDuration, &durationUs)
				&& durationUs > mDurationUs) {
			mDurationUs = durationUs;
		}
	}
}


size_t NuPPMediaExtractor::countTracks() const {
	Mutex::Autolock autoLock(mLock);

	return mImpl == NULL ? 0 : mImpl->countTracks();
}

status_t NuPPMediaExtractor::getTrackFormat(
	size_t index, sp <AMessage> *format) const {
	Mutex::Autolock autoLock(mLock);

	if (mImpl == NULL) {
		return -EINVAL;
	}

	if (index >= mImpl->countTracks()) {
		return -ERANGE;
	}

	sp<MetaData> meta = mImpl->getTrackMetaData(index);

	return convertMetaDataToMessage(meta, format);
}

status_t NuPPMediaExtractor::selectTrack(size_t index) {
	Mutex::Autolock autoLock(mLock);

	if (mImpl == NULL) {
		return -EINVAL;
	}

	if (index >= mImpl->countTracks()) {
		return -ERANGE;
	}

	for (size_t i = 0; i < mSelectedTracks.size(); ++i) {
		TrackInfo *info = &mSelectedTracks.editItemAt(i);

		if (info->mTrackIndex == index) {
			// This track has already been selected.
			return OK;
		}
	}

	sp<MediaSource> source = mImpl->getTrack(index);

	CHECK_EQ((status_t)OK, source->start());

	mSelectedTracks.push();
	TrackInfo *info = &mSelectedTracks.editItemAt(mSelectedTracks.size() - 1);

	info->mSource = source;
	info->mTrackIndex = index;
	info->mFinalResult = OK;
	info->mSample = NULL;
	info->mSampleTimeUs = -1ll;
	info->mTrackFlags = 0;

	sp<MetaData> meta =	source->getFormat();
	
	const char* mime;
	CHECK(meta->findCString(kKeyMIMEType, &mime));

	if (!strcasecmp(mime, MEDIA_MIMETYPE_AUDIO_AAC)) {
		mAudioTrackIndex = index;
	} else if (!strcasecmp(mime, MEDIA_MIMETYPE_VIDEO_AVC)) {
		mVideoTrackIndex = index;
	} else {

	}

	return OK;
}

void NuPPMediaExtractor::releaseTrackSamples() {
	for (size_t i = 0; i < mSelectedTracks.size(); ++i) {
		TrackInfo *info = &mSelectedTracks.editItemAt(i);

		if (info->mSample != NULL) {
			info->mSample->release();
			info->mSample = NULL;

			info->mSampleTimeUs = -1ll;
		}
	}
}


status_t NuPPMediaExtractor::unselectTrack(size_t index) {
	Mutex::Autolock autoLock(mLock);

	if (mImpl == NULL) {
		return -EINVAL;
	}

	if (index >= mImpl->countTracks()) {
		return -ERANGE;
	}

	size_t i;
	for (i = 0; i < mSelectedTracks.size(); ++i) {
		TrackInfo *info = &mSelectedTracks.editItemAt(i);

		if (info->mTrackIndex == index) {
			break;
		}
	}

	if (i == mSelectedTracks.size()) {
	// Not selected.
		return OK;
	}

	TrackInfo *info = &mSelectedTracks.editItemAt(i);

	if (info->mSample != NULL) {
		info->mSample->release();
		info->mSample = NULL;

		info->mSampleTimeUs = -1ll;
	}

	CHECK_EQ((status_t)OK, info->mSource->stop());

	mSelectedTracks.removeAt(i);

	return OK;

}


ssize_t NuPPMediaExtractor::fetchTrackSamples(
	int64_t seekTimeUs, MediaSource::ReadOptions::SeekMode mode) {
	TrackInfo *minInfo = NULL;
	ssize_t minIndex = -1;

	for (size_t i = 0; i < mSelectedTracks.size(); ++i) {
		TrackInfo *info = &mSelectedTracks.editItemAt(i);

		if (seekTimeUs >= 0ll) {
			info->mFinalResult = OK;

			if (info->mSample != NULL) {
				info->mSample->release();
				info->mSample = NULL;
				info->mSampleTimeUs = -1ll;
			}
		} else if (info->mFinalResult != OK) {
			continue;
		}

		if (info->mSample == NULL) {
			MediaSource::ReadOptions options;
			if (seekTimeUs >= 0ll) {
				options.setSeekTo(seekTimeUs, mode);
			}

			// as video/audio share one ppextractor instance. so only 
			// have video media source to do seek which will cause ppextractor to do seek.
			if (seekTimeUs >= 0ll/* && info->mTrackIndex != mVideoTrackIndex*/) {
				continue;
			}
			
			status_t err = info->mSource->read(&info->mSample, &options);

			if (err != OK) {
				CHECK(info->mSample == NULL);
			
				info->mFinalResult = err;
			
				if (info->mFinalResult != ERROR_END_OF_STREAM) {
					LOGW("read on track %d failed with error %d",
						  info->mTrackIndex, err);
				}
			
				info->mSampleTimeUs = -1ll;
				continue;
			} else {
				CHECK(info->mSample != NULL);
				CHECK(info->mSample->meta_data()->findInt64(
							kKeyTime, &info->mSampleTimeUs));
			}

		}

		if (minInfo == NULL  || info->mSampleTimeUs < minInfo->mSampleTimeUs) {
			minInfo = info;
			minIndex = i;
		}

	}

	return minIndex;
}

status_t NuPPMediaExtractor::seekTo(
	int64_t timeUs, MediaSource::ReadOptions::SeekMode mode) {
	Mutex::Autolock autoLock(mLock);

	ssize_t minIndex = fetchTrackSamples(timeUs, mode);
	mImpl->seekTo(timeUs);

	/*
	if (minIndex < 0) {
		return ERROR_END_OF_STREAM;
	}
	*/

	return OK;
}

status_t NuPPMediaExtractor::advance() {
	Mutex::Autolock autoLock(mLock);

	ssize_t minIndex = fetchTrackSamples();

	if (minIndex < 0) {
		return ERROR_END_OF_STREAM;
	}

	TrackInfo *info = &mSelectedTracks.editItemAt(minIndex);

	info->mSample->release();
	info->mSample = NULL;
	info->mSampleTimeUs = -1ll;

	return OK;
}

status_t NuPPMediaExtractor::readSampleData(const sp < ABuffer > & buffer) {
	Mutex::Autolock autoLock(mLock);

	ssize_t minIndex = fetchTrackSamples();

	if (minIndex < 0) {
		return ERROR_END_OF_STREAM;
	}

	TrackInfo *info = &mSelectedTracks.editItemAt(minIndex);

	size_t sampleSize = info->mSample->range_length();

	/*
	if (info->mTrackFlags & kIsVorbis) {
		// Each sample's data is suffixed by the number of page samples
		// or -1 if not available.
		sampleSize += sizeof(int32_t);
	}
	*/

	if (buffer->capacity() < sampleSize) {
		return -ENOMEM;
	}

	const uint8_t *src =
		(const uint8_t *)info->mSample->data()
			+ info->mSample->range_offset();

	memcpy((uint8_t *)buffer->data(), src, info->mSample->range_length());

	/*
	if (info->mTrackFlags & kIsVorbis) {
		int32_t numPageSamples;
		if (!info->mSample->meta_data()->findInt32(
					kKeyValidSamples, &numPageSamples)) {
			numPageSamples = -1;
		}

		memcpy((uint8_t *)buffer->data() + info->mSample->range_length(),
				&numPageSamples,
				sizeof(numPageSamples));
	}
	*/
	buffer->setRange(0, sampleSize);

	return OK;
}

status_t NuPPMediaExtractor::getSampleTrackIndex(size_t *trackIndex) {
	Mutex::Autolock autoLock(mLock);

	ssize_t minIndex = fetchTrackSamples();

	if (minIndex < 0) {
		return ERROR_END_OF_STREAM;
	}

	TrackInfo *info = &mSelectedTracks.editItemAt(minIndex);
	*trackIndex = info->mTrackIndex;

	return OK;
}

status_t NuPPMediaExtractor::getSampleTime(int64_t *sampleTimeUs) {
	Mutex::Autolock autoLock(mLock);

	ssize_t minIndex = fetchTrackSamples();

	if (minIndex < 0) {
		return ERROR_END_OF_STREAM;
	}

	TrackInfo *info = &mSelectedTracks.editItemAt(minIndex);
	*sampleTimeUs = info->mSampleTimeUs;

	return OK;
}

status_t NuPPMediaExtractor::getSampleMeta(sp < MetaData > * sampleMeta) {
	Mutex::Autolock autoLock(mLock);

	*sampleMeta = NULL;

	ssize_t minIndex = fetchTrackSamples();

	if (minIndex < 0) {
		return ERROR_END_OF_STREAM;
	}

	TrackInfo *info = &mSelectedTracks.editItemAt(minIndex);
	*sampleMeta = info->mSample->meta_data();

	return OK;
}

bool NuPPMediaExtractor::getTotalBitrate(int64_t *bitrate) const {
	if (mTotalBitrate >= 0) {
		*bitrate = mTotalBitrate;
		return true;
	}

	// off64_t size;
	off_t size;
	if (mDurationUs >= 0 && mDataSource->getSize(&size) == OK) {
		*bitrate = size * 8000000ll / mDurationUs;  // in bits/sec
		return true;
	}

	return false;
}

// Returns true if cached duration is available/applicable.
bool NuPPMediaExtractor::getCachedDuration(
	int64_t *durationUs, bool *eos) const {
	Mutex::Autolock autoLock(mLock);

	if (mImpl->isBuffering()) {
		*durationUs = -1ll; // microseconds
		*eos = mImpl->reachedEndOfStream();
//		LOGE("isBuffering!!!");
	} else {
		*durationUs = mImpl->getCachedDurationUs(); // microseconds
		*eos = mImpl->reachedEndOfStream();
//		LOGE("getCachedDuration: %d", *durationUs);
	}
	
	return true;
}

}
