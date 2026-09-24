/*
 * Runtime audio backend facade for OutputPlugin.
 *
 * Device I/O lives in libspiralcore (blocking AudioClient: PortAudio,
 * ALSA, OSS).  This facade keeps Sample mix / volume / HostInfo and
 * picks a compiled-in client from SpiralInfo / HostInfo.AUDIOCLIENT.
 */
#ifndef __OUTPUT_AUDIO_CLIENT_H__
#define __OUTPUT_AUDIO_CLIENT_H__

#include <string>
#include "SpiralPlugin.h"
#include "Sample.h"
using spiralcore::Sample;
#include "AudioClient.h"

class OutputAudioClient
{
public:
	static OutputAudioClient *Get();
	static void PackUpAndGoHome();
	~OutputAudioClient();

	bool Configure(const std::string &client, const std::string &destination);
	const std::string &ClientName() const { return m_ClientName; }
	const std::string &Destination() const { return m_Destination; }

	void AllocateBuffer();
	void DeallocateBuffer();
	void SendStereo(const Sample *ldata, const Sample *rdata);
	void GetStereo(Sample *ldata, Sample *rdata);
	void SetVolume(float s) { m_Volume = s; }
	void SetNumChannels(int s) { m_Channels = s; }
	float GetVolume() const { return m_Volume; }
	bool Play();
	bool Read();
	bool OpenReadWrite();
	bool OpenWrite();
	bool OpenRead();
	bool Close();
	void Kill();

	static const HostInfo *host;

private:
	OutputAudioClient();
	bool Select(const std::string &client);
	bool SelectFirstAvailable();
	void DestroyBackend();
	bool AttachMode(unsigned int inChans, unsigned int outChans);
	spiralcore::AudioClientOptions MakeOptions(unsigned int inChans, unsigned int outChans) const;

	static OutputAudioClient *m_Singleton;
	spiralcore::AudioClient *m_Client;
	std::string m_ClientName;
	std::string m_Destination;
	float m_Volume;
	int m_Channels;
	int m_Frames;
	int m_WriteBuf;
	int m_ReadBuf;
	bool m_IsDead;
	float *m_Out[2];
	float *m_In[2];
};

#define OUTPUTCLIENT OutputAudioClient

#endif
