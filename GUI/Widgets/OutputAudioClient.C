#include "OutputAudioClient.h"

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <cstring>
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
	m_Mix(0),
	m_Send(0),
	m_Underruns(0),
	m_IsDead(false)
{
	m_Out[0] = m_Out[1] = m_In[0] = m_In[1] = NULL;
	m_Ready[0] = m_Ready[1] = false;
	pthread_mutex_init(&m_PeriodLock, NULL);
	pthread_cond_init(&m_PeriodFree, NULL);
	pthread_cond_init(&m_PeriodReady, NULL);
}

OutputAudioClient::~OutputAudioClient()
{
	Close();
	DestroyBackend();
	pthread_cond_destroy(&m_PeriodReady);
	pthread_cond_destroy(&m_PeriodFree);
	pthread_mutex_destroy(&m_PeriodLock);
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
	if (!host) return;
	const int frames = host->BUFSIZE;
	if (m_Out[0] && m_Frames == frames) return;
	DeallocateBuffer();
	if (frames <= 0) return;
	m_Mix = m_Send = 0;
	m_Ready[0] = m_Ready[1] = false;
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
	if (m_Channels != 2 || !host || m_Frames != host->BUFSIZE || !m_Out[m_Mix] || m_IsDead) return;

	int on = 0;
	for (int n = 0; n < host->BUFSIZE; ++n)
	{
		if (m_IsDead) return;
		float l = ldata ? (*ldata)[n] * m_Volume : 0.f;
		float r = rdata ? (*rdata)[n] * m_Volume : 0.f;

		m_Out[m_Mix][on++] += l;
		m_Out[m_Mix][on++] += r;
	}
}

void OutputAudioClient::GetStereo(Sample *ldata, Sample *rdata)
{
	// The slot the engine owns is the one the transport has finished with;
	// its capture is complete. The other slot may still be filling.
	const int captured = m_Mix;
	if (m_Channels != 2 || !host || m_Frames != host->BUFSIZE || !m_In[captured] || m_IsDead) return;

	int on = 0;
	for (int n = 0; n < host->BUFSIZE; ++n)
	{
		if (m_IsDead) return;
		if (ldata) ldata->Set(n, m_In[captured][on] * m_Volume);
		on++;
		if (rdata) rdata->Set(n, m_In[captured][on] * m_Volume);
		on++;
	}
}

static void Deadline(struct timespec &deadline, unsigned microseconds)
{
	struct timeval now;
	gettimeofday(&now, NULL);
	unsigned long long nanoseconds = (unsigned long long)now.tv_usec * 1000 + (unsigned long long)microseconds * 1000;
	deadline.tv_sec = now.tv_sec + nanoseconds / 1000000000ULL;
	deadline.tv_nsec = nanoseconds % 1000000000ULL;
}

bool OutputAudioClient::WaitPeriod(unsigned microseconds)
{
	struct timespec deadline;
	Deadline(deadline, microseconds);
	pthread_mutex_lock(&m_PeriodLock);
	while (m_Ready[m_Mix] && !m_IsDead)
		if (pthread_cond_timedwait(&m_PeriodFree, &m_PeriodLock, &deadline)) break;
	const bool free = !m_Ready[m_Mix];
	pthread_mutex_unlock(&m_PeriodLock);
	return free;
}

void OutputAudioClient::CommitPeriod()
{
	if (!m_Out[0]) return;
	pthread_mutex_lock(&m_PeriodLock);
	__sync_synchronize();
	m_Ready[m_Mix] = true;
	m_Mix = !m_Mix;
	pthread_cond_signal(&m_PeriodReady);
	pthread_mutex_unlock(&m_PeriodLock);
}

bool OutputAudioClient::WaitReady(unsigned microseconds)
{
	struct timespec deadline;
	Deadline(deadline, microseconds);
	pthread_mutex_lock(&m_PeriodLock);
	while (!m_Ready[m_Send] && !m_IsDead)
		if (pthread_cond_timedwait(&m_PeriodReady, &m_PeriodLock, &deadline)) break;
	const bool ready = m_Ready[m_Send];
	pthread_mutex_unlock(&m_PeriodLock);
	return ready;
}

bool OutputAudioClient::TransportCycle(bool read, bool write)
{
	if (!host || m_Frames != host->BUFSIZE || !m_Out[0] || !m_Client) return false;

	const int slot = m_Send;
	const int samples = host->BUFSIZE * m_Channels;
	if (!m_Ready[slot])
	{
		// The engine is late: keep the device fed with silence.
		++m_Underruns;
		if (write)
		{
			memset(m_In[slot], 0, samples * sizeof(float));
			m_Client->Write(m_In[slot], (unsigned int)host->BUFSIZE);
		}
		return true;
	}

	bool ok = true;
	if (write)
	{
		ok = m_Client->Write(m_Out[slot], (unsigned int)host->BUFSIZE);
		memset(m_Out[slot], 0, samples * sizeof(float));
	}
	if (read)
	{
		memset(m_In[slot], 0, samples * sizeof(float));
		ok = m_Client->Read(m_In[slot], (unsigned int)host->BUFSIZE) && ok;
	}

	// Hand the slot back only after the device has it.
	__sync_synchronize();
	m_Ready[slot] = false;
	m_Send = !slot;
	if (!pthread_mutex_trylock(&m_PeriodLock))
	{
		pthread_cond_signal(&m_PeriodFree);
		pthread_mutex_unlock(&m_PeriodLock);
	}
	return ok;
}

bool OutputAudioClient::AttachMode(unsigned int inChans, unsigned int outChans)
{
	if (!m_Client) return false;
	DeallocateBuffer();
	AllocateBuffer();
	if (!m_Out[0]) return false;
	m_IsDead = false;
	return m_Client->Attach(m_ClientName == "jack" && m_Destination == "default" ? "SSM-Output" : m_Destination,
		MakeOptions(inChans, outChans)) && m_Client->Start();
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
