// UA OSS /dev/dsp client, moved into libspiralcore (Grok Build).
// Blocking write()/read() of S16 LE; AudioClient I/O is interleaved float.

#define _ISOC9X_SOURCE 1
#define _ISOC99_SOURCE 1
#include "config.h"
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#if defined (__FreeBSD__)
#include <machine/soundcard.h>
#else
#if defined (__NetBSD__) || defined (__OpenBSD__)
#include <soundcard.h>
#undef ioctl
#else
#include <sys/soundcard.h>
#endif
#endif
#ifdef __linux__
#include <endian.h>
#endif
#include <iostream>
#include <algorithm>
#include <poll.h>

#include "OSSClient.h"

using namespace std;
using namespace spiralcore;

OSSClient *OSSClient::m_Singleton = NULL;

#define CHECK_AND_REPORT_ERROR if (result<0) \
{ \
	perror("Sound device did not accept settings"); \
	Detach(); \
	return false; \
}

OSSClient *OSSClient::Get()
{
	if (!m_Singleton) m_Singleton = new OSSClient;
	return m_Singleton;
}

void OSSClient::PackUpAndGoHome()
{
	if (m_Singleton)
	{
		delete m_Singleton;
		m_Singleton = NULL;
	}
}

OSSClient::OSSClient() :
	m_Fd(-1),
	m_Channels(2),
	m_Frames(512), m_Input(false), m_Output(false), m_Frame(0), m_Latency(0),
	m_Samplerate(44100),
	m_NumBuffers(8),
	m_FragSize(256),
	m_Conv(NULL),
	m_ConvSamples(0),
	m_Device("/dev/dsp")
{
}

OSSClient::~OSSClient()
{
	Detach();
}

void OSSClient::FreeConv()
{
	delete [] m_Conv;
	m_Conv = NULL;
	m_ConvSamples = 0;
}

void OSSClient::Byteswap(short *buf, unsigned int nsamp) const
{
#ifdef WORDS_BIGENDIAN
	for (unsigned int n = 0; n < nsamp; ++n)
		buf[n] = (short)(((buf[n] << 8) & 0xff00) | ((buf[n] >> 8) & 0xff));
#else
	(void)buf;
	(void)nsamp;
#endif
}

void OSSClient::Detach()
{
	if (m_Fd >= 0)
	{
		cerr << "Closing dsp output" << endl;
		close(m_Fd);
		m_Fd = -1;
	}
	FreeConv();
}

bool OSSClient::OpenDevice(int flags)
{
	int result, val;
	const char *path = m_Device.c_str();
	cerr << "Opening dsp " << path << endl;
	m_Fd = open(path, flags | O_NONBLOCK);
	if (m_Fd < 0)
	{
		fprintf(stderr, "Can't open audio driver.\n");
		return false;
	}
	result = ioctl(m_Fd, SNDCTL_DSP_RESET, NULL);
	CHECK_AND_REPORT_ERROR;

	if (flags != O_RDONLY)
	{
		short fgmtsize = 0;
		int numfgmts = m_NumBuffers;
		if (numfgmts == -1) numfgmts = 0x7fff;
		else if (numfgmts <= 0) numfgmts = 8;
		int fragsize = (int)m_FragSize;
		if (fragsize <= 0) fragsize = 256;
		for (int i = 0; i < 31; i++)
			if (fragsize == (1 << i)) { fgmtsize = i; break; }
		if (fgmtsize == 0)
		{
			cerr << "Fragment size [" << fragsize << "] must be power of two!" << endl;
			fgmtsize = 8;
		}
		val = (numfgmts << 16) | (int)fgmtsize;
		result = ioctl(m_Fd, SNDCTL_DSP_SETFRAGMENT, &val);
		CHECK_AND_REPORT_ERROR;
		val = 1;
		result = ioctl(m_Fd, SOUND_PCM_WRITE_CHANNELS, &val);
		CHECK_AND_REPORT_ERROR;
		val = AFMT_S16_LE;
		result = ioctl(m_Fd, SNDCTL_DSP_SETFMT, &val);
		CHECK_AND_REPORT_ERROR;
		val = (m_Channels == 2) ? 1 : 0;
		result = ioctl(m_Fd, SNDCTL_DSP_STEREO, &val);
		CHECK_AND_REPORT_ERROR;
		val = (int)m_Samplerate;
		result = ioctl(m_Fd, SNDCTL_DSP_SPEED, &val);
		CHECK_AND_REPORT_ERROR;
		m_Samplerate = val;
	}
	else
	{
		val = 1;
		result = ioctl(m_Fd, SOUND_PCM_READ_CHANNELS, &val);
		CHECK_AND_REPORT_ERROR;
		val = AFMT_S16_LE;
		result = ioctl(m_Fd, SNDCTL_DSP_SETFMT, &val);
		CHECK_AND_REPORT_ERROR;
		val = (m_Channels == 2) ? 1 : 0;
		result = ioctl(m_Fd, SNDCTL_DSP_STEREO, &val);
		CHECK_AND_REPORT_ERROR;
		val = (int)m_Samplerate;
		result = ioctl(m_Fd, SNDCTL_DSP_SPEED, &val);
		CHECK_AND_REPORT_ERROR;
		m_Samplerate = val;
	}
	return true;
}

