#define LOG_TAG "Factory_A8"

#include "include-pp/IPPAudioTrack.h"
#include "os/audiotrack_a8.h"

namespace android {


extern "C" IPPAudioTrack* createAudioTrack(
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
    return new AudioTrack_A8(
			streamType,
			sampleRate,
		    format,
	        channels,
	        frameCount,
	        flags,
	        cbf,
	        user,
	        notificationFrames);
}

}
