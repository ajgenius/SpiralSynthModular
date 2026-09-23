// SSM blocking ALSA PCM client (Grok Build).  Device-paced snd_pcm_writei/readi.

#include <iostream>
#include <cerrno>
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
	snd_pcm_hw_params_t *params;
	snd_pcm_hw_params_alloca(&params);
	unsigned int rate=m_Samplerate;
	err=snd_pcm_hw_params_any(*slot,params);
	if (err>=0) err=snd_pcm_hw_params_set_access(*slot,params,SND_PCM_ACCESS_RW_INTERLEAVED);
	if (err>=0) err=snd_pcm_hw_params_set_format(*slot,params,SND_PCM_FORMAT_FLOAT);
	if (err>=0) err=snd_pcm_hw_params_set_channels(*slot,params,m_Channels);
	if (err>=0) err=snd_pcm_hw_params_set_rate_near(*slot,params,&rate,0);
	if (err>=0 && rate!=m_Samplerate) err=-EINVAL;
	unsigned int latency=500000;
	if (err>=0) err=snd_pcm_hw_params_set_buffer_time_near(*slot,params,&latency,0);
	if (err>=0) err=snd_pcm_hw_params(*slot,params);
	if (err>=0) err=snd_pcm_prepare(*slot);
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
	unsigned int done=0;
	unsigned int recoveries=0;
	while (done<nframes) {
		snd_pcm_sframes_t n = snd_pcm_writei(m_Playback, interleaved+done*m_Channels, nframes-done);
		if (n == -EINTR) continue;
		if (n < 0) {
			if (++recoveries>3 || ((n==-EPIPE || n==-ESTRPIPE) ? snd_pcm_prepare(m_Playback) : (int)n)<0) return false;
			continue;
		}
		if (n==0) return false;
		done+=(unsigned int)n;
		recoveries=0;
	}
	return true;
}

bool AlsaClient::Read(float *interleaved, unsigned int nframes)
{
	if (!m_Capture || !interleaved) return false;
	unsigned int done=0;
	unsigned int recoveries=0;
	while (done<nframes) {
		snd_pcm_sframes_t n = snd_pcm_readi(m_Capture, interleaved+done*m_Channels, nframes-done);
		if (n == -EINTR) continue;
		if (n < 0) {
			if (++recoveries>3 || ((n==-EPIPE || n==-ESTRPIPE) ? snd_pcm_prepare(m_Capture) : (int)n)<0) return false;
			continue;
		}
		if (n==0) return false;
		done+=(unsigned int)n;
		recoveries=0;
	}
	return true;
}
