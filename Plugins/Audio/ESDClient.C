#include "ESDClient.h"

#ifdef OUTPUT_BACKEND_ESD

#include <cerrno>
#include <climits>
#include <cstring>
#include <iostream>
#include <unistd.h>

using namespace std;

ESDClient *ESDClient::m_Singleton = NULL;
Patch *ESDClient::host = NULL;
void (*ESDClient::RunCallback)(void*, bool) = NULL;
void *ESDClient::RunContext = NULL;

namespace
{
    inline short ToS16(FloatType value)
    {
        if (value > (FloatType)1.0) value = (FloatType)1.0;
        if (value < (FloatType)-1.0) value = (FloatType)-1.0;
        return static_cast<short>(value * static_cast<FloatType>(SHRT_MAX));
    }
}

ESDClient::ESDClient() :
    m_PlaybackHandle(-1),
    m_CaptureHandle(-1),
    m_HasPlayback(false),
    m_HasCapture(false),
    m_Amp(0.5),
    m_Channels(2),
    m_ReadBufferNum(0),
    m_WriteBufferNum(0),
    m_IsDead(false),
    m_Destination("default")
{
    m_Buffer[0] = m_Buffer[1] = NULL;
    m_InBuffer[0] = m_InBuffer[1] = NULL;
}

ESDClient::~ESDClient()
{
    Close();
    DeallocateBuffer();
}

void ESDClient::AllocateBuffer()
{
    if (m_Buffer[0] || !host)
        return;

    const std::size_t samples =
        static_cast<std::size_t>(host->SampleCount()) *
        static_cast<std::size_t>(m_Channels);

    m_Buffer[0] = new short[samples];
    m_Buffer[1] = new short[samples];
    m_InBuffer[0] = new short[samples];
    m_InBuffer[1] = new short[samples];

    std::memset(m_Buffer[0], 0, samples * sizeof(short));
    std::memset(m_Buffer[1], 0, samples * sizeof(short));
    std::memset(m_InBuffer[0], 0, samples * sizeof(short));
    std::memset(m_InBuffer[1], 0, samples * sizeof(short));
}

void ESDClient::DeallocateBuffer()
{
    delete [] m_Buffer[0];
    delete [] m_Buffer[1];
    delete [] m_InBuffer[0];
    delete [] m_InBuffer[1];

    m_Buffer[0] = m_Buffer[1] = NULL;
    m_InBuffer[0] = m_InBuffer[1] = NULL;
}

void ESDClient::SendStereo(InputPort *ldata, InputPort *rdata)
{
    if (!host || m_Channels != 2 || !m_Buffer[m_WriteBufferNum])
        return;

    std::size_t on = 0;
    for (UnsignedType n = 0; n < host->SampleCount(); ++n)
    {
        if (m_IsDead) return;

        if (ldata)
        {
            const FloatType v = ldata->GetSampleValue(n, 0) * m_Amp;
            int mixed = static_cast<int>(m_Buffer[m_WriteBufferNum][on]) +
                        static_cast<int>(ToS16(v));
            if (mixed > SHRT_MAX) mixed = SHRT_MAX;
            if (mixed < SHRT_MIN) mixed = SHRT_MIN;
            m_Buffer[m_WriteBufferNum][on] = static_cast<short>(mixed);
        }
        ++on;

        if (rdata)
        {
            const FloatType v = rdata->GetSampleValue(n, 0) * m_Amp;
            int mixed = static_cast<int>(m_Buffer[m_WriteBufferNum][on]) +
                        static_cast<int>(ToS16(v));
            if (mixed > SHRT_MAX) mixed = SHRT_MAX;
            if (mixed < SHRT_MIN) mixed = SHRT_MIN;
            m_Buffer[m_WriteBufferNum][on] = static_cast<short>(mixed);
        }
        ++on;
    }
}

bool ESDClient::WriteAll(const void *source, std::size_t bytes)
{
    if (m_PlaybackHandle < 0)
        return false;

    const unsigned char *data = static_cast<const unsigned char *>(source);
    std::size_t done = 0;

    while (done < bytes)
    {
        const ssize_t result = write(m_PlaybackHandle, data + done, bytes - done);
        if (result > 0)
        {
            done += static_cast<std::size_t>(result);
            continue;
        }
        if (result < 0 && errno == EINTR)
            continue;

        cerr << "ESD: playback write failed";
        if (result < 0) cerr << ": " << strerror(errno);
        cerr << endl;
        return false;
    }

    return true;
}

bool ESDClient::ReadAll(void *destination, std::size_t bytes)
{
    if (m_CaptureHandle < 0)
        return false;

    unsigned char *data = static_cast<unsigned char *>(destination);
    std::size_t done = 0;

    while (done < bytes)
    {
        const ssize_t result = read(m_CaptureHandle, data + done, bytes - done);
        if (result > 0)
        {
            done += static_cast<std::size_t>(result);
            continue;
        }
        if (result < 0 && errno == EINTR)
            continue;

        if (result < 0)
            cerr << "ESD: capture read failed: " << strerror(errno) << endl;
        else
            cerr << "ESD: capture stream closed by server" << endl;
        return false;
    }

    return true;
}

