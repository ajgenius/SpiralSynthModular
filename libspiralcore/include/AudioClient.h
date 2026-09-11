// Copyright (C) 2003 David Griffiths <dave@pawfal.org>
// SSM blocking-output adaptation (Grok Build).
//
// Common device-paced audio I/O for PortAudio / ALSA / OSS.
// Attach/Detach match JackClient/PortAudioClient; Write/Read are
// blocking interleaved float (the engine clock), not a callback.

#ifndef SPIRALCORE_AUDIO_CLIENT
#define SPIRALCORE_AUDIO_CLIENT

#include <string>

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

	/* Blocking, interleaved float32. nframes is the engine block. */
	virtual bool Write(const float *interleaved, unsigned int nframes) = 0;
	virtual bool Read(float *interleaved, unsigned int nframes) = 0;
};

}

#endif
