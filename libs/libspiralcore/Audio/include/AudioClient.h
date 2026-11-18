// Copyright (C) 2003 David Griffiths <dave@pawfal.org>
// SSM blocking-output adaptation (Grok Build).
//

#ifndef SPIRALCORE_AUDIO_CLIENT
#define SPIRALCORE_AUDIO_CLIENT

#include <string>
#include "AudioTiming.h"

namespace Spiral
{
namespace Core
{

struct AudioClientOptions
{
	unsigned int BufferSize;
	int NumBuffers;
	unsigned int FragSize;
	unsigned int Samplerate;
	unsigned int InChannels;
	unsigned int OutChannels;

	AudioClientOptions() :
		BufferSize(512),
		NumBuffers(8),
		FragSize(256),
		Samplerate(44100),
		InChannels(0),
		OutChannels(2)
	{}
};

// A playback-only endpoint whose hardware clock starts on the first write.
class PreparedPlayback
{
public:
	virtual ~PreparedPlayback() {}
	virtual bool PreparedTiming(AudioCycleTiming &timing) const = 0;
};

class AudioClient
{
public:
	virtual ~AudioClient() {}

	virtual bool Attach(const std::string &device, const AudioClientOptions &opt) = 0;
	virtual void Detach() = 0;
	virtual bool IsAttached() const = 0;

	virtual bool IsCallbackDriven() const { return false; }

	virtual void SetCallback(void (*)(void *, unsigned int), void *) {}

	virtual bool Start() { return IsAttached(); }

	virtual int WaitForCycle(unsigned milliseconds) { return -1; }

	virtual unsigned long GetBufferSize() const { return 0; }

	virtual unsigned long GetSampleRate() const { return 0; }

	virtual bool GetCycleTiming(AudioCycleTiming &timing) const { return false; }
	virtual double GetChannelTime(bool input, unsigned channel) const
	{
		AudioCycleTiming timing;
		return GetCycleTiming(timing) ? (input ? timing.InputTime : timing.OutputTime) : 0;
	}

	virtual double GetInputLatency() const { return 0; }
	virtual double GetOutputLatency() const { return 0; }

	virtual bool Write(const float *interleaved, unsigned int nframes) = 0;
	virtual bool Read(float *interleaved, unsigned int nframes) = 0;

	virtual bool GetTransport(unsigned long &frame, bool &rolling) const { return false; }
	virtual bool StartTransport() { return false; }
	virtual bool StopTransport() { return false; }
	virtual bool LocateTransport(unsigned long frame) { return false; }
};

}
}

#endif
