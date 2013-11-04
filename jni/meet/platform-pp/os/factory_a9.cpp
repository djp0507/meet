#define LOG_TAG "Factory_A9"

#include "include-pp/IPPAudioTrack.h"
#include "os/audiotrack_a9.h"
#include "device/xiaomi_mioneplus_ppAudioTrack.h"

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
    return new AudioTrack_A9(
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

extern "C" IPPAudioTrack* createAudioTrack_Xiaomi(
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
    return new Xiaomi_MIONEPlus_PPAudioTrack(
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

}
