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

//#define LOG_NDEBUG 0
#define LOG_TAG "MediaExtractor"
#include "libppplayer/a14/MPEG4Extractor.h"
#include "libppplayer/a14/MediaExtractor.h"
#include "libppplayer/a14/MediaDefs.h"
#include "libppplayer/a14/ppExtractor.h"
#include "libppplayer/a14/ppDataSource.h"

//#include "include-pp/log.h"

//#include "include/AMRExtractor.h"
//#include "include/MP3Extractor.h"
//#include "include/WAVExtractor.h"
//#include "include/OggExtractor.h"
#include "include-pp/a14/frameworks/base/include/utils/Log.h"
#include "include-pp/a14/frameworks/base/include/media/stagefright/MetaData.h"
#include "include-pp/a14/frameworks/base/include/media/stagefright/foundation/AMessage.h"
#include "include-pp/a14/frameworks/base/include/utils/String8.h"

namespace android {

sp<MetaData> MediaExtractor::getMetaData() {
    return new MetaData;
}

uint32_t MediaExtractor::flags() const {
    return CAN_SEEK_BACKWARD | CAN_SEEK_FORWARD | CAN_PAUSE;
}

// static
sp<MediaExtractor> MediaExtractor::Create(
        const sp<DataSource> &source, const char *mime) {

    sp<AMessage> meta;
    
    String8 tmp;
    if (mime == NULL) {
        float confidence;
        if (!source->sniff(&tmp, &confidence, &meta)) {
            LOGV("FAILED to autodetect media content.");

            return NULL;
        }

        mime = tmp.string();
        LOGV("Autodetected media content as '%s' with confidence %.2f",
             mime, confidence);
    }

    if (!strcasecmp(mime, MEDIA_MIMETYPE_CONTAINER_MPEG4)
            || !strcasecmp(mime, "audio/mp4")) {
        LOGD("creating MPEG4Extractor");
        return new MPEG4Extractor(source);
    } else if (!strcasecmp(mime, MEDIA_MIMETYPE_AUDIO_MPEG)) {
//        return new MP3Extractor(source);
    } else if (!strcasecmp(mime, MEDIA_MIMETYPE_AUDIO_AMR_NB)
            || !strcasecmp(mime, MEDIA_MIMETYPE_AUDIO_AMR_WB)) {
//        return new AMRExtractor(source);
    } else if (!strcasecmp(mime, MEDIA_MIMETYPE_CONTAINER_WAV)) {
//        return new WAVExtractor(source);
    } else if (!strcasecmp(mime, MEDIA_MIMETYPE_CONTAINER_OGG)) {
//        return new OggExtractor(source);
    }else if (!strcasecmp(mime, MEDIA_MIMETYPE_CONTAINER_PP)) {
        LOGD("creating PPExtractor");
        return new PPExtractor(source);
    }

    return NULL;
}

}  // namespace android
