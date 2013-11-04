#ifndef ANDROID_PPMEDIA_PPMEDIAEXTRACTOR_H_
#define ANDROID_PPMEDIA_PPMEDIAEXTRACTOR_H_

#include "include-pp/MediaSource.h"

#include "include-pp/utils/RefBase.h"
#include "include-pp/utils/KeyedVector.h"
#include "include-pp/utils/String8.h"

#include "jni.h"

namespace android {

struct MetaData;
struct NuPPMediaExtractor;

struct JPPMediaExtractor : public RefBase {
    JPPMediaExtractor(JNIEnv *env, jobject thiz);

    status_t setDataSource(
            const char *path,
            const KeyedVector<String8, String8> *headers);

    size_t countTracks() const;
    status_t getTrackFormat(size_t index, jobject *format) const;

    status_t selectTrack(size_t index);
    status_t unselectTrack(size_t index);

    status_t seekTo(int64_t timeUs, MediaSource::ReadOptions::SeekMode mode);
    
    status_t advance();

    status_t readSampleData(jobject byteBuf, size_t offset, size_t *sampleSize);
    status_t getSampleTrackIndex(size_t *trackIndex);
    status_t getSampleTime(int64_t *sampleTimeUs);
    status_t getSampleFlags(uint32_t *sampleFlags);
    status_t getSampleMeta(sp<MetaData> *sampleMeta);

    bool getCachedDuration(int64_t *durationUs, bool *eos) const;

protected: 
    virtual ~JPPMediaExtractor();

private: 
    jclass mClass;
    jweak mObject;
    sp<NuPPMediaExtractor> mImpl;

    JPPMediaExtractor(const JPPMediaExtractor &);
    JPPMediaExtractor &operator=(const JPPMediaExtractor &);
};

}

#endif 
