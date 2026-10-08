// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SPIRALCORE_AUDIO_STREAM_H
#define SPIRALCORE_AUDIO_STREAM_H
#include "AudioClient.h"
#include "TimedAudioBuffer.h"
#include <vector>
#include <pthread.h>

namespace spiralcore
{
// Borrows a prepared native client. The owner stops callbacks before Configure
// or destruction. Engine and callback own separate scratch and queue ends.
class AudioStream
{
public:
	AudioStream();
	~AudioStream();
	bool Configure(AudioClient *client, unsigned inputs, unsigned outputs,
		unsigned engineFrames, double engineRate);
	bool Start();
	void Stop();
	static void Callback(void *context, unsigned frames);
	bool Transfer(unsigned frames);
	bool Playback(const float *samples, unsigned frames, const AudioStamp &stamp);
	void Capture(float *samples, unsigned frames, const AudioStamp &stamp);
	void SetGeneration(unsigned value) { __sync_lock_test_and_set(&m_Generation, value); }
	bool Timing(AudioCycleTiming &timing);
	AudioClient *Client() const { return m_Client; }
	unsigned Capacity() const { return m_Capacity; }
	unsigned Errors() const { return __sync_fetch_and_add(&m_Errors, 0); }
	bool Failed() const { return __sync_fetch_and_add(&m_Failed, 0) != 0; }
	double Lookahead() const;

private:
	void Clear();
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
	AudioCycleTiming m_LastTiming, m_PreviousTiming;
	AudioRateEstimator m_InputRate, m_OutputRate;
	AudioStream(const AudioStream &);
	AudioStream &operator=(const AudioStream &);
};
}
#endif
