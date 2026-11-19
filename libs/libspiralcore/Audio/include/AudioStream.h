// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SPIRALCORE_AUDIO_STREAM_H
#define SPIRALCORE_AUDIO_STREAM_H
#include "AudioClient.h"
#include "TimedAudioBuffer.h"
#include <vector>
#include <pthread.h>

namespace Spiral
{
namespace Core
{
class AudioStream
{
public:
	AudioStream();
	~AudioStream();
	bool Configure(AudioClient *client, unsigned inputs, unsigned outputs,
		unsigned engineFrames, double engineRate);
	bool Start();
	void Stop();
	// Called with graph processing excluded. The native worker keeps its clock
	// and sends complete silent periods until the replacement supplies audio.
	void BeginPatchReplacement();
	void EndPatchReplacement();
	static void Callback(void *context, unsigned frames);
	bool Transfer(unsigned frames);
	bool Playback(const float *samples, unsigned frames, const AudioStamp &stamp);
	void Capture(float *samples, unsigned frames, const AudioStamp &stamp);
	// The first block of a generation sets its physical presentation time.
	void SetGeneration(unsigned value, double firstOutputTime = 0);
	bool Timing(AudioCycleTiming &timing);
	AudioClient *Client() const { return m_Client; }
	unsigned Capacity() const { return m_Capacity; }
	unsigned Errors() const { return __sync_fetch_and_add(&m_Errors, 0); }
	bool Failed() const { return __sync_fetch_and_add(&m_Failed, 0) != 0; }
	double Lookahead() const;

private:
	void Clear();
	bool PublishPreparedTiming();
	bool FirstOutputTime(unsigned generation, double &time);
	enum OutputState { Playing, Held, WaitingForGraph, GraphReady };
	int m_OutputState;
	bool Transfer(unsigned frames, double &startTime);
	static void *Worker(void *context);
	pthread_t m_Worker;
	bool m_Running;
	int m_Stop;
	AudioClient *m_Client;
	std::vector<TimedAudioBuffer *> m_Input, m_Output;
	std::vector<float> m_Native, m_NativeMono, m_EngineMono;
	unsigned m_Capacity, m_EngineFrames, m_InvalidTiming, m_NativeEpoch;
	double m_EngineRate, m_NativeRate;
	mutable unsigned m_Errors;
	mutable int m_Failed;
	unsigned m_Generation;
	AudioTimingMailbox m_Mailbox;
	bool m_CallbackOutput;
	AudioTimingMailbox m_GraphStarts;
	AudioCycleTiming m_GraphStart;
	AudioCycleTiming m_LastTiming, m_PreviousTiming;
	AudioRateEstimator m_InputRate, m_OutputRate;
	AudioStream(const AudioStream &);
	AudioStream &operator=(const AudioStream &);
};
}
}
#endif
