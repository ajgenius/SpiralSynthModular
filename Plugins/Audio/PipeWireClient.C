#include "PipeWireClient.h"

#ifdef OUTPUT_BACKEND_PIPEWIRE

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <thread>

#include <spa/utils/result.h>

using namespace std;

PipeWireClient *PipeWireClient::m_Singleton = NULL;
Patch *PipeWireClient::host = NULL;
void (*PipeWireClient::RunCallback)(void*, bool) = NULL;
void *PipeWireClient::RunContext = NULL;

namespace
{
    const unsigned kPipeWireChannels = 2;
    const std::size_t kMinimumRingFrames = 4096;
    const unsigned kRingBlocks = 8;
    const unsigned kOpenTimeoutMs = 2000;

    inline float ClampAudio(FloatType v)
    {
        if (v > (FloatType)1.0) return 1.0f;
        if (v < (FloatType)-1.0) return -1.0f;
        return static_cast<float>(v);
    }
}

PipeWireClient::AudioRingBuffer::AudioRingBuffer() :
    m_CapacityFrames(0), m_Channels(0), m_ReadFrame(0), m_WriteFrame(0)
{
}

void PipeWireClient::AudioRingBuffer::Configure(std::size_t capacityFrames,
                                                unsigned channels)
{
    m_CapacityFrames = capacityFrames;
    m_Channels = channels;
    m_Data.assign(capacityFrames * channels, 0.0f);
    Reset();
}

void PipeWireClient::AudioRingBuffer::Reset()
{
    m_ReadFrame.store(0, std::memory_order_relaxed);
    m_WriteFrame.store(0, std::memory_order_relaxed);
    if (!m_Data.empty())
        std::fill(m_Data.begin(), m_Data.end(), 0.0f);
}

std::size_t PipeWireClient::AudioRingBuffer::ReadAvailable() const
{
    const std::uint64_t read = m_ReadFrame.load(std::memory_order_acquire);
    const std::uint64_t write = m_WriteFrame.load(std::memory_order_acquire);
    const std::uint64_t available = write - read;
    return static_cast<std::size_t>(
        std::min<std::uint64_t>(available, m_CapacityFrames));
}

std::size_t PipeWireClient::AudioRingBuffer::WriteAvailable() const
{
    return m_CapacityFrames - ReadAvailable();
}

std::size_t PipeWireClient::AudioRingBuffer::Write(const float *src,
                                                   std::size_t frames)
{
    if (!src || !frames || !m_CapacityFrames || !m_Channels)
        return 0;

    const std::uint64_t write = m_WriteFrame.load(std::memory_order_relaxed);
    const std::uint64_t read = m_ReadFrame.load(std::memory_order_acquire);
    const std::uint64_t used64 = write - read;
    const std::size_t used = static_cast<std::size_t>(
        std::min<std::uint64_t>(used64, m_CapacityFrames));
    const std::size_t todo = std::min(frames, m_CapacityFrames - used);
    if (!todo) return 0;

    const std::size_t start = static_cast<std::size_t>(write % m_CapacityFrames);
    const std::size_t first = std::min(todo, m_CapacityFrames - start);
    const std::size_t second = todo - first;

    std::memcpy(&m_Data[start * m_Channels], src,
                first * m_Channels * sizeof(float));
    if (second)
        std::memcpy(&m_Data[0], src + first * m_Channels,
                    second * m_Channels * sizeof(float));

    m_WriteFrame.store(write + todo, std::memory_order_release);
    return todo;
}

std::size_t PipeWireClient::AudioRingBuffer::Read(float *dst,
                                                  std::size_t frames)
{
    if (!dst || !frames || !m_CapacityFrames || !m_Channels)
        return 0;

    const std::uint64_t read = m_ReadFrame.load(std::memory_order_relaxed);
    const std::uint64_t write = m_WriteFrame.load(std::memory_order_acquire);
    const std::size_t available = static_cast<std::size_t>(
        std::min<std::uint64_t>(write - read, m_CapacityFrames));
    const std::size_t todo = std::min(frames, available);
    if (!todo) return 0;

    const std::size_t start = static_cast<std::size_t>(read % m_CapacityFrames);
    const std::size_t first = std::min(todo, m_CapacityFrames - start);
    const std::size_t second = todo - first;

    std::memcpy(dst, &m_Data[start * m_Channels],
                first * m_Channels * sizeof(float));
    if (second)
        std::memcpy(dst + first * m_Channels, &m_Data[0],
                    second * m_Channels * sizeof(float));

    m_ReadFrame.store(read + todo, std::memory_order_release);
    return todo;
}

