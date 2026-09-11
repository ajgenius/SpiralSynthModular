#include "OutputAudioClient.h"

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <cstring>
#include <iostream>

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
	m_WriteBuf(0),
	m_ReadBuf(0),
	m_IsDead(false)
{
	m_Out[0] = m_Out[1] = m_In[0] = m_In[1] = NULL;
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
	m_Client = NULL;
	m_ClientName.clear();
}

bool OutputAudioClient::Select(const string &client)
{
	DestroyBackend();
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
	if (!host) return;
	const int frames = host->BUFSIZE;
	if (m_Out[0] && m_Frames == frames) return;
	DeallocateBuffer();
	m_Frames = frames;
	const int samples = frames * m_Channels;
	m_Out[0] = new float[samples];
	m_Out[1] = new float[samples];
	m_In[0] = new float[samples];
	m_In[1] = new float[samples];
	memset(m_Out[0], 0, samples * sizeof(float));
	memset(m_Out[1], 0, samples * sizeof(float));
	memset(m_In[0], 0, samples * sizeof(float));
	memset(m_In[1], 0, samples * sizeof(float));
}

void OutputAudioClient::DeallocateBuffer()
{
	delete [] m_Out[0]; delete [] m_Out[1];
	delete [] m_In[0]; delete [] m_In[1];
	m_Out[0] = m_Out[1] = m_In[0] = m_In[1] = NULL;
	m_Frames = 0;
}

void OutputAudioClient::SendStereo(const Sample *ldata, const Sample *rdata)
{
	if (m_Channels != 2 || !host || !m_Out[m_WriteBuf] || m_IsDead) return;
	int on = 0;
	for (int n = 0; n < host->BUFSIZE; ++n)
	{
		if (m_IsDead) return;
		float l = ldata ? (*ldata)[n] * m_Volume : 0.f;
		float r = rdata ? (*rdata)[n] * m_Volume : 0.f;
		if (l > 1) l = 1; if (l < -1) l = -1;
		if (r > 1) r = 1; if (r < -1) r = -1;
		m_Out[m_WriteBuf][on++] += l;
		m_Out[m_WriteBuf][on++] += r;
	}
}

void OutputAudioClient::GetStereo(Sample *ldata, Sample *rdata)
{
	if (m_Channels != 2 || !host || !m_In[m_ReadBuf] || m_IsDead) return;
	int on = 0;
	for (int n = 0; n < host->BUFSIZE; ++n)
	{
		if (m_IsDead) return;
		if (ldata) ldata->Set(n, m_In[m_ReadBuf][on] * m_Volume);
		on++;
		if (rdata) rdata->Set(n, m_In[m_ReadBuf][on] * m_Volume);
		on++;
	}
}

void OutputAudioClient::Play()
{
	if (!host || !m_Out[0]) return;
	const int send = !m_WriteBuf;
	const int samples = host->BUFSIZE * m_Channels;
	if (m_Client)
		m_Client->Write(m_Out[send], (unsigned int)host->BUFSIZE);
	memset(m_Out[send], 0, samples * sizeof(float));
	m_WriteBuf = send;
}

void OutputAudioClient::Read()
{
	if (!host || !m_In[0]) return;
	const int got = !m_ReadBuf;
	const int samples = host->BUFSIZE * m_Channels;
	memset(m_In[got], 0, samples * sizeof(float));
	if (m_Client)
		m_Client->Read(m_In[got], (unsigned int)host->BUFSIZE);
	m_ReadBuf = got;
}

bool OutputAudioClient::AttachMode(unsigned int inChans, unsigned int outChans)
{
	if (!m_Client) return false;
	AllocateBuffer();
	m_IsDead = false;
	return m_Client->Attach(m_Destination, MakeOptions(inChans, outChans));
}

bool OutputAudioClient::OpenWrite()     { Close(); return AttachMode(0, (unsigned int)m_Channels); }
bool OutputAudioClient::OpenRead()      { Close(); return AttachMode((unsigned int)m_Channels, 0); }
bool OutputAudioClient::OpenReadWrite() { Close(); return AttachMode((unsigned int)m_Channels, (unsigned int)m_Channels); }

bool OutputAudioClient::Close()
{
	if (m_Client) m_Client->Detach();
	return true;
}

void OutputAudioClient::Kill()
{
	m_IsDead = true;
	Close();
}
