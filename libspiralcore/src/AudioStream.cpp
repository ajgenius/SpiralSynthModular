// SPDX-License-Identifier: GPL-2.0-or-later
#include "AudioStream.h"
#include <algorithm>
#include <cstring>
#include <cmath>
#include <unistd.h>
#include <iostream>
using namespace spiralcore;

AudioStream::AudioStream() : m_Running(false), m_Stop(0), m_Client(NULL), m_Capacity(0), m_EngineFrames(0), m_InvalidTiming(0), m_NativeEpoch(1),
	m_EngineRate(0), m_NativeRate(0), m_Errors(0), m_Failed(0), m_Generation(0)
{
}

AudioStream::~AudioStream()
{
	Stop();
	Clear();
}

void AudioStream::Clear()
{
	for (unsigned n = 0; n < m_Input.size(); ++n) delete m_Input[n];

	for (unsigned n = 0; n < m_Output.size(); ++n) delete m_Output[n];

	m_Input.clear();
	m_Output.clear();
	std::vector<float>().swap(m_Native);
	std::vector<float>().swap(m_NativeMono);
	std::vector<float>().swap(m_EngineMono);
	m_Capacity = m_EngineFrames = 0;
	m_Client = NULL;
	m_LastTiming.Valid = false;
	m_PreviousTiming.Valid = false;
	++m_NativeEpoch;
	AudioCycleTiming unused;
	m_Mailbox.Read(unused);
	m_InputRate.Reset();
	m_OutputRate.Reset();
}

bool AudioStream::Configure(AudioClient *client, unsigned inputs, unsigned outputs,
	unsigned engineFrames, double engineRate)
{
	Stop();
	Clear();
	if (!client || !client->IsAttached() || (!inputs && !outputs) || inputs > 256 || outputs > 256 ||
		!engineFrames || engineFrames > 65536 || engineRate < 8000 || engineRate > 384000) return false;

	const unsigned nativeFrames = client->GetBufferSize();
	const double nativeRate = client->GetSampleRate();
	if (!nativeFrames || nativeFrames > 65536 || nativeRate < 8000 || nativeRate > 384000) return false;

	m_Client = client;
	m_EngineFrames = engineFrames;
	m_EngineRate = engineRate;
	m_NativeRate = nativeRate;
	m_InvalidTiming = 0;
	m_Capacity = std::max(4096U, std::max(engineFrames, nativeFrames) * 4);
	const double latency = std::max(client->GetInputLatency(), client->GetOutputLatency());
	if (latency < 0 || latency > 2) { Clear(); return false; }

	const unsigned capacity = std::max(m_Capacity * 4,
		unsigned((latency + 0.25) * std::max(nativeRate, engineRate)));
	unsigned rounded = 1;
	while (rounded < capacity) rounded <<= 1;

	const double inputTaps = 64 * std::ceil(std::max(1.0, nativeRate / engineRate));
	const double outputTaps = 64 * std::ceil(std::max(1.0, engineRate / nativeRate));
	const double bytes = double(rounded) * (inputs + outputs) * 36 +
		double(m_Capacity) * (std::max(inputs, outputs) + 1) * sizeof(float) +
		engineFrames * sizeof(float) + 513 * sizeof(float) * (inputs * inputTaps + outputs * outputTaps);
	if (bytes > 128 * 1024 * 1024)
	{
		std::cerr << "Audio stream: format exceeds the 128 MiB queue budget" << std::endl;
		Clear();
		return false;
	}

	m_Native.assign(m_Capacity * std::max(inputs, outputs), 0);
	m_NativeMono.assign(m_Capacity, 0);
	m_EngineMono.assign(engineFrames, 0);
	for (unsigned direction = 0; direction < 2; ++direction)
	{
		std::vector<TimedAudioBuffer *> &queues = direction ? m_Input : m_Output;
		const unsigned channels = direction ? inputs : outputs;
		for (unsigned n = 0; n < channels; ++n)
		{
			TimedAudioBuffer *queue = new TimedAudioBuffer;
			queues.push_back(queue);
			if (!queue->Configure(capacity, 1, direction ? nativeRate : engineRate,
				direction ? engineRate : nativeRate)) { Clear(); return false; }

		}

	}

	__sync_lock_test_and_set(&m_Errors, 0);
	__sync_lock_test_and_set(&m_Failed, 0);
	return true;
}

void AudioStream::Callback(void *context, unsigned frames)
{
	AudioStream *stream = static_cast<AudioStream *>(context);
	if (frames) stream->Transfer(frames);

}