PipeWireClient::StreamState::StreamState() :
    owner(NULL), stream(NULL), capture(false), state(PW_STREAM_STATE_UNCONNECTED)
{
    std::memset(&listener, 0, sizeof(listener));
    std::memset(&events, 0, sizeof(events));
}

PipeWireClient::PipeWireClient() :
    m_Loop(NULL), m_Context(NULL), m_Core(NULL),
    m_Amp(0.5), m_Channels(2),
    m_ReadBufferNum(0), m_WriteBufferNum(0),
    m_IsDead(false), m_Connected(false), m_LoopStarted(false),
    m_PlaybackUnderruns(0), m_CaptureOverruns(0),
    m_Destination("default")
{
    m_Buffer[0] = m_Buffer[1] = NULL;
    m_InBuffer[0] = m_InBuffer[1] = NULL;
    m_Playback.owner = this;
    m_Playback.capture = false;
    m_Capture.owner = this;
    m_Capture.capture = true;
}

PipeWireClient::~PipeWireClient()
{
    Close();
    DeallocateBuffer();
}

void PipeWireClient::AllocateBuffer()
{
    if (m_Buffer[0] || !host) return;

    const std::size_t frames = host->SampleCount();
    const std::size_t samples = frames * static_cast<std::size_t>(m_Channels);

    m_Buffer[0] = new float[samples];
    m_Buffer[1] = new float[samples];
    m_InBuffer[0] = new float[samples];
    m_InBuffer[1] = new float[samples];

    std::memset(m_Buffer[0], 0, samples * sizeof(float));
    std::memset(m_Buffer[1], 0, samples * sizeof(float));
    std::memset(m_InBuffer[0], 0, samples * sizeof(float));
    std::memset(m_InBuffer[1], 0, samples * sizeof(float));

    const std::size_t ringFrames = std::max<std::size_t>(
        kMinimumRingFrames,
        frames * static_cast<std::size_t>(kRingBlocks));

    m_PlaybackRing.Configure(ringFrames, m_Channels);
    m_CaptureRing.Configure(ringFrames, m_Channels);
}

void PipeWireClient::DeallocateBuffer()
{
    delete [] m_Buffer[0];
    delete [] m_Buffer[1];
    delete [] m_InBuffer[0];
    delete [] m_InBuffer[1];
    m_Buffer[0] = m_Buffer[1] = NULL;
    m_InBuffer[0] = m_InBuffer[1] = NULL;
}

bool PipeWireClient::OpenWrite()
{
    return Setup(true, false);
}

bool PipeWireClient::OpenRead()
{
    return Setup(false, true);
}

bool PipeWireClient::OpenReadWrite()
{
    return Setup(true, true);
}

bool PipeWireClient::Setup(bool playback, bool capture)
{
    Close();

    if (!host)
    {
        cerr << "PipeWire: host Patch is NULL" << endl;
        return false;
    }
    if (m_Channels != static_cast<int>(kPipeWireChannels))
    {
        cerr << "PipeWire: only stereo is currently supported" << endl;
        return false;
    }

    m_IsDead.store(false, std::memory_order_release);
    m_PlaybackUnderruns.store(0, std::memory_order_relaxed);
    m_CaptureOverruns.store(0, std::memory_order_relaxed);

    DeallocateBuffer();
    AllocateBuffer();
    if (!m_Buffer[0]) return false;

    m_PlaybackRing.Reset();
    m_CaptureRing.Reset();

    pw_init(NULL, NULL);

    m_Loop = pw_thread_loop_new("Spiral PipeWire", NULL);
    if (!m_Loop)
    {
        cerr << "PipeWire: cannot create thread loop" << endl;
        return false;
    }

    if (playback && !CreateStream(m_Playback, false))
    {
        Close();
        return false;
    }
    if (capture && !CreateStream(m_Capture, true))
    {
        Close();
        return false;
    }

    const int loopResult = pw_thread_loop_start(m_Loop);
    if (loopResult < 0)
    {
        cerr << "PipeWire: cannot start thread loop: "
             << spa_strerror(loopResult) << endl;
        Close();
        return false;
    }
    m_LoopStarted = true;

    if (playback && !WaitForReady(m_Playback))
    {
        Close();
        return false;
    }
    if (capture && !WaitForReady(m_Capture))
    {
        Close();
        return false;
    }

    m_Connected = true;
    cerr << "PipeWire: connected, rate=" << host->SampleRate()
         << ", channels=" << m_Channels
         << ", engine block=" << host->SampleCount()
         << ", compatibility ring=" << m_PlaybackRing.CapacityFrames()
         << " frames" << endl;
    return true;
}

