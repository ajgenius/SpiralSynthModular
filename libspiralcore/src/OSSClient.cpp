// UA OSS /dev/dsp client, moved into libspiralcore (Grok Build).
// Blocking write()/read() of S16 LE; AudioClient I/O is interleaved float.

#define _ISOC9X_SOURCE 1
#define _ISOC99_SOURCE 1
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
#if defined(__BYTE_ORDER) && __BYTE_ORDER == BIG_ENDIAN
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
	m_Fd = open(path, flags);
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
		for (int i = 0; i < 32; i++)
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
	}
	else
	{
		val = 1;
		result = ioctl(m_Fd, SOUND_PCM_READ_CHANNELS, &val);
		CHECK_AND_REPORT_ERROR;
		val = AFMT_S16_LE;
		result = ioctl(m_Fd, SNDCTL_DSP_SETFMT, &val);
		CHECK_AND_REPORT_ERROR;
		val = (int)m_Samplerate;
		result = ioctl(m_Fd, SNDCTL_DSP_SPEED, &val);
		CHECK_AND_REPORT_ERROR;
	}
	return true;
}

bool OSSClient::Attach(const string &device, const AudioClientOptions &opt)
{
	Detach();
	if (device.empty() || device == "default")
		m_Device = "/dev/dsp";
	else
		m_Device = device;
	m_Samplerate = opt.Samplerate;
	m_NumBuffers = opt.NumBuffers;
	m_FragSize = opt.FragSize;
	m_Channels = opt.OutChannels ? (int)opt.OutChannels
	            : (opt.InChannels ? (int)opt.InChannels : 2);
	if (m_Channels < 1) m_Channels = 2;

	int flags = O_RDWR;
	if (opt.OutChannels && !opt.InChannels) flags = O_WRONLY;
	if (opt.InChannels && !opt.OutChannels) flags = O_RDONLY;

	if (!OpenDevice(flags)) return false;
	return true;
}

bool OSSClient::Write(const float *interleaved, unsigned int nframes)
{
	if (m_Fd < 0 || !interleaved) return false;
	const unsigned int nsamp = nframes * (unsigned int)m_Channels;
	if (!m_Conv || m_ConvSamples < nsamp)
	{
		FreeConv();
		m_Conv = new short[nsamp];
		m_ConvSamples = nsamp;
	}
	for (unsigned int i = 0; i < nsamp; ++i)
	{
		float t = interleaved[i];
		if (t > 1) t = 1;
		if (t < -1) t = -1;
		m_Conv[i] = (short)lrintf(t * (float)SHRT_MAX);
	}
	Byteswap(m_Conv, nsamp);
	const ssize_t bytes = (ssize_t)(nsamp * sizeof(short));
	if (write(m_Fd, m_Conv, bytes) != bytes) return false;
	return true;
}

bool OSSClient::Read(float *interleaved, unsigned int nframes)
{
	if (m_Fd < 0 || !interleaved) return false;
	const unsigned int nsamp = nframes * (unsigned int)m_Channels;
	if (!m_Conv || m_ConvSamples < nsamp)
	{
		FreeConv();
		m_Conv = new short[nsamp];
		m_ConvSamples = nsamp;
	}
	const ssize_t bytes = (ssize_t)(nsamp * sizeof(short));
	memset(m_Conv, 0, bytes);
	if (read(m_Fd, m_Conv, bytes) < 0) return false;
	Byteswap(m_Conv, nsamp);
	for (unsigned int i = 0; i < nsamp; ++i)
		interleaved[i] = m_Conv[i] / (float)SHRT_MAX;
	return true;
}
