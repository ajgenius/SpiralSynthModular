// SSM blocking ALSA PCM client (Grok Build).  Device-paced snd_pcm_writei/readi.

#include <iostream>
#include "AlsaClient.h"

using namespace std;
using namespace spiralcore;

AlsaClient *AlsaClient::m_Singleton = NULL;

AlsaClient *AlsaClient::Get()
{
	if (!m_Singleton) m_Singleton = new AlsaClient;
	return m_Singleton;
}

void AlsaClient::PackUpAndGoHome()
{
	if (m_Singleton)
	{
		delete m_Singleton;
		m_Singleton = NULL;
	}
}

AlsaClient::AlsaClient() :
	m_Playback(NULL),
	m_Capture(NULL),
	m_Channels(2),
	m_Samplerate(44100),
	m_Device("default")
{
}

AlsaClient::~AlsaClient()
{
	Detach();
}

bool AlsaClient::OpenStream(snd_pcm_t **slot, snd_pcm_stream_t stream)
{
	if (*slot)
	{
		snd_pcm_close(*slot);
		*slot = NULL;
	}
	const char *dev = m_Device.c_str();
	int err = snd_pcm_open(slot, dev, stream, 0);
	if (err < 0)
	{
		cerr << "ALSA open '" << dev << "': " << snd_strerror(err) << endl;
		return false;
	}
	err = snd_pcm_set_params(*slot, SND_PCM_FORMAT_FLOAT_LE,
	                         SND_PCM_ACCESS_RW_INTERLEAVED,
	                         m_Channels, m_Samplerate, 1, 500000);
	if (err < 0)
	{
		cerr << "ALSA params: " << snd_strerror(err) << endl;
		snd_pcm_close(*slot);
		*slot = NULL;
		return false;
	}
	cerr << "ALSA: " << (stream == SND_PCM_STREAM_PLAYBACK ? "playback" : "capture")
	     << " on " << dev << " sr=" << m_Samplerate << " ch=" << m_Channels << endl;
	return true;
}

bool AlsaClient::Attach(const string &device, const AudioClientOptions &opt)
{
	Detach();
	if (device.empty() || device == "/dev/dsp")
		m_Device = "default";
	else
		m_Device = device;
	m_Samplerate = opt.Samplerate;
	m_Channels = opt.OutChannels ? (int)opt.OutChannels
	            : (opt.InChannels ? (int)opt.InChannels : 2);
	if (m_Channels < 1) m_Channels = 2;

	bool ok = true;
	if (opt.OutChannels)
		ok = OpenStream(&m_Playback, SND_PCM_STREAM_PLAYBACK) && ok;
	if (opt.InChannels)
		ok = OpenStream(&m_Capture, SND_PCM_STREAM_CAPTURE) && ok;
	if (!ok) Detach();
	return ok && IsAttached();
}

void AlsaClient::Detach()
{
	if (m_Playback) { snd_pcm_close(m_Playback); m_Playback = NULL; }
	if (m_Capture)  { snd_pcm_close(m_Capture);  m_Capture = NULL; }
}

bool AlsaClient::Write(const float *interleaved, unsigned int nframes)
{
	if (!m_Playback || !interleaved) return false;
	snd_pcm_sframes_t n = snd_pcm_writei(m_Playback, interleaved, nframes);
	if (n < 0) n = snd_pcm_recover(m_Playback, (int)n, 1);
	if (n < 0)
	{
		cerr << "ALSA write: " << snd_strerror((int)n) << endl;
		return false;
	}
	return true;
}

bool AlsaClient::Read(float *interleaved, unsigned int nframes)
{
	if (!m_Capture || !interleaved) return false;
	snd_pcm_sframes_t n = snd_pcm_readi(m_Capture, interleaved, nframes);
	if (n < 0) n = snd_pcm_recover(m_Capture, (int)n, 1);
	if (n < 0)
	{
		cerr << "ALSA read: " << snd_strerror((int)n) << endl;
		return false;
	}
	return true;
}
