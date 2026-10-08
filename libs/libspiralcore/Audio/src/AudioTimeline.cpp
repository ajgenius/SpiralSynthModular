// SPDX-License-Identifier: GPL-2.0-or-later
#include "AudioTimeline.h"
#include <algorithm>
using namespace std;
using namespace spiralcore;

AudioTimeline::AudioTimeline() : m_Master(NULL), m_Frame(0), m_Rolling(true) {}

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

AudioClient *AudioTimeline::MasterClient() const
{
	return m_Master ? m_Master->Client() : NULL;
}

bool AudioTimeline::Prepare(unsigned frames, double rate, double now, AudioStream *preferred)
{
	if (!frames || !(rate >= 8000 && rate <= 384000)) return false;

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
		m_Streams[n]->SetGeneration(m_PlaybackStamp.Generation);

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
	AudioClient *client = MasterClient();
	if (client && client->GetTransport(frame, rolling))
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
	if (!(MasterClient() && MasterClient()->StartTransport())) m_Rolling=true;
}

void AudioTimeline::Stop()
{
	if (!(MasterClient() && MasterClient()->StopTransport())) m_Rolling=false;
}

void AudioTimeline::Locate(unsigned long frame)
{
	m_Presentation.Reset();
	if (!(MasterClient() && MasterClient()->LocateTransport(frame))) m_Frame=frame;
}

