// Copyright (C) 2003 David Griffiths <dave@pawfal.org>
//
// Adapted on the FLTK / UA tree: blocking Pa_WriteStream / Pa_ReadStream
// (PortAudio v19) instead of the original callback Process().  UA's
// engine is paced by the device write; a callback would be a second clock.

#ifndef PA_CLIENT
#define PA_CLIENT

#include <string>
#include <portaudio.h>
#include "AudioClient.h"

namespace spiralcore
{

class PortAudioClient : public AudioClient
{
public:
	static PortAudioClient *Get();
	static void PackUpAndGoHome();

	typedef AudioClientOptions DeviceOptions;

	bool Attach(const std::string &device, const AudioClientOptions &opt);
	void Detach();
	bool IsAttached() const { return m_Attached; }
	bool Write(const float *interleaved, unsigned int nframes);
	bool Read(float *interleaved, unsigned int nframes);

protected:
	PortAudioClient();
	~PortAudioClient();

private:
	PortAudioClient(const PortAudioClient &);
	PortAudioClient &operator=(const PortAudioClient &);

	bool Check(PaError err, const char *op) const;
	PaDeviceIndex FindDevice(bool input) const;
	bool FillParameters(PaStreamParameters &params, bool input) const;

	static PortAudioClient *m_Singleton;
	PaStream *m_Stream;
	bool m_Attached;
	bool m_Initialized;
	bool m_HasInput;
	bool m_HasOutput;
	int m_Channels;
	AudioClientOptions m_Opt;
	std::string m_Device;
};

}

#endif
