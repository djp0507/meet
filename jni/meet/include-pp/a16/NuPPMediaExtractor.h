#ifndef NU_PP_MEDIA_EXTRACTOR_H_
#define NU_PP_MEDIA_EXTRACTOR_H_

#include "include-pp/MediaSource.h"

#include "include-pp/utils/KeyedVector.h"
#include "include-pp/utils/RefBase.h"
#include "include-pp/utils/String8.h"
#include "include-pp/utils/threads.h"

namespace android {

struct ABuffer;
struct AMessage;
struct MediaSource;
struct MetaData;
struct PPDataSource;
struct PPExtractor;

struct NuPPMediaExtractor : public RefBase {
    enum SampleFlags {
        SAMPLE_FLAG_SYNC        = 1,
        SAMPLE_FLAG_ENCRYPTED   = 2,
    };

    NuPPMediaExtractor();

    status_t setDataSource(
        const char *path,
        const KeyedVector<String8, String8> *headers = NULL);

    size_t countTracks() const;
    status_t getTrackFormat(size_t index, sp<AMessage> *format) const;

    
    status_t selectTrack(size_t index);
    status_t unselectTrack(size_t index);

    status_t seekTo(
            int64_t timeUs,
            MediaSource::ReadOptions::SeekMode mode =
                MediaSource::ReadOptions::SEEK_CLOSEST_SYNC);

    status_t advance();
    status_t readSampleData(const sp<ABuffer> &buffer);
    status_t getSampleTrackIndex(size_t *trackIndex);
    status_t getSampleTime(int64_t *sampleTimeUs);
    status_t getSampleMeta(sp<MetaData> *sampleMeta);

    bool getCachedDuration(int64_t *durationUs, bool *eos) const;

protected:
    virtual ~NuPPMediaExtractor();


private:
    enum TrackFlags {
        kIsVorbis       = 1,
    };

    struct TrackInfo {
        sp<MediaSource> mSource;
        size_t mTrackIndex;
        status_t mFinalResult;
        MediaBuffer *mSample;
        int64_t mSampleTimeUs;

        uint32_t mTrackFlags;   // bitmask of "TrackFlags"
    };

    mutable Mutex mLock;

    sp<PPDataSource> mDataSource;

    sp<PPExtractor> mImpl;

    Vector<TrackInfo> mSelectedTracks;
    int64_t mTotalBitrate;
    int64_t mDurationUs;

    size_t mAudioTrackIndex;
    size_t mVideoTrackIndex;

    ssize_t fetchTrackSamples(
            int64_t seekTimeUs = -1ll,
            MediaSource::ReadOptions::SeekMode mode =
                MediaSource::ReadOptions::SEEK_CLOSEST_SYNC);

    void releaseTrackSamples();

    bool getTotalBitrate(int64_t *bitrate) const;
    void updateDurationAndBitrate();

    
    // DISALLOW_EVIL_CONSTRUCTORS
    NuPPMediaExtractor(const NuPPMediaExtractor&);
    NuPPMediaExtractor &operator=(const NuPPMediaExtractor&);
};

}

#endif
