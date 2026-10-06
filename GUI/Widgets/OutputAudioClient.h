/*
 * Runtime audio backend facade for OutputPlugin.
 *
 * Device I/O and timestamped transport live in libspiralcore.  This facade keeps Sample mix / volume / HostInfo and
 * picks a compiled-in client from SpiralInfo / HostInfo.AUDIOCLIENT.
 */
#ifndef __OUTPUT_AUDIO_CLIENT_H__
#define __OUTPUT_AUDIO_CLIENT_H__

#include <string>
#include <pthread.h>
#include "SpiralPlugin.h"
#include "Sample.h"
using spiralcore::Sample;
#include "AudioStream.h"

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

	void BeginPeriod(const spiralcore::AudioStamp &capture);
	void CommitPeriod(const spiralcore::AudioStamp &playback);
	spiralcore::AudioStream *Stream() { return &m_Stream; }
	unsigned Underruns() const { return m_Stream.Errors(); }

	bool GetTransport(unsigned long &frame, bool &rolling) const
		{ return m_Client && m_Client->GetTransport(frame, rolling); }
	bool StartTransport() { return m_Client && m_Client->StartTransport(); }
	bool StopTransport() { return m_Client && m_Client->StopTransport(); }
	bool LocateTransport(unsigned long frame) { return m_Client && m_Client->LocateTransport(frame); }
	bool OpenReadWrite();
	bool OpenWrite();
	bool OpenRead();
	bool Close();
	void Kill();

	bool IsAttached() const { return m_Client && m_Client->IsAttached(); }

	bool IsCallbackDriven() const { return m_Client && m_Client->IsCallbackDriven(); }

	void SetCallback(void (*run)(void *, unsigned int), void *context)
		{ if (m_Client) m_Client->SetCallback(run, context); }

	unsigned long BufferSize() const { return m_Client ? m_Client->GetBufferSize() : 0; }

	unsigned long SampleRate() const { return m_Client ? m_Client->GetSampleRate() : 0; }

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
	bool m_IsDead;
	std::vector<float> m_Out, m_In;
	spiralcore::AudioStream m_Stream;

};

#define OUTPUTCLIENT OutputAudioClient

#endif