bool PipeWireClient::CreateStream(StreamState &state, bool capture)
{
    state.owner = this;
    state.capture = capture;
    state.state.store(PW_STREAM_STATE_UNCONNECTED, std::memory_order_relaxed);

    std::memset(&state.events, 0, sizeof(state.events));
    state.events.version = PW_VERSION_STREAM_EVENTS;
    state.events.state_changed = &PipeWireClient::OnStreamStateChanged;
    state.events.process = &PipeWireClient::OnStreamProcess;

    char latency[64];
    std::snprintf(latency, sizeof(latency), "%u/%u",
                  static_cast<unsigned>(host->SampleCount()),
                  static_cast<unsigned>(host->SampleRate()));

    pw_properties *props = pw_properties_new(
        PW_KEY_MEDIA_TYPE, "Audio",
        PW_KEY_MEDIA_CATEGORY, capture ? "Capture" : "Playback",
        PW_KEY_MEDIA_ROLE, "Music",
        PW_KEY_APP_NAME, "SpiralSynthModular",
        PW_KEY_NODE_NAME, capture ? "spiral.capture" : "spiral.playback",
        PW_KEY_NODE_DESCRIPTION,
            capture ? "SpiralSynthModular Capture" : "SpiralSynthModular Playback",
        PW_KEY_NODE_LATENCY, latency,
        NULL);

    if (!props)
    {
        cerr << "PipeWire: cannot allocate stream properties" << endl;
        return false;
    }

    if (!m_Destination.empty() && m_Destination != "default")
        pw_properties_set(props, "target.object", m_Destination.c_str());

    state.stream = pw_stream_new_simple(
        pw_thread_loop_get_loop(m_Loop),
        capture ? "Spiral Capture" : "Spiral Playback",
        props,
        &state.events,
        &state);

    /* pw_stream_new_simple() takes ownership of props. */
    if (!state.stream)
    {
        cerr << "PipeWire: cannot create "
             << (capture ? "capture" : "playback") << " stream" << endl;
        return false;
    }

    spa_audio_info_raw info;
    std::memset(&info, 0, sizeof(info));
    info.format = SPA_AUDIO_FORMAT_F32;
    info.rate = host->SampleRate();
    info.channels = m_Channels;
    info.position[0] = SPA_AUDIO_CHANNEL_FL;
    info.position[1] = SPA_AUDIO_CHANNEL_FR;

    uint8_t podBuffer[1024];
    spa_pod_builder builder = SPA_POD_BUILDER_INIT(podBuffer, sizeof(podBuffer));
    const spa_pod *params[1];
    params[0] = spa_format_audio_raw_build(
        &builder, SPA_PARAM_EnumFormat, &info);

    if (!params[0])
    {
        cerr << "PipeWire: could not build audio format" << endl;
        DestroyStream(state);
        return false;
    }

    const enum pw_direction direction =
        capture ? PW_DIRECTION_INPUT : PW_DIRECTION_OUTPUT;

    /* Keep the callback on our thread loop for now.  RT_PROCESS moves it to
       PipeWire's data-loop thread and adds a second lifetime/threading domain
       that this legacy compatibility layer does not need. */
    const enum pw_stream_flags flags = static_cast<enum pw_stream_flags>(
        PW_STREAM_FLAG_AUTOCONNECT |
        PW_STREAM_FLAG_MAP_BUFFERS);

    const int result = pw_stream_connect(
        state.stream,
        direction,
        PW_ID_ANY,
        flags,
        params,
        1);

    if (result < 0)
    {
        cerr << "PipeWire: cannot connect "
             << (capture ? "capture" : "playback")
             << " stream: " << spa_strerror(result) << endl;
        DestroyStream(state);
        return false;
    }

    state.state.store(PW_STREAM_STATE_CONNECTING, std::memory_order_release);
    return true;
}

