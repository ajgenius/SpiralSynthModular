/*
 * PipeWire audio client for Spiral.
 *
 * This preserves the old OSS/ALSA-facing block API used by the engine, while
 * using PipeWire's callback-driven realtime model internally.
 */

#ifndef __PipeWireClient_H_
#define __PipeWireClient_H_

#ifdef OUTPUT_BACKEND_PIPEWIRE

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include <pipewire/pipewire.h>
#include <spa/param/audio/format-utils.h>

#include "Port.h"
#include "Device.h"
#include "Patch.h"
#include "RiffWav.h"

using namespace Spiral;

class PipeWireClient
{
public:
    static PipeWireClient *Get()
    {
        if (!m_Singleton)
            m_Singleton = new PipeWireClient;
        return m_Singleton;
    }

    static void PackUpAndGoHome()
    {
        if (m_Singleton)
        {
            delete m_Singleton;
            m_Singleton = NULL;
        }
    }

    ~PipeWireClient();

    void AllocateBuffer();
    void DeallocateBuffer();

    void SendStereo(InputPort *ldata, InputPort *rdata);
    void GetStereo(OutputPort *ldata, OutputPort *rdata);

    void SetVolume(FloatType s) { m_Amp = s; }
    void SetNumChannels(int s)  { m_Channels = s; }
    FloatType GetVolume()       { return m_Amp; }
    void SetDestination(const std::string &s) { m_Destination = s; }

    /* Compatibility calls.  These preserve the old engine's timing model:
       Play() submits one engine block and Read() obtains one engine block. */
    void Play();
    void Read();

    bool OpenReadWrite();
    bool OpenWrite();
    bool OpenRead();
    bool Close();

    void Kill()
    {
        m_IsDead.store(true, std::memory_order_release);
        PackUpAndGoHome();
    }

    static Patch *host;
    static void (*RunCallback)(void*, bool);
    static void *RunContext;

    /* Same contract as OSSClient: engine values, not PipeWire quantum size. */
    inline UnsignedType SampleRate()  { return host->SampleRate(); }
    inline UnsignedType SampleCount() { return host->SampleCount(); }

private:
    PipeWireClient();
    PipeWireClient(const PipeWireClient &);
    PipeWireClient &operator=(const PipeWireClient &);

    class AudioRingBuffer
    {
    public:
        AudioRingBuffer();

        void Configure(std::size_t capacityFrames, unsigned channels);
        void Reset();

        std::size_t Read(float *dst, std::size_t frames);
        std::size_t Write(const float *src, std::size_t frames);

        std::size_t ReadAvailable() const;
        std::size_t WriteAvailable() const;
        std::size_t CapacityFrames() const { return m_CapacityFrames; }

    private:
        std::vector<float> m_Data;
        std::size_t m_CapacityFrames;
        unsigned m_Channels;
        std::atomic<std::uint64_t> m_ReadFrame;
        std::atomic<std::uint64_t> m_WriteFrame;
    };

    struct StreamState
    {
        StreamState();

        PipeWireClient *owner;
        pw_stream *stream;
        spa_hook listener;
        pw_stream_events events;
        bool capture;
        std::atomic<int> state;
    };

    bool Setup(bool playback, bool capture);
    bool CreateStream(StreamState &stream, bool capture);
    bool WaitForReady(StreamState &stream);
    void DestroyStream(StreamState &stream);

    static void OnStreamStateChanged(void *data,
                                     enum pw_stream_state oldState,
                                     enum pw_stream_state state,
                                     const char *error);
    static void OnStreamProcess(void *data);

    void ProcessPlayback(StreamState &stream);
    void ProcessCapture(StreamState &stream);

    bool PlaybackOperational() const;
    bool CaptureOperational() const;

    static PipeWireClient *m_Singleton;

    pw_thread_loop *m_Loop;
    pw_context *m_Context;
    pw_core *m_Core;

    StreamState m_Playback;
    StreamState m_Capture;

    float *m_Buffer[2];
    float *m_InBuffer[2];

    AudioRingBuffer m_PlaybackRing;
    AudioRingBuffer m_CaptureRing;

    FloatType m_Amp;
    int m_Channels;
    int m_ReadBufferNum;
    int m_WriteBufferNum;

    std::atomic<bool> m_IsDead;
    bool m_Connected;
    bool m_LoopStarted;

    std::atomic<std::uint64_t> m_PlaybackUnderruns;
    std::atomic<std::uint64_t> m_CaptureOverruns;
    std::string m_Destination;
};

#endif /* OUTPUT_BACKEND_PIPEWIRE */

#endif
