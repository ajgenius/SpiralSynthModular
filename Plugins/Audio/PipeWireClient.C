// SPDX-License-Identifier: GPL-2.0-or-later
#include "PipeWireClient.h"
#include <spa/param/audio/format-utils.h>
#include <algorithm>
#include <cstring>
#include <cstdio>
#include <pthread.h>
#include <unistd.h>
using namespace spiralcore;

namespace
{
	pthread_once_t initialized = PTHREAD_ONCE_INIT;

	void Initialize()
	{
		pw_init(NULL, NULL);
	}

	const unsigned MaxFrames = 16384;
}

PipeWireClient::PipeWireClient():
    m_Loop(NULL),
    m_LoopStarted(false),
    m_Attached(false),
    m_InputDelay(0),
    m_OutputDelay(0),
    m_Quantum(0),
    m_CurrentInput(NULL),
    m_CurrentOutput(NULL),
    m_CurrentFrames(0),
    m_Run(NULL),
    m_Context(NULL)
{
}

PipeWireClient::~PipeWireClient()
{
	Detach();
}

bool PipeWireClient::Attach(const std::string &device, const AudioClientOptions &options)
{
	Detach();
	if (!options.BufferSize || options.BufferSize > MaxFrames || options.Samplerate < 8000 ||
		options.Samplerate > 384000 || options.InChannels > 2 || options.OutChannels > 2 ||
		(!options.InChannels && !options.OutChannels))
		return false;

	m_Options = options;
	m_Timing = AudioCycleTiming();
	m_Input.Frame = m_Output.Frame = 0;
	m_InputDelay.store(0);
	m_OutputDelay.store(0);
	m_Quantum.store(options.BufferSize);
	pthread_once(&initialized, Initialize);
	if (options.InChannels && options.OutChannels &&
		!m_Capture.Configure(65536, options.InChannels, options.Samplerate, options.Samplerate))
		return false;

	m_Loop = pw_thread_loop_new("ssm-audio", NULL);
	if (!m_Loop)
		return false;

	if ((options.InChannels && !CreateStream(m_Input, true, device)) ||
		(options.OutChannels && !CreateStream(m_Output, false, device)) || pw_thread_loop_start(m_Loop) < 0)
	{
		Detach();
		return false;
	}

	m_LoopStarted = true;
	if ((options.InChannels && !WaitReady(m_Input)) || (options.OutChannels && !WaitReady(m_Output)))
	{
		Detach();
		return false;
	}

	m_Attached.store(true);
	return true;
}

bool PipeWireClient::CreateStream(Stream &stream, bool input, const std::string &device)
{
	stream.Owner = this;
	stream.Input = input;
	std::memset(&stream.Events, 0, sizeof(stream.Events));
	stream.Events.version = PW_VERSION_STREAM_EVENTS;
	stream.Events.state_changed = StateChanged;
	stream.Events.process = Process;
	char latency[64];
	std::snprintf(latency, sizeof(latency), "%u/%u", m_Options.BufferSize, m_Options.Samplerate);
	pw_properties *props = pw_properties_new(PW_KEY_MEDIA_TYPE, "Audio",
						 PW_KEY_MEDIA_CATEGORY, input ? "Capture" : "Playback", PW_KEY_MEDIA_ROLE, "Music",
						 PW_KEY_APP_NAME, "SpiralSynthModular", PW_KEY_NODE_LATENCY, latency, NULL);
	if (!props)
		return false;

	if (!device.empty() && device != "default")
		pw_properties_set(props, "target.object", device.c_str());

	stream.Handle = pw_stream_new_simple(pw_thread_loop_get_loop(m_Loop),
					     input ? "Spiral Capture" : "Spiral Playback", props, &stream.Events, &stream);
	if (!stream.Handle)
		return false;

	spa_audio_info_raw info;
	std::memset(&info, 0, sizeof(info));
	info.format = SPA_AUDIO_FORMAT_F32;
	info.rate = m_Options.Samplerate;
	info.channels = input ? m_Options.InChannels : m_Options.OutChannels;
	info.position[0] = info.channels == 1 ? SPA_AUDIO_CHANNEL_MONO : SPA_AUDIO_CHANNEL_FL;
	info.position[1] = SPA_AUDIO_CHANNEL_FR;
	uint8_t storage[1024];
	spa_pod_builder builder = SPA_POD_BUILDER_INIT(storage, sizeof(storage));
	const spa_pod *parameters[] = {spa_format_audio_raw_build(&builder, SPA_PARAM_EnumFormat, &info)};
	stream.State.store(PW_STREAM_STATE_CONNECTING);
	// Both directions run on this one loop, so duplex capture history has one
	// producer and consumer. No callback runs the graph or takes the host gate.
	return pw_stream_connect(stream.Handle, input ? PW_DIRECTION_INPUT : PW_DIRECTION_OUTPUT,
				 PW_ID_ANY, static_cast<pw_stream_flags>(PW_STREAM_FLAG_AUTOCONNECT |
		PW_STREAM_FLAG_MAP_BUFFERS | PW_STREAM_FLAG_INACTIVE), parameters, 1)
		>= 0;
}

