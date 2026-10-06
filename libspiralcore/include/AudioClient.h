// Copyright (C) 2003 David Griffiths <dave@pawfal.org>
// SSM blocking-output adaptation (Grok Build).
//
// Device-paced or callback-driven audio I/O. Existing blocking clients keep
// their Attach/Read/Write behavior; callback clients start after host setup.

#ifndef SPIRALCORE_AUDIO_CLIENT
#define SPIRALCORE_AUDIO_CLIENT

#include <string>
#include "AudioTiming.h"

namespace spiralcore
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

class AudioClient
{
public:
	virtual ~AudioClient() {}

	virtual bool Attach(const std::string &device, const AudioClientOptions &opt) = 0;
	virtual void Detach() = 0;
	virtual bool IsAttached() const = 0;

	// Attach prepares a callback client without running the engine. Install the
	// callback and agree on its actual format before Start. Detach must run on
	// the control path, never from the device callback it waits to finish.
	virtual bool IsCallbackDriven() const { return false; }

	virtual void SetCallback(void (*)(void *, unsigned int), void *) {}

	virtual bool Start() { return IsAttached(); }

	// Non-callback clients prepare one nonblocking I/O cycle on a dedicated
	// worker. Return 1 for ready, 0 for timeout, -1 for a failed device.
	virtual int WaitForCycle(unsigned milliseconds) { return -1; }

	virtual unsigned long GetBufferSize() const { return 0; }

	virtual unsigned long GetSampleRate() const { return 0; }

	// Current native I/O cycle, read only on its callback/transport thread.
	// Times are mapped to AudioMonotonicTime and refer to ADC/DAC presentation,
	// not callback arrival or musical transport. False means unavailable.
	virtual bool GetCycleTiming(AudioCycleTiming &timing) const { return false; }
	virtual double GetChannelTime(bool input, unsigned channel) const
	{
		AudioCycleTiming timing;
		return GetCycleTiming(timing) ? (input ? timing.InputTime : timing.OutputTime) : 0;
	}

	virtual double GetInputLatency() const { return 0; }
	virtual double GetOutputLatency() const { return 0; }

	// Interleaved float32. Blocking clients pace the caller; callback clients
	// exchange only the current cycle's buffers from inside their callback.

	virtual bool Write(const float *interleaved, unsigned int nframes) = 0;
	virtual bool Read(float *interleaved, unsigned int nframes) = 0;

	// A device that carries a transport (JACK) reports its position and the
	// engine slaves to it; otherwise the engine keeps its own.
	virtual bool GetTransport(unsigned long &frame, bool &rolling) const { return false; }
	virtual bool StartTransport() { return false; }
	virtual bool StopTransport() { return false; }
	virtual bool LocateTransport(unsigned long frame) { return false; }
};

}

#endif
