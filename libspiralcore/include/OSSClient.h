// Copyright (C) 2001 David Griffiths <dave@pawfal.org>
// Extracted from the UA OutputPlugin OSS client; AudioClient shape
// matches the adapted PortAudioClient (blocking device write).

#ifndef SPIRALCORE_OSS_CLIENT
#define SPIRALCORE_OSS_CLIENT

#include <string>
#include "AudioClient.h"

namespace spiralcore
{

class OSSClient : public AudioClient
{
public:
	static OSSClient *Get();
	static void PackUpAndGoHome();

	bool Attach(const std::string &device, const AudioClientOptions &opt);
	void Detach();
	bool IsAttached() const { return m_Fd >= 0; }
	bool Write(const float *interleaved, unsigned int nframes);
	bool Read(float *interleaved, unsigned int nframes);

protected:
	OSSClient();
	~OSSClient();

private:
	OSSClient(const OSSClient &);
	OSSClient &operator=(const OSSClient &);

	bool OpenDevice(int flags);
	void FreeConv();
	void Byteswap(short *buf, unsigned int nsamp) const;

	static OSSClient *m_Singleton;
	int m_Fd;
	int m_Channels;
	unsigned int m_Samplerate;
	int m_NumBuffers;
	unsigned int m_FragSize;
	short *m_Conv;
	unsigned int m_ConvSamples;
	std::string m_Device;
};

}

#endif