void ESDClient::Play()
{
    if (!host || !m_Buffer[0])
        return;

    const int bufferToSend = !m_WriteBufferNum;
    const std::size_t samples =
        static_cast<std::size_t>(host->SampleCount()) *
        static_cast<std::size_t>(m_Channels);
    const std::size_t bytes = samples * sizeof(short);

    if (m_HasPlayback && !m_IsDead)
        WriteAll(m_Buffer[bufferToSend], bytes);

    std::memset(m_Buffer[bufferToSend], 0, bytes);
    m_WriteBufferNum = bufferToSend;
}

void ESDClient::Read()
{
    if (!host || !m_InBuffer[0])
        return;

    const int bufferToRead = !m_ReadBufferNum;
    const std::size_t samples =
        static_cast<std::size_t>(host->SampleCount()) *
        static_cast<std::size_t>(m_Channels);
    const std::size_t bytes = samples * sizeof(short);

    std::memset(m_InBuffer[bufferToRead], 0, bytes);
    if (m_HasCapture && !m_IsDead)
        ReadAll(m_InBuffer[bufferToRead], bytes);

    m_ReadBufferNum = bufferToRead;
}

void ESDClient::GetStereo(OutputPort *ldata, OutputPort *rdata)
{
    if (!host || m_Channels != 2 || !m_InBuffer[m_ReadBufferNum])
        return;

    std::size_t on = 0;
    for (UnsignedType n = 0; n < host->SampleCount(); ++n)
    {
        if (m_IsDead) return;

        if (ldata)
            ldata->SetSampleValue(
                n,
                (static_cast<FloatType>(m_InBuffer[m_ReadBufferNum][on]) * m_Amp) /
                    static_cast<FloatType>(SHRT_MAX),
                0);
        ++on;

        if (rdata)
            rdata->SetSampleValue(
                n,
                (static_cast<FloatType>(m_InBuffer[m_ReadBufferNum][on]) * m_Amp) /
                    static_cast<FloatType>(SHRT_MAX),
                0);
        ++on;
    }
}

const char *ESDClient::ServerName() const
{
    if (m_Destination.empty() || m_Destination == "default")
        return NULL;
    return m_Destination.c_str();
}

bool ESDClient::Close()
{
    if (m_PlaybackHandle >= 0)
    {
        esd_close(m_PlaybackHandle);
        m_PlaybackHandle = -1;
    }
    if (m_CaptureHandle >= 0)
    {
        esd_close(m_CaptureHandle);
        m_CaptureHandle = -1;
    }

    m_HasPlayback = false;
    m_HasCapture = false;
    return true;
}

bool ESDClient::OpenWrite()
{
    return Open(true, false);
}

bool ESDClient::OpenRead()
{
    return Open(false, true);
}

bool ESDClient::OpenReadWrite()
{
    return Open(true, true);
}

bool ESDClient::Open(bool playback, bool capture)
{
    Close();

    if (!host)
    {
        cerr << "ESD: host Patch is NULL" << endl;
        return false;
    }
    if (m_Channels != 2)
    {
        cerr << "ESD: only stereo is supported" << endl;
        return false;
    }

    m_IsDead = false;
    DeallocateBuffer();
    AllocateBuffer();
    if (!m_Buffer[0])
        return false;

    const int baseFormat = ESD_STREAM | ESD_BITS16 | ESD_STEREO;
    const char *server = ServerName();

    if (playback)
    {
        m_PlaybackHandle = esd_play_stream(
            baseFormat | ESD_PLAY,
            static_cast<int>(host->SampleRate()),
            server,
            "SpiralSynthModular");

        if (m_PlaybackHandle < 0)
        {
            cerr << "ESD: cannot open playback stream"
                 << (server ? " on requested server" : " on default server")
                 << endl;
            Close();
            return false;
        }
        m_HasPlayback = true;
    }

    if (capture)
    {
        m_CaptureHandle = esd_record_stream(
            baseFormat | ESD_RECORD,
            static_cast<int>(host->SampleRate()),
            server,
            "SpiralSynthModular");

        if (m_CaptureHandle < 0)
        {
            cerr << "ESD: cannot open capture stream"
                 << (server ? " on requested server" : " on default server")
                 << endl;
            Close();
            return false;
        }
        m_HasCapture = true;
    }

    cerr << "ESD: connected, rate=" << host->SampleRate()
         << ", channels=" << m_Channels
         << ", engine block=" << host->SampleCount()
         << ", server=" << (server ? server : "default") << endl;

    return true;
}

#endif /* OUTPUT_BACKEND_ESD */
