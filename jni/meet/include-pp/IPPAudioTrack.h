#ifndef ANDROID_IPPAUDIOTRACK_H
#define ANDROID_IPPAUDIOTRACK_H

#include <sys/types.h>
#include "include-pp/utils/Errors.h"

namespace android {

// ----------------------------------------------------------------------------

class IPPAudioTrack
{
public: 	
			IPPAudioTrack(){}
            virtual uint32_t    frameCount() const = 0;
            virtual int         channelCount() const = 0;
            virtual int         frameSize() const = 0;
            virtual status_t    getPosition(uint32_t *position) = 0;
            virtual status_t    setVolume(float left, float right) = 0;
            virtual void        start() = 0;
            virtual uint32_t     latency() const = 0;
            virtual ssize_t     write(const void* buffer, size_t size) = 0;
            virtual void        stop() = 0;
            virtual void        flush() = 0;
            virtual void        pause() = 0;
            virtual status_t    initCheck() const = 0;
			virtual ~IPPAudioTrack() {};

			
    enum channel_index {
        MONO   = 0,
        LEFT   = 0,
        RIGHT  = 1
    };

    /* Events used by AudioTrack callback function (audio_track_cblk_t).
     */
    enum event_type {
        EVENT_MORE_DATA = 0,        // Request to write more data to PCM buffer.
        EVENT_UNDERRUN = 1,         // PCM buffer underrun occured.
        EVENT_LOOP_END = 2,         // Sample loop end was reached; playback restarted from loop start if loop count was not 0.
        EVENT_MARKER = 3,           // Playback head is at the specified marker position (See setMarkerPosition()).
        EVENT_NEW_POS = 4,          // Playback head is at a new position (See setPositionUpdatePeriod()).
        EVENT_BUFFER_END = 5        // Playback head is at the end of the buffer.
    };
	
    class Buffer
    {
    public:
        enum {
            MUTE    = 0x00000001
        };
        uint32_t    flags;
        int         channelCount;
        int         format;
        size_t      frameCount;
        size_t      size;
        union {
            void*       raw;
            short*      i16;
            int8_t*     i8;
        };
    };
};

}
#endif
