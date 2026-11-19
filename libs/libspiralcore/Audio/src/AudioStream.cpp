// SPDX-License-Identifier: GPL-2.0-or-later
#include "AudioStream.h"
#include <algorithm>
#include <cstring>
#include <cmath>
#include <unistd.h>
#include <iostream>
using namespace Spiral::Core;

AudioStream::AudioStream() : m_OutputState(Playing), m_Running(false), m_Stop(0), m_Client(NULL), m_Capacity(0), m_EngineFrames(0), m_InvalidTiming(0), m_NativeEpoch(1),
	m_EngineRate(0), m_NativeRate(0), m_Errors(0), m_Failed(0), m_Generation(0), m_CallbackOutput(false)
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
	m_OutputState = Playing;
	m_LastTiming.Valid = false;
	m_PreviousTiming = AudioCycleTiming();
	++m_NativeEpoch;
	AudioCycleTiming unused;
	m_Mailbox.Read(unused);
	m_GraphStarts.Read(unused);
	m_GraphStart = AudioCycleTiming();
	m_Generation = 0;
	m_CallbackOutput = false;
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
	m_CallbackOutput = outputs && client->IsCallbackDriven();
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

void AudioStream::SetGeneration(unsigned value, double firstOutputTime)
{
	if (__sync_fetch_and_add(&m_Generation, 0) == value)
		return;

	if (m_CallbackOutput)
	{
		AudioCycleTiming start;
		start.Epoch = value;
		start.OutputTime = firstOutputTime;
		m_GraphStarts.Publish(start);
	}
	__sync_lock_test_and_set(&m_Generation, value);
}

void AudioStream::Callback(void *context, unsigned frames)
{
	AudioStream *stream = static_cast<AudioStream *>(context);
	if (frames) stream->Transfer(frames);

}

bool AudioStream::Transfer(unsigned frames)
{
	double startTime = -1;
	return Transfer(frames, startTime);
}

void AudioStream::BeginPatchReplacement()
{
	__sync_lock_test_and_set(&m_OutputState, Held);
}

void AudioStream::EndPatchReplacement()
{
	__sync_lock_test_and_set(&m_OutputState, m_Output.empty() ? Playing : WaitingForGraph);
}

bool AudioStream::FirstOutputTime(unsigned generation, double &first)
{
	bool ready = true;
	first = -1;
	for (unsigned c = 0; c < m_Output.size(); ++c)
	{
		double time;
		if (!m_Output[c]->FirstTime(generation, time))
			ready = false;
		else if (first < 0 || time < first)
			first = time;
	}
	return ready;
}

