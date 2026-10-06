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
#include <pthread.h>
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

	// Two period slots between the engine and the transport. The engine
	// mixes into one while the transport plays the other: WaitPeriod
	// blocks until the mix slot is free and reports whether it is; the
	// engine must not touch the slot otherwise. CommitPeriod hands it
	// over. TransportCycle plays the ready slot and captures into it,
	// from the device callback or the blocking thread; without a ready
	// slot it plays silence and reports an underrun. GetStereo reads the
	// capture of the engine's own slot. Neither side ever waits on the
	// other inside a device call.
	bool WaitPeriod(unsigned microseconds);
	void CommitPeriod();
	bool WaitReady(unsigned microseconds);
	bool TransportCycle(bool read, bool write);
	unsigned Underruns() const { return m_Underruns; }
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
	int m_Mix;
	int m_Send;
	volatile bool m_Ready[2];
	volatile unsigned m_Underruns;
	pthread_mutex_t m_PeriodLock;
	pthread_cond_t m_PeriodFree;
	pthread_cond_t m_PeriodReady;
	bool m_IsDead;
	float *m_Out[2];
	float *m_In[2];
};

#define OUTPUTCLIENT OutputAudioClient

#endif