bool PipeWireClient::WaitReady(Stream &stream)
{
	for (unsigned n = 0; n < 1000; ++n)
	{
		const int state = stream.State.load();
		if (state == PW_STREAM_STATE_PAUSED || state == PW_STREAM_STATE_STREAMING)
			return true;
		if (state == PW_STREAM_STATE_ERROR || state == PW_STREAM_STATE_UNCONNECTED)
			return false;
		usleep(1000);
	}
	return false;
}

void PipeWireClient::SetCallback(void (*run)(void *, unsigned), void *context)
{
	m_Run = run;
	m_Context = context;
}

bool PipeWireClient::Start()
{
	if (!IsAttached() || !m_Run)
		return false;
	pw_thread_loop_lock(m_Loop);
	const bool ok = (!m_Input.Handle || pw_stream_set_active(m_Input.Handle, true) >= 0) &&
		(!m_Output.Handle || pw_stream_set_active(m_Output.Handle, true) >= 0);
	pw_thread_loop_unlock(m_Loop);
	return ok;
}

void PipeWireClient::Detach()
{
	m_Attached.store(false);
	if (m_LoopStarted)
		pw_thread_loop_stop(m_Loop);
	m_LoopStarted = false;
	if (m_Input.Handle)
		pw_stream_destroy(m_Input.Handle);
	if (m_Output.Handle)
		pw_stream_destroy(m_Output.Handle);
	m_Input.Handle = m_Output.Handle = NULL;
	if (m_Loop)
		pw_thread_loop_destroy(m_Loop);
	m_Loop = NULL;
	m_CurrentInput = m_CurrentOutput = NULL;
}

void PipeWireClient::StateChanged(void *data, pw_stream_state, pw_stream_state state, const char *)
{
	Stream &stream = *static_cast<Stream *>(data);
	stream.State.store(state);
	if (state == PW_STREAM_STATE_ERROR || state == PW_STREAM_STATE_UNCONNECTED)
		stream.Owner->m_Attached.store(false);
}

bool PipeWireClient::Timestamp(Stream &stream, unsigned frames, AudioCycleTiming &timing)
{
	pw_time value;
	std::memset(&value, 0, sizeof(value));
	if (pw_stream_get_time_n(stream.Handle, &value, sizeof(value)) < 0 || !value.rate.denom || value.now <= 0)
		return false;

	const double step = 1.0 / m_Options.Samplerate;
	const double delay = std::max(0.0, value.delay * double(value.rate.num) / value.rate.denom);
	const double queued = (value.queued + value.buffered) * step;
	timing = AudioCycleTiming();
	timing.Frame = stream.Frame;
	timing.CallbackTime = AudioMonotonicTime();
	timing.Step = step;
	timing.SampleRate = m_Options.Samplerate;
	timing.Frames = frames;
	// pw_time.now is CLOCK_MONOTONIC. Playback follows PipeWire's documented
	// delay + queued + resampler-buffered formula; capture covers the preceding
	// buffer interval. Capture timing remains an estimate until loopback checked.
	// https://docs.pipewire.org/structpw__time.html
	timing.OutputTime = value.now / 1e9 + delay + queued;
	timing.InputTime = value.now / 1e9 - delay - queued - frames * step;
	timing.Valid = true;
	timing.Estimated = stream.Input;
	(stream.Input ? m_InputDelay : m_OutputDelay).store(unsigned(std::min(2.0, delay + queued + frames * step) * 1e6));
	stream.Frame += frames;
	return true;
}

