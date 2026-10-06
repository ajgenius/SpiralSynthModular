#include "OutputAudioClient.h"
#include "AudioTransportHub.h"

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <cstring>
#include <algorithm>
#include <iostream>
#include <sys/time.h>

#ifdef HAVE_CORE_AUDIO_CLIENT
#include "CoreAudioClient.h"
#endif

#ifdef HAVE_JACK_CLIENT
#include "JackClient.h"
#endif

#ifdef HAVE_OUTPUT_PORTAUDIO
#include "PortAudioClient.h"
#endif
#ifdef HAVE_OUTPUT_ALSA
#include "AlsaClient.h"
#endif
#ifdef HAVE_OUTPUT_OSS
#include "OSSClient.h"
#endif

using namespace std;
using namespace spiralcore;

OutputAudioClient *OutputAudioClient::m_Singleton = NULL;
const HostInfo *OutputAudioClient::host = NULL;

OutputAudioClient *OutputAudioClient::Get()
{
	if (!m_Singleton) m_Singleton = new OutputAudioClient;
	return m_Singleton;
}

void OutputAudioClient::PackUpAndGoHome()
{
	if (m_Singleton)
	{
		delete m_Singleton;
		m_Singleton = NULL;
	}
}

OutputAudioClient::OutputAudioClient() :
	m_Client(NULL),
	m_ClientName(""),
	m_Destination("default"),
	m_Volume(0.5f),
	m_Channels(2),
	m_Frames(0),
	m_IsDead(false)
{

}

OutputAudioClient::~OutputAudioClient()
{
	Close();
	DestroyBackend();
}

void OutputAudioClient::DestroyBackend()
{
	DeallocateBuffer();
#ifdef HAVE_OUTPUT_PORTAUDIO
	if (m_ClientName == "portaudio") PortAudioClient::PackUpAndGoHome();
#endif
#ifdef HAVE_OUTPUT_ALSA
	if (m_ClientName == "alsa") AlsaClient::PackUpAndGoHome();
#endif
#ifdef HAVE_OUTPUT_OSS
	if (m_ClientName == "oss") OSSClient::PackUpAndGoHome();

#endif
#ifdef HAVE_JACK_CLIENT
	if (m_ClientName == "jack") delete m_Client;

#endif
#ifdef HAVE_CORE_AUDIO_CLIENT
	if (m_ClientName == "coreaudio") delete m_Client;

#endif
	m_Client = NULL;
	m_ClientName.clear();
}

bool OutputAudioClient::Select(const string &client)
{
	DestroyBackend();
#ifdef HAVE_CORE_AUDIO_CLIENT
	if (client == "coreaudio")
	{
		m_Client = new CoreAudioClient;
		m_ClientName = "coreaudio";
		return true;
	}

#endif
#ifdef HAVE_JACK_CLIENT
	if (client == "jack")
	{
		m_Client = new JackClient;
		m_ClientName = "jack";
		return true;
	}

#endif
#ifdef HAVE_OUTPUT_PORTAUDIO
	if (client == "portaudio")
	{
		m_Client = PortAudioClient::Get();
		m_ClientName = "portaudio";
		return true;
	}
#endif
#ifdef HAVE_OUTPUT_ALSA
	if (client == "alsa")
	{
		m_Client = AlsaClient::Get();
		m_ClientName = "alsa";
		return true;
	}
#endif
#ifdef HAVE_OUTPUT_OSS
	if (client == "oss")
	{
		m_Client = OSSClient::Get();
		m_ClientName = "oss";
		return true;
	}
#endif
	return false;
}

bool OutputAudioClient::SelectFirstAvailable()
{
#ifdef HAVE_OUTPUT_PORTAUDIO
	if (Select("portaudio")) return true;
#endif
#ifdef HAVE_CORE_AUDIO_CLIENT
	if (Select("coreaudio")) return true;

#endif
#ifdef HAVE_OUTPUT_ALSA
	if (Select("alsa")) return true;
#endif
#ifdef HAVE_OUTPUT_OSS
	if (Select("oss")) return true;
#endif
	return false;
}

bool OutputAudioClient::Configure(const string &client, const string &destination)
{
	Close();
	string requested = client;
	if (requested.empty())
	{
#ifdef DEFAULT_OUTPUT_AUDIO_CLIENT
		requested = DEFAULT_OUTPUT_AUDIO_CLIENT;
#endif
	}
	if (!Select(requested))
	{
		if (!requested.empty())
			cerr << "OutputPlugin: audio client '" << requested
			     << "' is not available in this build; selecting a fallback" << endl;
		if (!SelectFirstAvailable())
		{
			cerr << "OutputPlugin: no audio output backend was configured at build time" << endl;
			return false;
		}
	}
	m_Destination = destination.empty() ? "default" : destination;
	m_IsDead = false;
	cerr << "OutputPlugin: using " << m_ClientName
	     << " backend, destination=" << m_Destination << endl;
	return true;
}