bool AudioStream::Transfer(unsigned frames, double &startTime)
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

	timing.Epoch = m_NativeEpoch;
	timing.PlaybackRestart = m_PreviousTiming.PlaybackRestart;
	if (m_PreviousTiming.Valid)
	{
		const double elapsed = timing.OutputTime - m_PreviousTiming.OutputTime;
		const double predicted = double(timing.Frame - m_PreviousTiming.Frame) / timing.SampleRate;
		if (timing.Frame <= m_PreviousTiming.Frame ||
			std::fabs(elapsed - predicted) > std::max(0.002, timing.Frames / timing.SampleRate * 0.5))
		{
			timing.PlaybackRestart = false;
			++timing.Epoch;
			m_InputRate.Reset();
			m_OutputRate.Reset();
		}

	}

	double inputStep = m_InputRate.Observe(timing.Frame, timing.InputTime, timing.SampleRate);
	double outputStep = m_OutputRate.Observe(timing.Frame, timing.OutputTime, timing.SampleRate);
	if (timing.Step > 0 && std::fabs(timing.Step * timing.SampleRate - 1) < 0.01)
		inputStep = outputStep = timing.Step;

	timing.Step = m_Output.empty() ? inputStep : outputStep;
	AudioStamp stamp;
	stamp.Frame = timing.Frame;
	stamp.Step = inputStep;
	if (!m_Input.empty() && !m_Client->Read(&m_Native[0], frames))
	{
		__sync_lock_test_and_set(&m_Failed, 1);
		return false;
	}

	// The new graph resets presentation time and publishes all channels before
	// GraphReady. Preserve a silent prefix up to its first scheduled sample.
	const int outputState = __sync_fetch_and_add(&m_OutputState, 0);
	if (m_CallbackOutput)
	{
		// JACK can call before the graph has a clock, and routing can change
		// that clock's latency. Take the epoch and its first DAC time together.
		m_GraphStarts.Read(m_GraphStart);
		stamp.Generation = m_GraphStart.Epoch;
		startTime = m_GraphStart.OutputTime;
	}
	else
		stamp.Generation = __sync_fetch_and_add(&m_Generation, 0);

	bool silence = outputState != Playing || (m_CallbackOutput && !stamp.Generation);
	if (outputState == WaitingForGraph || outputState == GraphReady)
	{
		double first;
		if (FirstOutputTime(stamp.Generation, first) && outputState == GraphReady)
		{
			startTime = first;
			silence = false;
		}
	}

	if (!m_Input.empty() && !silence)
		for (unsigned c = 0; c < m_Input.size(); ++c)
		{
			for (unsigned n = 0; n < frames; ++n) m_NativeMono[n] = m_Native[n * m_Input.size() + c];

			stamp.Time = m_Client->GetChannelTime(true, c);
			if (!m_Input[c]->Write(&m_NativeMono[0], frames, stamp)) __sync_fetch_and_add(&m_Errors, 1);

		}

	bool complete = true;
	bool pastStart = true;
	for (unsigned c = 0; c < m_Output.size(); ++c)
	{
		const double time = m_Client->GetChannelTime(false, c);
		unsigned prefix = 0;
		if (silence)
		{
			std::fill(m_NativeMono.begin(), m_NativeMono.begin() + frames, 0);
			prefix = frames;
		}
		else if (startTime > time)
		{
			// The first scheduled sample can fall inside a hardware period.
			while (prefix < frames && time + prefix * outputStep + 1e-10 < startTime)
				m_NativeMono[prefix++] = 0;
			if (prefix == frames)
				pastStart = false;
		}

		unsigned count = frames - prefix;
		if (count && m_Output[c]->Read(&m_NativeMono[prefix], count,
			time + prefix * outputStep, outputStep, stamp.Generation) != count)
			complete = false;

		for (unsigned n = 0; n < frames; ++n) m_Native[n * m_Output.size() + c] = m_NativeMono[n];

	}

	if (pastStart && !silence)
	{
		startTime = -1;
		if (outputState == GraphReady)
			__sync_bool_compare_and_swap(&m_OutputState, GraphReady, Playing);
	}

	if (!m_Output.empty() && !m_Client->Write(&m_Native[0], frames))
	{
		PlaybackRecovery *recovery = dynamic_cast<PlaybackRecovery *>(m_Client);
		if (!recovery || !recovery->RecoverPlaybackWrite())
		{
			if (!complete) __sync_fetch_and_add(&m_Errors, 1);
			__sync_lock_test_and_set(&m_Failed, 1);
			return false;
		}

		// The PCM accepted none of this period. Fetch a new timestamp and
		// consume fresh audio before writing to the restarted device.
		++m_NativeEpoch;
		m_PreviousTiming.Valid = false;
		m_PreviousTiming.PlaybackRestart = true;
		m_InputRate.Reset();
		m_OutputRate.Reset();
		__sync_fetch_and_add(&m_Errors, 1);
		return true;
	}

	// Only completed transfers establish the native presentation clock.
	m_NativeEpoch = timing.Epoch;
	m_PreviousTiming = timing;
	m_Mailbox.Publish(timing);
	if (!complete) __sync_fetch_and_add(&m_Errors, 1);
	return true;
}

bool AudioStream::Playback(const float *samples, unsigned frames, const AudioStamp &stamp)
{
	if (!samples || frames > m_EngineFrames) return false;

	const int state = __sync_fetch_and_add(&m_OutputState, 0);
	if (state == Held)
		return true;

	bool result = true;
	for (unsigned c = 0; c < m_Output.size(); ++c)
	{
		for (unsigned n = 0; n < frames; ++n) m_EngineMono[n] = samples[n * m_Output.size() + c];

		if (!m_Output[c]->Write(&m_EngineMono[0], frames, stamp)) result = false;

	}

	if (!result) __sync_fetch_and_add(&m_Errors, 1);
	else if (state == WaitingForGraph)
		__sync_bool_compare_and_swap(&m_OutputState, WaitingForGraph, GraphReady);

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


bool AudioStream::PublishPreparedTiming()
{
	PreparedPlayback *prepared = dynamic_cast<PreparedPlayback *>(m_Client);
	AudioCycleTiming timing;
	if (!prepared || !prepared->PreparedTiming(timing))
		return false;

	timing.PlaybackRestart = false;
	timing.Epoch = m_NativeEpoch;
	m_Mailbox.Publish(timing);
	return true;
}

bool AudioStream::Start()
{
	if (m_Running) return true;

	if (!m_Client || !m_Client->Start()) return false;

	if (m_Client->IsCallbackDriven()) return true;

	// Give the graph its presentation clock before the worker can start the PCM.
	PublishPreparedTiming();
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
	double startTime = -1;
	if (stream->m_Input.empty() && !stream->m_Output.empty() && stream->PublishPreparedTiming())
	{
		// Wait for all channels before the first write. Each consumer also
		// makes room for the new timeline if an old generation filled it.
		while (!__sync_fetch_and_add(&stream->m_Stop, 0))
		{
			const unsigned generation = __sync_fetch_and_add(&stream->m_Generation, 0);
			if (__sync_fetch_and_add(&stream->m_OutputState, 0) == Held ||
			    stream->FirstOutputTime(generation, startTime))
				break;

			usleep(250);
			stream->PublishPreparedTiming();
		}
	}

	while (!__sync_fetch_and_add(&stream->m_Stop, 0))
	{
		const int ready = stream->m_Client->WaitForCycle(10);
		if (ready < 0 || (ready && !stream->Transfer(stream->m_Client->GetBufferSize(), startTime)))
		{
			__sync_lock_test_and_set(&stream->m_Failed, 1);
			break;
		}

		if (!ready) usleep(1000);

	}

	return NULL;
}
