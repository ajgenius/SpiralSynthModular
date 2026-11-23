// SPDX-License-Identifier: GPL-2.0-or-later
#include "AudioTimeline.h"
#include <algorithm>
#include <limits>
using namespace std;
using namespace Spiral::Core;

AudioTimeline::AudioTimeline() : m_ExternalTransport(false), m_Master(NULL), m_Frame(0), m_Rolling(true), m_Rate(0) {}

void AudioTimeline::Reset()
{
	m_Presentation.Reset();
}

void AudioTimeline::Register(AudioStream *stream)
{
	if (find(m_Streams.begin(), m_Streams.end(), stream) != m_Streams.end()) return;

	m_Streams.push_back(stream);
	m_Presentation.Reset();
}

void AudioTimeline::Unregister(AudioStream *stream)
{
	vector<AudioStream *>::iterator entry = find(m_Streams.begin(), m_Streams.end(), stream);
	if (entry == m_Streams.end()) return;

	m_Streams.erase(entry);
	if (m_Master == stream) m_Master = NULL;

	m_Presentation.Reset();
}

static unsigned long TransportFrames(unsigned long frame, double from, double to)
{
	if (from <= 0 || to <= 0 || from == to) return frame;
	const long double scaled = static_cast<long double>(frame) * to / from + 0.5L;
	const unsigned long limit = std::numeric_limits<unsigned long>::max();
	return scaled >= limit ? limit : static_cast<unsigned long>(scaled);
}

AudioClient *AudioTimeline::TransportClient(unsigned long &frame, bool &rolling) const
{
	if (!m_ExternalTransport)
	{
		AudioClient *client = m_Master ? m_Master->Client() : NULL;
		return client && client->GetTransport(frame, rolling) ? client : NULL;
	}
	// JACK implements the optional transport API. A separate audio clock may still pace the graph.
	for (size_t i = 0; i < m_Streams.size(); ++i)
	{
		AudioClient *client = m_Streams[i]->Client();
		if (!client || !client->IsAttached() || m_Streams[i]->Failed()) continue;
		if (client->GetTransport(frame, rolling))
		{
			frame = TransportFrames(frame, client->GetSampleRate(), m_Rate);
			return client;
		}
	}
	return NULL;
}

bool AudioTimeline::Prepare(unsigned frames, double rate, double now, AudioStream *preferred)
{
	if (!frames || !(rate >= 8000 && rate <= 384000)) return false;

	m_Rate = rate;
	const double period = double(frames) / rate;
	double nativePeriod = period, outputLatency = 0, inputLatency = 0, lookahead = 0;
	AudioStream *master = NULL;
	AudioCycleTiming masterTiming;
	for (unsigned n = 0; n < m_Streams.size(); ++n)
	{
		AudioStream *stream = m_Streams[n];
		AudioClient *client = stream->Client();
		if (!client || !client->IsAttached() || stream->Failed()) continue;

		const double nativeRate = client->GetSampleRate();
		if (nativeRate < 8000 || nativeRate > 384000 || !client->GetBufferSize()) continue;

		nativePeriod = max(nativePeriod, double(client->GetBufferSize()) / nativeRate);
		outputLatency = max(outputLatency, client->GetOutputLatency());
		inputLatency = max(inputLatency, client->GetInputLatency());
		lookahead = max(lookahead, stream->Lookahead());
		AudioCycleTiming timing;
		if (!stream->Timing(timing) || now - timing.CallbackTime >= 0.25) continue;

		if (!master || (masterTiming.Estimated && !timing.Estimated) ||
			(masterTiming.Estimated == timing.Estimated && stream == preferred))
		{
			master = stream;
			masterTiming = timing;
		}

	}

	if (master != m_Master)
	{
		m_Master = master;
		m_Presentation.Reset();
	}

	const double ahead = max(0.005, outputLatency + 2 * nativePeriod + 2 * period + lookahead);
	if (!m_Presentation.Prepare(now, frames, rate,
		master ? &masterTiming : NULL, ahead, m_PlaybackStamp)) return false;

	m_CaptureStamp = m_PlaybackStamp;
	m_CaptureStamp.Time -= ahead + inputLatency + 2 * nativePeriod + lookahead;
	for (unsigned n = 0; n < m_Streams.size(); ++n)
		m_Streams[n]->SetGeneration(m_PlaybackStamp.Generation, m_PlaybackStamp.Time);

	return true;
}

unsigned AudioTimeline::SleepMicroseconds() const
{
	return unsigned(min(0.001, max(0.00005, m_Presentation.Wait())) * 1000000);
}

void AudioTimeline::Begin()
{
	unsigned long frame;
	bool rolling;
	if (TransportClient(frame, rolling))
	{
		m_Frame = frame;
		m_Rolling = rolling;
	}
}

void AudioTimeline::Commit(unsigned frames)
{
	m_Presentation.Commit(frames);
	if (m_Rolling)
		m_Frame += frames;
}

void AudioTimeline::Start()
{
	unsigned long position;
	bool rolling;
	AudioClient *client = m_ExternalTransport ? TransportClient(position, rolling) : (m_Master ? m_Master->Client() : NULL);
	if (!client || !client->StartTransport()) m_Rolling=true;
}

void AudioTimeline::Stop()
{
	unsigned long position;
	bool rolling;
	AudioClient *client = m_ExternalTransport ? TransportClient(position, rolling) : (m_Master ? m_Master->Client() : NULL);
	if (!client || !client->StopTransport()) m_Rolling=false;
}

void AudioTimeline::Locate(unsigned long frame)
{
	m_Presentation.Reset();
	unsigned long position;
	bool rolling;
	AudioClient *client = m_ExternalTransport ? TransportClient(position, rolling) : (m_Master ? m_Master->Client() : NULL);
	if (!client || !client->LocateTransport(m_ExternalTransport ? TransportFrames(frame, m_Rate, client->GetSampleRate()) : frame))
		m_Frame = frame;
}