AudioClientOptions OutputAudioClient::MakeOptions(unsigned int inChans, unsigned int outChans) const
{
	AudioClientOptions opt;
	if (host)
	{
		opt.BufferSize = (unsigned int)host->BUFSIZE;
		opt.NumBuffers = host->FRAGCOUNT;
		opt.FragSize = (unsigned int)host->FRAGSIZE;
		opt.Samplerate = (unsigned int)host->SAMPLERATE;
	}
	opt.InChannels = inChans;
	opt.OutChannels = outChans;
	return opt;
}

void OutputAudioClient::AllocateBuffer()
{
	if (!host || host->BUFSIZE <= 0) return;

	m_Frames = host->BUFSIZE;
	m_Out.assign(m_Frames * m_Channels, 0);
	m_In.assign(m_Frames * m_Channels, 0);
}

void OutputAudioClient::DeallocateBuffer()
{
	m_Out.clear();
	m_In.clear();
	m_Frames = 0;
}

void OutputAudioClient::SendStereo(const Sample *ldata, const Sample *rdata)
{
	if (m_Channels != 2 || !host || m_Frames != host->BUFSIZE || m_Out.empty() || m_IsDead) return;

	int on = 0;
	for (int n = 0; n < host->BUFSIZE; ++n)
	{
		if (m_IsDead) return;
		float l = ldata ? (*ldata)[n] * m_Volume : 0.f;
		float r = rdata ? (*rdata)[n] * m_Volume : 0.f;

		m_Out[on++] += l;
		m_Out[on++] += r;
	}
}

void OutputAudioClient::GetStereo(Sample *ldata, Sample *rdata)
{
	if (m_Channels != 2 || !host || m_Frames != host->BUFSIZE || m_In.empty() || m_IsDead) return;

	int on = 0;
	for (int n = 0; n < host->BUFSIZE; ++n)
	{
		if (m_IsDead) return;
		if (ldata) ldata->Set(n, m_In[on] * m_Volume);
		on++;
		if (rdata) rdata->Set(n, m_In[on] * m_Volume);
		on++;
	}
}

void OutputAudioClient::BeginPeriod(const AudioStamp &capture)
{
	if (m_Out.empty()) return;

	std::fill(m_Out.begin(), m_Out.end(), 0);
	std::fill(m_In.begin(), m_In.end(), 0);
	m_Stream.Capture(&m_In[0], m_Frames, capture);
}

void OutputAudioClient::CommitPeriod(const AudioStamp &playback)
{
	if (!m_Out.empty() && IsAttached()) m_Stream.Playback(&m_Out[0], m_Frames, playback);

}

bool OutputAudioClient::AttachMode(unsigned int inChans, unsigned int outChans)
{
	if (!m_Client) return false;
	DeallocateBuffer();
	AllocateBuffer();
	if (m_Out.empty()) return false;
	m_IsDead = false;
	if (!m_Client->Attach(m_ClientName == "jack" && m_Destination == "default" ? "SSM-Output" : m_Destination,
		MakeOptions(inChans, outChans))) return false;

	if (!m_Stream.Configure(m_Client, inChans, outChans, m_Frames, host->SAMPLERATE))
	{
		m_Client->Detach();
		return false;
	}

	m_Client->SetCallback(AudioStream::Callback, &m_Stream);
	AudioTransportHub::Get()->RegisterStream(&m_Stream);
	if (!m_Stream.Start()) { Close(); return false; }

	return true;
}

bool OutputAudioClient::OpenWrite()     { Close(); return AttachMode(0, (unsigned int)m_Channels); }
bool OutputAudioClient::OpenRead()      { Close(); return AttachMode((unsigned int)m_Channels, 0); }
bool OutputAudioClient::OpenReadWrite() { Close(); return AttachMode((unsigned int)m_Channels, (unsigned int)m_Channels); }

bool OutputAudioClient::Close()
{
	AudioTransportHub::Get()->UnregisterStream(&m_Stream);
	m_Stream.Stop();
	if (m_Client) m_Client->Detach();

	return true;
}

void OutputAudioClient::Kill()
{
	m_IsDead = true;
	Close();
}
