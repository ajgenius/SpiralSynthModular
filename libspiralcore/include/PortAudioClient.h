// Copyright (C) 2003 David Griffiths <dave@pawfal.org>
//
// PortAudio v19 callback transport. The callback exchanges already rendered
// buffers and native presentation timestamps; it never runs the graph.

#ifndef PA_CLIENT
#define PA_CLIENT

#include <string>
#include <portaudio.h>
#include "AudioClient.h"

namespace spiralcore
{

class PortAudioClient : public AudioClient
{
public:
	static PortAudioClient *Get();
	static void PackUpAndGoHome();

	typedef AudioClientOptions DeviceOptions;

	bool Attach(const std::string &device, const AudioClientOptions &opt);
	void Detach();
	bool IsAttached() const { return __sync_fetch_and_add(&m_Attached, 0) != 0; }
	bool IsCallbackDriven() const { return true; }
	void SetCallback(void (*run)(void *, unsigned), void *context) { m_Run = run; m_Context = context; }
	bool Start();
	unsigned long GetBufferSize() const { return m_Opt.BufferSize; }
	unsigned long GetSampleRate() const { return m_Opt.Samplerate; }
	bool GetCycleTiming(AudioCycleTiming &timing) const;
	double GetInputLatency() const { return m_InputLatency; }
	double GetOutputLatency() const { return m_OutputLatency; }
	bool Write(const float *interleaved, unsigned int nframes);
	bool Read(float *interleaved, unsigned int nframes);

protected:
	PortAudioClient();
	~PortAudioClient();

private:
	PortAudioClient(const PortAudioClient &);
	PortAudioClient &operator=(const PortAudioClient &);

	bool Check(PaError err, const char *op) const;
	PaDeviceIndex FindDevice(bool input) const;
	bool FillParameters(PaStreamParameters &params, bool input) const;

	static PortAudioClient *m_Singleton;
	PaStream *m_Stream;
	mutable int m_Attached;
	bool m_Initialized;
	bool m_HasInput;
	bool m_HasOutput;
	int m_Channels;
	AudioClientOptions m_Opt;
	std::string m_Device;
	bool m_Started;
	const float *m_Input;
	float *m_Output;
	unsigned m_ProcessFrames;
	uint64_t m_Frame;
	double m_ClockOffset, m_InputLatency, m_OutputLatency;
	AudioCycleTiming m_Timing;
	void (*m_Run)(void *, unsigned);
	void *m_Context;
	static void Finished(void *context);
	static int Process(const void *, void *, unsigned long, const PaStreamCallbackTimeInfo *, PaStreamCallbackFlags, void *);
};

}

#endif