bool PipeWireClient::WaitForReady(StreamState &stream)
{
    for (unsigned waited = 0; waited < kOpenTimeoutMs; ++waited)
    {
        const int state = stream.state.load(std::memory_order_acquire);

        if (state == PW_STREAM_STATE_PAUSED ||
            state == PW_STREAM_STATE_STREAMING)
            return true;

        if (state == PW_STREAM_STATE_ERROR ||
            state == PW_STREAM_STATE_UNCONNECTED)
            return false;

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    cerr << "PipeWire: stream connection timed out" << endl;
    return false;
}

void PipeWireClient::DestroyStream(StreamState &state)
{
    pw_stream *stream = state.stream;
    state.stream = NULL;
    state.state.store(PW_STREAM_STATE_UNCONNECTED, std::memory_order_release);

    if (stream)
        pw_stream_destroy(stream);
}

bool PipeWireClient::Close()
{
    m_Connected = false;
    m_IsDead.store(true, std::memory_order_release);

    if (m_Loop && m_LoopStarted)
    {
        pw_thread_loop_stop(m_Loop);
        m_LoopStarted = false;
    }

    /* With the loop stopped no process callback can race stream destruction. */
    DestroyStream(m_Playback);
    DestroyStream(m_Capture);

    /* m_Context/m_Core are intentionally unused with pw_stream_new_simple(). */
    m_Context = NULL;
    m_Core = NULL;

    if (m_Loop)
    {
        pw_thread_loop_destroy(m_Loop);
        m_Loop = NULL;
    }

    const std::uint64_t underruns =
        m_PlaybackUnderruns.load(std::memory_order_relaxed);
    const std::uint64_t overruns =
        m_CaptureOverruns.load(std::memory_order_relaxed);

    if (underruns)
        cerr << "PipeWire: playback callback underruns=" << underruns << endl;
    if (overruns)
        cerr << "PipeWire: capture callback overruns=" << overruns << endl;

    return true;
}

void PipeWireClient::OnStreamStateChanged(void *data,
                                          enum pw_stream_state oldState,
                                          enum pw_stream_state state,
                                          const char *error)
{
    StreamState *stream = static_cast<StreamState *>(data);
    if (!stream) return;

    stream->state.store(static_cast<int>(state), std::memory_order_release);

    cerr << "PipeWire: " << (stream->capture ? "capture" : "playback")
         << " state " << pw_stream_state_as_string(oldState)
         << " -> " << pw_stream_state_as_string(state);
    if (error && *error) cerr << ": " << error;
    cerr << endl;
}

void PipeWireClient::OnStreamProcess(void *data)
{
    StreamState *stream = static_cast<StreamState *>(data);
    if (!stream || !stream->owner || !stream->stream) return;

    if (stream->capture)
        stream->owner->ProcessCapture(*stream);
    else
        stream->owner->ProcessPlayback(*stream);
}

void PipeWireClient::ProcessPlayback(StreamState &state)
{
    if (!state.stream) return;

    pw_buffer *pwbuf = pw_stream_dequeue_buffer(state.stream);
    if (!pwbuf) return;

    spa_buffer *buffer = pwbuf->buffer;
    if (!buffer || !buffer->datas || buffer->n_datas == 0)
    {
        pw_stream_queue_buffer(state.stream, pwbuf);
        return;
    }

    spa_data &data = buffer->datas[0];
    if (!data.data || !data.chunk || data.maxsize == 0)
    {
        pw_stream_queue_buffer(state.stream, pwbuf);
        return;
    }

    const std::size_t stride =
        sizeof(float) * static_cast<std::size_t>(m_Channels);
    if (!stride || data.maxsize < stride)
    {
        data.chunk->offset = 0;
        data.chunk->stride = static_cast<int32_t>(stride);
        data.chunk->size = 0;
        pw_stream_queue_buffer(state.stream, pwbuf);
        return;
    }

    std::size_t frames = data.maxsize / stride;
    if (pwbuf->requested > 0)
        frames = std::min<std::size_t>(frames, pwbuf->requested);

    float *dst = static_cast<float *>(data.data);
    const std::size_t got = m_PlaybackRing.Read(dst, frames);

    for (std::size_t i = 0;
         i < got * static_cast<std::size_t>(m_Channels);
         ++i)
    {
        if (dst[i] > 1.0f) dst[i] = 1.0f;
        else if (dst[i] < -1.0f) dst[i] = -1.0f;
    }

    if (got < frames)
    {
        std::memset(dst + got * m_Channels,
                    0,
                    (frames - got) * stride);
        m_PlaybackUnderruns.fetch_add(1, std::memory_order_relaxed);
    }

    data.chunk->offset = 0;
    data.chunk->stride = static_cast<int32_t>(stride);
    data.chunk->size = static_cast<uint32_t>(frames * stride);

    pw_stream_queue_buffer(state.stream, pwbuf);
}

void PipeWireClient::ProcessCapture(StreamState &state)
{
    if (!state.stream) return;

    pw_buffer *pwbuf = pw_stream_dequeue_buffer(state.stream);
    if (!pwbuf) return;

    spa_buffer *buffer = pwbuf->buffer;
    if (!buffer || !buffer->datas || buffer->n_datas == 0)
    {
        pw_stream_queue_buffer(state.stream, pwbuf);
        return;
    }

    spa_data &data = buffer->datas[0];
    if (data.data && data.chunk && data.maxsize)
    {
        const std::size_t stride =
            sizeof(float) * static_cast<std::size_t>(m_Channels);

        if (stride)
        {
            const std::size_t offset =
                std::min<std::size_t>(data.chunk->offset, data.maxsize);
            const std::size_t remaining = data.maxsize - offset;
            const std::size_t bytes =
                std::min<std::size_t>(data.chunk->size, remaining);
            const std::size_t frames = bytes / stride;

            const uint8_t *base =
                static_cast<const uint8_t *>(data.data) + offset;
            const float *src = reinterpret_cast<const float *>(base);

            const std::size_t written =
                m_CaptureRing.Write(src, frames);
            if (written < frames)
                m_CaptureOverruns.fetch_add(1, std::memory_order_relaxed);
        }
    }

    pw_stream_queue_buffer(state.stream, pwbuf);
}

void PipeWireClient::SendStereo(InputPort *ldata, InputPort *rdata)
{
    if (m_Channels != 2 || !host || !m_Buffer[m_WriteBufferNum]) return;

    std::size_t on = 0;
    for (UnsignedType n = 0; n < host->SampleCount(); ++n)
    {
        if (m_IsDead.load(std::memory_order_acquire)) return;

        if (ldata)
            m_Buffer[m_WriteBufferNum][on] +=
                ClampAudio(ldata->GetSampleValue(n, 0) * m_Amp);
        ++on;

        if (rdata)
            m_Buffer[m_WriteBufferNum][on] +=
                ClampAudio(rdata->GetSampleValue(n, 0) * m_Amp);
        ++on;
    }
}

bool PipeWireClient::PlaybackOperational() const
{
    const int state = m_Playback.state.load(std::memory_order_acquire);
    return m_Playback.stream &&
           state != PW_STREAM_STATE_ERROR &&
           state != PW_STREAM_STATE_UNCONNECTED;
}

bool PipeWireClient::CaptureOperational() const
{
    const int state = m_Capture.state.load(std::memory_order_acquire);
    return m_Capture.stream &&
           state != PW_STREAM_STATE_ERROR &&
           state != PW_STREAM_STATE_UNCONNECTED;
}

void PipeWireClient::Play()
{
    if (!host || !m_Buffer[0]) return;

    const int bufferToSend = !m_WriteBufferNum;
    const std::size_t frames = host->SampleCount();
    const std::size_t samples =
        frames * static_cast<std::size_t>(m_Channels);

    if (PlaybackOperational())
    {
        std::size_t done = 0;
        while (done < frames &&
               !m_IsDead.load(std::memory_order_acquire))
        {
            const std::size_t wrote = m_PlaybackRing.Write(
                m_Buffer[bufferToSend] + done * m_Channels,
                frames - done);
            done += wrote;

            if (done < frames)
            {
                if (!PlaybackOperational()) break;
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        }
    }

    std::memset(m_Buffer[bufferToSend], 0, samples * sizeof(float));
    m_WriteBufferNum = bufferToSend;
}

void PipeWireClient::Read()
{
    if (!host || !m_InBuffer[0]) return;

    const int bufferToRead = !m_ReadBufferNum;
    const std::size_t frames = host->SampleCount();
    const std::size_t samples =
        frames * static_cast<std::size_t>(m_Channels);

    std::memset(m_InBuffer[bufferToRead], 0, samples * sizeof(float));

    if (CaptureOperational())
    {
        std::size_t done = 0;
        while (done < frames &&
               !m_IsDead.load(std::memory_order_acquire))
        {
            const std::size_t got = m_CaptureRing.Read(
                m_InBuffer[bufferToRead] + done * m_Channels,
                frames - done);
            done += got;

            if (done < frames)
            {
                if (!CaptureOperational()) break;
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        }
    }

    m_ReadBufferNum = bufferToRead;
}

void PipeWireClient::GetStereo(OutputPort *ldata, OutputPort *rdata)
{
    if (m_Channels != 2 || !host || !m_InBuffer[m_ReadBufferNum]) return;

    std::size_t on = 0;
    for (UnsignedType n = 0; n < host->SampleCount(); ++n)
    {
        if (m_IsDead.load(std::memory_order_acquire)) return;

        if (ldata)
            ldata->SetSampleValue(
                n,
                static_cast<FloatType>(m_InBuffer[m_ReadBufferNum][on]) * m_Amp,
                0);
        ++on;

        if (rdata)
            rdata->SetSampleValue(
                n,
                static_cast<FloatType>(m_InBuffer[m_ReadBufferNum][on]) * m_Amp,
                0);
        ++on;
    }
}

#endif /* OUTPUT_BACKEND_PIPEWIRE */
