#ifndef ANDROID_AUDIOSINK_H
#define ANDROID_AUDIOSINK_H

namespace android {
	
#define DEFAULT_AUDIOSINK_BUFFERCOUNT 4
#define DEFAULT_AUDIOSINK_BUFFERSIZE 1200
#define DEFAULT_AUDIOSINK_SAMPLERATE 44100

// AudioSink: abstraction layer for audio output
class AudioSink  : virtual public RefBase
{
public:
    // Callback returns the number of bytes actually written to the buffer.
    typedef size_t (*AudioCallback)(
            AudioSink *audioSink, void *buffer, size_t size, void *cookie);

    virtual             ~AudioSink() {}
    virtual bool        ready() const = 0; // audio output is open and ready
    virtual bool        realtime() const = 0; // audio output is real-time output
    virtual ssize_t     bufferSize() const = 0;
    virtual ssize_t     frameCount() const = 0;
    virtual ssize_t     channelCount() const = 0;
    virtual ssize_t     frameSize() const = 0;
    virtual uint32_t    latency() const = 0;
    virtual float       msecsPerFrame() const = 0;
    virtual status_t    getPosition(uint32_t *position) = 0;

    // If no callback is specified, use the "write" API below to submit
    // audio data.
    virtual status_t    open(
			            uint32_t sampleRate, 
			            int channelCount,
			            int format,
			            int bufferCount,
			            AudioCallback cb = NULL,
			            void *cookie = NULL) = 0;

    virtual void        start() = 0;
    virtual ssize_t     write(const void* buffer, size_t size) = 0;
    virtual void        stop() = 0;
    virtual void        flush() = 0;
    virtual void        pause() = 0;
    virtual void        close() = 0;
};

} // namesapce android

#endif
