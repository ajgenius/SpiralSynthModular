/*
 * Legacy Enlightened Sound Daemon (ESD) client for SpiralSynthModular.
 *
 * ESD is retained only as an optional compatibility backend.  The public
 * interface mirrors the other OutputPlugin clients so it can be selected at
 * runtime when configure detects libesd.
 */
#ifndef __ESDClient_H__
#define __ESDClient_H__

#ifdef OUTPUT_BACKEND_ESD

#include <string>
#include <esd.h>

#include "Port.h"
#include "Device.h"
#include "Patch.h"
#include "RiffWav.h"

using namespace Spiral;

class ESDClient
{
public:
    static ESDClient *Get()
    {
        if (!m_Singleton) m_Singleton = new ESDClient;
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

    ~ESDClient();

    void AllocateBuffer();
    void DeallocateBuffer();
    void SendStereo(InputPort *ldata, InputPort *rdata);
    void GetStereo(OutputPort *ldata, OutputPort *rdata);

    void SetVolume(FloatType s) { m_Amp = s; }
    void SetNumChannels(int s) { m_Channels = s; }
    FloatType GetVolume() { return m_Amp; }
    void SetDestination(const std::string &s) { m_Destination = s; }

    void Play();
    void Read();

    bool OpenReadWrite();
    bool OpenWrite();
    bool OpenRead();
    bool Close();

    void Kill()
    {
        m_IsDead = true;
        PackUpAndGoHome();
    }

    short *GetBuffer() { return m_Buffer[m_WriteBufferNum]; }

    /* Keep the same contract as OSS/ALSA/PortAudio: engine values. */
    inline UnsignedType SampleRate() { return host->SampleRate(); }
    inline UnsignedType SampleCount() { return host->SampleCount(); }

    static Patch *host;
    static void (*RunCallback)(void*, bool);
    static void *RunContext;

private:
    ESDClient();
    ESDClient(const ESDClient &);
    ESDClient &operator=(const ESDClient &);

    bool Open(bool playback, bool capture);
    bool WriteAll(const void *data, std::size_t bytes);
    bool ReadAll(void *data, std::size_t bytes);
    const char *ServerName() const;

    static ESDClient *m_Singleton;

    int m_PlaybackHandle;
    int m_CaptureHandle;
    bool m_HasPlayback;
    bool m_HasCapture;

    short *m_Buffer[2];
    short *m_InBuffer[2];

    FloatType m_Amp;
    int m_Channels;
    int m_ReadBufferNum;
    int m_WriteBufferNum;
    bool m_IsDead;
    std::string m_Destination;
};

#endif /* OUTPUT_BACKEND_ESD */

#endif
