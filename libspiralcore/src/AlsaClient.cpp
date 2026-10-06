// SSM blocking ALSA PCM client (Grok Build).  Device-paced snd_pcm_writei/readi.

#include "config.h"
#include <iostream>
#include <algorithm>
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
	m_Frames(512), m_Written(0), m_Read(0), m_Latency(0),
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
	int err = snd_pcm_open(slot, dev, stream, SND_PCM_NONBLOCK);
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
	if (err >= 0 && m_Playback && slot != &m_Playback && rate != m_Samplerate) err = -EINVAL;

	snd_pcm_uframes_t period = m_Frames, buffer = m_Frames * 3;
	if (err >= 0) err = snd_pcm_hw_params_set_period_size_near(*slot, params, &period, NULL);

	buffer = period * 3;
	if (err >= 0) err = snd_pcm_hw_params_set_buffer_size_near(*slot, params, &buffer);

	if (err >= 0) err = snd_pcm_hw_params(*slot, params);

	if (err >= 0)
	{
		m_Samplerate = rate;
		m_Frames = period;
		m_Latency = std::max(m_Latency, double(buffer) / rate);
		snd_pcm_sw_params_t *software;
		snd_pcm_sw_params_alloca(&software);
		err = snd_pcm_sw_params_current(*slot, software);
		if (err >= 0) err = snd_pcm_sw_params_set_avail_min(*slot, software, period);

		if (err >= 0) err = snd_pcm_sw_params_set_start_threshold(*slot, software, period);

		if (err >= 0) err = snd_pcm_sw_params_set_tstamp_mode(*slot, software, SND_PCM_TSTAMP_ENABLE);

#ifdef HAVE_ALSA_MONOTONIC_TIMESTAMP
		if (err >= 0) err = snd_pcm_sw_params_set_tstamp_type(*slot, software, SND_PCM_TSTAMP_TYPE_MONOTONIC);

#endif
		if (err >= 0) err = snd_pcm_sw_params(*slot, software);

		if (err >= 0) err = snd_pcm_prepare(*slot);

		if (err >= 0 && stream == SND_PCM_STREAM_CAPTURE) err = snd_pcm_start(*slot);

	}
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
	if ((!opt.InChannels && !opt.OutChannels) ||
		(opt.InChannels && opt.OutChannels && opt.InChannels != opt.OutChannels)) return false;


	if (device.empty() || device == "/dev/dsp")
		m_Device = "default";
	else
		m_Device = device;
	m_Samplerate = opt.Samplerate;
	m_Frames = opt.BufferSize;
	m_Written = m_Read = 0;
	m_Latency = 0;
	m_Timing.Valid = false;
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

bool AlsaClient::Timestamp(snd_pcm_t *stream, bool input, double &time)
{
	if (!stream) { time = AudioMonotonicTime(); return true; }

	snd_pcm_status_t *status;
	snd_pcm_status_alloca(&status);
	const double before = AudioMonotonicTime();
	if (snd_pcm_status(stream, status) < 0) return false;

	const double after = AudioMonotonicTime();
	snd_pcm_sframes_t delay = snd_pcm_status_get_delay(status);
	if (delay < 0) delay = 0;

#ifdef HAVE_ALSA_MONOTONIC_TIMESTAMP
	snd_htimestamp_t timestamp;
	snd_pcm_status_get_htstamp(status, &timestamp);
	double now = timestamp.tv_sec + timestamp.tv_nsec / 1e9;
	// A prepared stream has not acquired its first hardware timestamp yet.
	if (now <= 0 || after - now > 1)
	{
		now = (before + after) * 0.5;
		m_Timing.Estimated = true;
	}

#else
	// Old ALSA has no selectable monotonic timestamp. Bound the query with
	// the system clock instead of combining a wall timestamp with DAC time.
	const double now = (before + after) * 0.5;
	m_Timing.Estimated = true;
#endif
	time = now + (input ? -double(delay) : double(delay)) / m_Samplerate;
	return true;
}

int AlsaClient::WaitForCycle(unsigned milliseconds)
{
	snd_pcm_t *streams[] = {m_Playback, m_Capture};
	for (unsigned n = 0; n < 2; ++n)
	{
		snd_pcm_t *stream = streams[n];
		if (!stream) continue;

		snd_pcm_sframes_t available = snd_pcm_avail_update(stream);
		if (available == -EPIPE || available == -ESTRPIPE)
		{
			if (snd_pcm_prepare(stream) < 0 || (n && snd_pcm_start(stream) < 0)) return -1;

			return 0;
		}

		if (available < 0) return available == -EAGAIN || available == -EINTR ? 0 : -1;

		if (available < snd_pcm_sframes_t(m_Frames))
		{
			const int result = snd_pcm_wait(stream, std::min(milliseconds, 10U));
			return result < 0 && result != -EINTR ? -1 : 0;
		}

	}

	m_Timing.Frame = m_Playback ? m_Written : m_Read;
	m_Timing.Frames = m_Frames;
	m_Timing.SampleRate = m_Samplerate;
	m_Timing.CallbackTime = AudioMonotonicTime();
	m_Timing.Estimated = false;
	m_Timing.Valid = Timestamp(m_Capture, true, m_Timing.InputTime) &&
		Timestamp(m_Playback, false, m_Timing.OutputTime);
	return m_Timing.Valid ? 1 : -1;
}

bool AlsaClient::Write(const float *interleaved, unsigned int frames)
{
	if (!m_Playback || !interleaved) return false;

	// WaitForCycle reserved a complete period. Never wait or recover midway
	// through timestamped audio: a short write invalidates this stream epoch.
	const snd_pcm_sframes_t written = snd_pcm_writei(m_Playback, interleaved, frames);
	if (written > 0) m_Written += written;

	return written == snd_pcm_sframes_t(frames);
}

bool AlsaClient::Read(float *interleaved, unsigned int frames)
{
	if (!m_Capture || !interleaved) return false;

	const snd_pcm_sframes_t captured = snd_pcm_readi(m_Capture, interleaved, frames);
	if (captured > 0) m_Read += captured;

	return captured == snd_pcm_sframes_t(frames);
}