bool AudioStream::Transfer(unsigned frames)
{
	if (!m_Client || !frames || frames > m_Capacity)
	{
		__sync_lock_test_and_set(&m_Failed, 1);
		return false;
	}

	AudioCycleTiming timing;
	if (!m_Client->GetCycleTiming(timing) || !timing.Valid)
	{
		__sync_fetch_and_add(&m_Errors, 1);
		if (++m_InvalidTiming > 100) __sync_lock_test_and_set(&m_Failed, 1);

		return false;
	}

	m_InvalidTiming = 0;
	if (timing.SampleRate != m_NativeRate ||
		(!m_Output.empty() && !(timing.OutputTime >= 0 && timing.OutputTime < 1e12)) ||
		(!m_Input.empty() && !(timing.InputTime >= 0 && timing.InputTime < 1e12)))
	{
		__sync_lock_test_and_set(&m_Failed, 1);
		return false;
	}

	if (m_Output.empty()) timing.OutputTime = timing.InputTime;

	if (m_PreviousTiming.Valid)
	{
		const double elapsed = timing.OutputTime - m_PreviousTiming.OutputTime;
		const double predicted = double(timing.Frame - m_PreviousTiming.Frame) / timing.SampleRate;
		if (timing.Frame <= m_PreviousTiming.Frame ||
			std::fabs(elapsed - predicted) > std::max(0.002, timing.Frames / timing.SampleRate * 0.5))
		{
			++m_NativeEpoch;
			m_InputRate.Reset();
			m_OutputRate.Reset();
		}

	}

	timing.Epoch = m_NativeEpoch;
	m_PreviousTiming = timing;
	double inputStep = m_InputRate.Observe(timing.Frame, timing.InputTime, timing.SampleRate);
	double outputStep = m_OutputRate.Observe(timing.Frame, timing.OutputTime, timing.SampleRate);
	if (timing.Step > 0 && std::fabs(timing.Step * timing.SampleRate - 1) < 0.01)
		inputStep = outputStep = timing.Step;

	timing.Step = m_Output.empty() ? inputStep : outputStep;
	m_Mailbox.Publish(timing);
	AudioStamp stamp;
	stamp.Frame = timing.Frame;
	stamp.Generation = __sync_fetch_and_add(&m_Generation, 0);
	stamp.Step = inputStep;
	if (!m_Input.empty() && !m_Client->Read(&m_Native[0], frames))
	{
		__sync_lock_test_and_set(&m_Failed, 1);
		return false;
	}

	if (!m_Input.empty())
		for (unsigned c = 0; c < m_Input.size(); ++c)
		{
			for (unsigned n = 0; n < frames; ++n) m_NativeMono[n] = m_Native[n * m_Input.size() + c];

			stamp.Time = m_Client->GetChannelTime(true, c);
			if (!m_Input[c]->Write(&m_NativeMono[0], frames, stamp)) __sync_fetch_and_add(&m_Errors, 1);

		}

	bool complete = true;
	for (unsigned c = 0; c < m_Output.size(); ++c)
	{
		const double time = m_Client->GetChannelTime(false, c);
		if (m_Output[c]->Read(&m_NativeMono[0], frames, time, outputStep, stamp.Generation) != frames) complete = false;

		for (unsigned n = 0; n < frames; ++n) m_Native[n * m_Output.size() + c] = m_NativeMono[n];

	}

	if (!complete) __sync_fetch_and_add(&m_Errors, 1);

	if (!m_Output.empty() && !m_Client->Write(&m_Native[0], frames))
	{
		__sync_lock_test_and_set(&m_Failed, 1);
		return false;
	}

	return true;
}

bool AudioStream::Playback(const float *samples, unsigned frames, const AudioStamp &stamp)
{
	if (!samples || frames > m_EngineFrames) return false;

	bool result = true;
	for (unsigned c = 0; c < m_Output.size(); ++c)
	{
		for (unsigned n = 0; n < frames; ++n) m_EngineMono[n] = samples[n * m_Output.size() + c];

		if (!m_Output[c]->Write(&m_EngineMono[0], frames, stamp)) result = false;

	}

	if (!result) __sync_fetch_and_add(&m_Errors, 1);

	return result;
}

void AudioStream::Capture(float *samples, unsigned frames, const AudioStamp &stamp)
{
	if (!samples || frames > m_EngineFrames) return;

	for (unsigned c = 0; c < m_Input.size(); ++c)
	{
		m_Input[c]->Read(&m_EngineMono[0], frames, stamp.Time, stamp.Step, stamp.Generation);
		for (unsigned n = 0; n < frames; ++n) samples[n * m_Input.size() + c] = m_EngineMono[n];

	}

}

bool AudioStream::Timing(AudioCycleTiming &timing)
{
	m_Mailbox.Read(m_LastTiming);
	timing = m_LastTiming;
	return timing.Valid;
}

double AudioStream::Lookahead() const
{
	double seconds = 0;
	for (unsigned n = 0; n < m_Output.size(); ++n)
		seconds = std::max(seconds, m_Output[n]->Lookahead() / m_EngineRate);

	for (unsigned n = 0; n < m_Input.size(); ++n)
		seconds = std::max(seconds, double(m_Input[n]->Lookahead()) / m_Client->GetSampleRate());

	return seconds;
}


bool AudioStream::Start()
{
	if (m_Running) return true;

	if (!m_Client || !m_Client->Start()) return false;

	if (m_Client->IsCallbackDriven()) return true;

	__sync_lock_test_and_set(&m_Stop, 0);
	m_Running = pthread_create(&m_Worker, NULL, Worker, this) == 0;
	return m_Running;
}

void AudioStream::Stop()
{
	if (!m_Running) return;

	__sync_lock_test_and_set(&m_Stop, 1);
	pthread_join(m_Worker, NULL);
	m_Running = false;
}

void *AudioStream::Worker(void *context)
{
	AudioStream *stream = static_cast<AudioStream *>(context);
	while (!__sync_fetch_and_add(&stream->m_Stop, 0))
	{
		const int ready = stream->m_Client->WaitForCycle(10);
		if (ready < 0 || (ready && !stream->Transfer(stream->m_Client->GetBufferSize())))
		{
			__sync_lock_test_and_set(&stream->m_Failed, 1);
			break;
		}

		// A driver that returns early must not spin while another duplex side
		// has no period ready. Shutdown is bounded even for a stalled device.
		if (!ready) usleep(1000);

	}

	return NULL;
}