void PipeWireClient::Process(void *data)
{
	Stream &stream = *static_cast<Stream *>(data);
	stream.Owner->Transfer(stream);
}

void PipeWireClient::Transfer(Stream &stream)
{
	pw_buffer *buffer = pw_stream_dequeue_buffer(stream.Handle);
	if (!buffer)
		return;
	spa_buffer *spa = buffer->buffer;
	if (!spa || !spa->n_datas || !spa->datas[0].data || !spa->datas[0].chunk)
	{
		pw_stream_queue_buffer(stream.Handle, buffer);
		return;
	}

	spa_data &data = spa->datas[0];
	const unsigned channels = stream.Input ? m_Options.InChannels : m_Options.OutChannels;
	const unsigned stride = channels * sizeof(float);
	const unsigned offset = stream.Input ? data.chunk->offset : 0;
	unsigned frames = offset <= data.maxsize ? (data.maxsize - offset) / stride : 0;
	frames = stream.Input ? std::min(frames, data.chunk->size / stride) :
				std::min<uint64_t>(frames, buffer->requested ? buffer->requested : m_Options.BufferSize);
	if (!frames || frames > MaxFrames)
	{
		m_Attached.store(false);
		pw_stream_queue_buffer(stream.Handle, buffer);
		return;
	}

	// Account for the negotiated quantum in host pacing and duplex lookback.
	m_Quantum.store(std::max(m_Quantum.load(), frames));

	float *samples = reinterpret_cast<float *>(static_cast<char *>(data.data) + offset);
	if (!stream.Input)
	{
		std::memset(samples, 0, frames * stride);
		data.chunk->offset = 0;
		data.chunk->stride = stride;
		data.chunk->size = frames * stride;
		buffer->size = frames;
	}

	AudioCycleTiming timing;
	if (m_Attached.load() && Timestamp(stream, frames, timing))
	{
		if (stream.Input && m_Options.OutChannels)
		{
			AudioStamp stamp;
			stamp.Frame = timing.Frame;
			stamp.Generation = 1;
			stamp.Time = timing.InputTime;
			stamp.Step = timing.Step;
			m_Capture.Write(samples, frames, stamp);
		}
		else
		{
			m_Timing = timing;
			m_CurrentFrames = frames;
			m_CurrentInput = stream.Input ? samples : NULL;
			m_CurrentOutput = stream.Input ? NULL : samples;
			if (!stream.Input && m_Options.InChannels)
				m_Timing.InputTime = timing.CallbackTime - GetInputLatency();
			if (m_Run)
				m_Run(m_Context, frames);
			m_CurrentInput = m_CurrentOutput = NULL;
		}
	}
	pw_stream_queue_buffer(stream.Handle, buffer);
}

bool PipeWireClient::Write(const float *samples, unsigned frames)
{
	if (!m_CurrentOutput || frames != m_CurrentFrames)
		return false;
	std::memcpy(m_CurrentOutput, samples, frames * m_Options.OutChannels * sizeof(float));
	return true;
}

bool PipeWireClient::Read(float *samples, unsigned frames)
{
	if (frames != m_CurrentFrames)
		return false;
	if (m_CurrentInput)
		std::memcpy(samples, m_CurrentInput, frames * m_Options.InChannels * sizeof(float));
	else if (m_Options.OutChannels)
		m_Capture.Read(samples, frames, m_Timing.InputTime, m_Timing.Step, 1);
	else
		return false;
	return true;
}
