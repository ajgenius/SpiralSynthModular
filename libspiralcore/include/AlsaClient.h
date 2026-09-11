// Copyright (C) 2003 David Griffiths <dave@pawfal.org>
// SSM blocking ALSA PCM client, same AudioClient shape as PortAudioClient.

#ifndef SPIRALCORE_ALSA_CLIENT
#define SPIRALCORE_ALSA_CLIENT

#include <string>
#include <alsa/asoundlib.h>
#include "AudioClient.h"

namespace spiralcore
{

class AlsaClient : public AudioClient
{
public:
	static AlsaClient *Get();
	static void PackUpAndGoHome();

	bool Attach(const std::string &device, const AudioClientOptions &opt);
	void Detach();
	bool IsAttached() const { return m_Playback != NULL || m_Capture != NULL; }
	bool Write(const float *interleaved, unsigned int nframes);
	bool Read(float *interleaved, unsigned int nframes);

protected:
	AlsaClient();
	~AlsaClient();

private:
	AlsaClient(const AlsaClient &);
	AlsaClient &operator=(const AlsaClient &);

	bool OpenStream(snd_pcm_t **slot, snd_pcm_stream_t stream);

	static AlsaClient *m_Singleton;
	snd_pcm_t *m_Playback;
	snd_pcm_t *m_Capture;
	int m_Channels;
	unsigned int m_Samplerate;
	std::string m_Device;
};

}

#endif