bool OSSClient::Attach(const string &device, const AudioClientOptions &opt)
{
	Detach();
	if ((!opt.InChannels && !opt.OutChannels) ||
		(opt.InChannels && opt.OutChannels && opt.InChannels != opt.OutChannels)) return false;

	if (opt.InChannels > 2 || opt.OutChannels > 2) return false;


	if (device.empty() || device == "default")
		m_Device = "/dev/dsp";
	else
		m_Device = device;
	m_Samplerate = opt.Samplerate;
	m_Frames = opt.BufferSize;
	m_Input = opt.InChannels != 0;
	m_Output = opt.OutChannels != 0;
	m_Frame = 0;
	m_Timing.Valid = false;
	m_NumBuffers = opt.NumBuffers;
	m_FragSize = opt.FragSize;
	m_Channels = opt.OutChannels ? (int)opt.OutChannels
	            : (opt.InChannels ? (int)opt.InChannels : 2);
	if (m_Channels < 1) m_Channels = 2;

	int flags = O_RDWR;
	if (opt.OutChannels && !opt.InChannels) flags = O_WRONLY;
	if (opt.InChannels && !opt.OutChannels) flags = O_RDONLY;

	if (!OpenDevice(flags)) return false;

	audio_buf_info space;
	if (ioctl(m_Fd, m_Output ? SNDCTL_DSP_GETOSPACE : SNDCTL_DSP_GETISPACE, &space) < 0 ||
		space.fragsize <= 0 || !m_Samplerate) { Detach(); return false; }

	m_Frames = std::max(1U, unsigned(space.fragsize / (m_Channels * sizeof(short))));
	m_Latency = double(space.fragstotal * space.fragsize) / (m_Channels * sizeof(short) * m_Samplerate);
	if (m_Output)
	{
#ifdef SNDCTL_DSP_GETODELAY
		int delay = 0;
		if (ioctl(m_Fd, SNDCTL_DSP_GETODELAY, &delay) < 0) { Detach(); return false; }

#else
		std::cerr << "OSS: driver cannot report playback delay for presentation alignment" << std::endl;
		Detach();
		return false;
#endif
	}

	m_ConvSamples = m_Frames * m_Channels;
	m_Conv = new short[m_ConvSamples];
	return true;
}

int OSSClient::WaitForCycle(unsigned milliseconds)
{
	const unsigned bytes = m_Frames * m_Channels * sizeof(short);
	audio_buf_info input, output;
	memset(&input, 0, sizeof(input));
	memset(&output, 0, sizeof(output));
	if ((m_Input && ioctl(m_Fd, SNDCTL_DSP_GETISPACE, &input) < 0) ||
		(m_Output && ioctl(m_Fd, SNDCTL_DSP_GETOSPACE, &output) < 0)) return -1;

	if ((m_Input && unsigned(input.bytes) < bytes) || (m_Output && unsigned(output.bytes) < bytes))
	{
		struct pollfd descriptor;
		descriptor.fd = m_Fd;
		descriptor.events = (m_Input ? POLLIN : 0) | (m_Output ? POLLOUT : 0);
		descriptor.revents = 0;
		const int result = poll(&descriptor, 1, std::min(milliseconds, 10U));
		return result < 0 && errno != EINTR ? -1 : 0;
	}

	int delay = 0;
#ifdef SNDCTL_DSP_GETODELAY
	if (m_Output && ioctl(m_Fd, SNDCTL_DSP_GETODELAY, &delay) < 0) return -1;

#endif
	const double now = AudioMonotonicTime();
	const double bytesPerSecond = double(m_Channels * sizeof(short)) * m_Samplerate;
	m_Timing.Frame = m_Frame;
	m_Frame += m_Frames;
	m_Timing.Frames = m_Frames;
	m_Timing.SampleRate = m_Samplerate;
	m_Timing.CallbackTime = now;
	// OSS exposes queue depth rather than hardware timestamps. This is a
	// query-time estimate; its accuracy is limited by the device's reporting.
	m_Timing.OutputTime = now + std::max(0, delay) / bytesPerSecond;
	m_Timing.InputTime = now - input.bytes / bytesPerSecond;
	m_Timing.Valid = true;
	m_Timing.Estimated = true;
	return 1;
}

bool OSSClient::Write(const float *interleaved, unsigned int frames)
{
	const unsigned samples = frames * m_Channels;
	if (m_Fd < 0 || !interleaved || samples > m_ConvSamples) return false;

	for (unsigned n = 0; n < samples; ++n)
	{
		const float value = std::max(-1.f, std::min(1.f, interleaved[n]));
		m_Conv[n] = (short)lrintf(value * SHRT_MAX);
	}

	Byteswap(m_Conv, samples);
	const ssize_t bytes = samples * sizeof(short);
	return write(m_Fd, m_Conv, bytes) == bytes;
}

bool OSSClient::Read(float *interleaved, unsigned int frames)
{
	const unsigned samples = frames * m_Channels;
	if (m_Fd < 0 || !interleaved || samples > m_ConvSamples) return false;

	const ssize_t bytes = samples * sizeof(short);
	if (read(m_Fd, m_Conv, bytes) != bytes) return false;

	Byteswap(m_Conv, samples);
	for (unsigned n = 0; n < samples; ++n) interleaved[n] = m_Conv[n] / float(SHRT_MAX);

	return true;
}
