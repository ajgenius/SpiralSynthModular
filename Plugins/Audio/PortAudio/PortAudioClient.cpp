// Copyright (C) 2003 David Griffiths <dave@pawfal.org>
// PortAudio v19 transport with native ADC/DAC presentation timestamps.

#include <cctype>
#include <cstdlib>
#include <cstring>
#include <iostream>

#include "PortAudioClient.h"
#include "AudioBackend.h"

using namespace std;
using namespace spiralcore;

PortAudioClient *PortAudioClient::m_Singleton = NULL;

PortAudioClient *PortAudioClient::Get()
{
	if (!m_Singleton) m_Singleton = new PortAudioClient;
	return m_Singleton;
}

void PortAudioClient::PackUpAndGoHome()
{
	if (m_Singleton)
	{
		delete m_Singleton;
		m_Singleton = NULL;
	}
}

PortAudioClient::PortAudioClient() :
	m_Stream(NULL),
	m_Attached(false),
	m_Initialized(false),
	m_HasInput(false),
	m_HasOutput(false),
	m_Channels(2),
	m_Device("default"),
	m_Started(false), m_Input(NULL), m_Output(NULL), m_ProcessFrames(0), m_Frame(0),
	m_ClockOffset(0), m_InputLatency(0), m_OutputLatency(0), m_Run(NULL), m_Context(NULL)
{
}

PortAudioClient::~PortAudioClient()
{
	Detach();
}

bool PortAudioClient::Check(PaError err, const char *op) const
{
	if (err == paNoError) return true;
	cerr << "PortAudio " << op << " failed: " << Pa_GetErrorText(err) << endl;
	return false;
}

PaDeviceIndex PortAudioClient::FindDevice(bool input) const
{
	if (m_Device.empty() || m_Device == "default")
		return input ? Pa_GetDefaultInputDevice() : Pa_GetDefaultOutputDevice();

	bool numeric = !m_Device.empty();
	for (size_t i = 0; i < m_Device.size(); ++i)
	{
		if (!isdigit((unsigned char)m_Device[i]))
		{
			numeric = false;
			break;
		}
	}

	const PaDeviceIndex count = Pa_GetDeviceCount();
	if (count < 0) return paNoDevice;

	if (numeric)
	{
		const long value = strtol(m_Device.c_str(), NULL, 10);
		if (value >= 0 && value < count)
		{
			const PaDeviceInfo *info = Pa_GetDeviceInfo((PaDeviceIndex)value);
			if (info && (input ? info->maxInputChannels : info->maxOutputChannels) >= int(input ? m_Opt.InChannels : m_Opt.OutChannels))
				return (PaDeviceIndex)value;
		}
		return paNoDevice;
	}

	PaDeviceIndex partial = paNoDevice;
	for (PaDeviceIndex i = 0; i < count; ++i)
	{
		const PaDeviceInfo *info = Pa_GetDeviceInfo(i);
		if (!info || !info->name) continue;
		if ((input ? info->maxInputChannels : info->maxOutputChannels) < int(input ? m_Opt.InChannels : m_Opt.OutChannels))
			continue;
		if (m_Device == info->name) return i;
		if (partial == paNoDevice && string(info->name).find(m_Device) != string::npos)
			partial = i;
	}
	return partial;
}

bool PortAudioClient::FillParameters(PaStreamParameters &params, bool input) const
{
	memset(&params, 0, sizeof(params));
	params.device = FindDevice(input);
	if (params.device == paNoDevice)
	{
		cerr << "PortAudio: no " << (input ? "input" : "output")
		     << " device matches destination '" << m_Device << "'" << endl;
		return false;
	}
	const PaDeviceInfo *info = Pa_GetDeviceInfo(params.device);
	if (!info) return false;
	params.channelCount = input ? m_Opt.InChannels : m_Opt.OutChannels;
	params.sampleFormat = paFloat32;
	params.suggestedLatency = input ? info->defaultLowInputLatency
	                                : info->defaultLowOutputLatency;
	params.hostApiSpecificStreamInfo = NULL;
	return true;
}

bool PortAudioClient::Attach(const string &device, const AudioClientOptions &opt)
{
	Detach();
	if ((!opt.InChannels && !opt.OutChannels) || !opt.BufferSize || !opt.Samplerate) return false;

	m_Opt = opt;
	m_Device = device.empty() ? "default" : device;
	m_Channels = opt.OutChannels ? (int)opt.OutChannels
	            : (opt.InChannels ? (int)opt.InChannels : 2);
	if (m_Channels < 1) m_Channels = 2;

	if (!m_Initialized)
	{
		if (!Check(Pa_Initialize(), "init")) return false;
		m_Initialized = true;
	}

	PaStreamParameters inP, outP;
	PaStreamParameters *in = NULL, *out = NULL;
	if (opt.InChannels)
	{
		if (!FillParameters(inP, true)) { Detach(); return false; }
		in = &inP;
		m_HasInput = true;
	}
	if (opt.OutChannels)
	{
		if (!FillParameters(outP, false)) { Detach(); return false; }
		out = &outP;
		m_HasOutput = true;
	}

	// Prepare first; the owner allocates its queues before Start enables callbacks.
	PaError err = Pa_OpenStream(&m_Stream, in, out,
	                            opt.Samplerate, opt.BufferSize,
	                            paNoFlag,
	                            Process, this);
	if (!Check(err, "open")) { Detach(); return false; }
	if (!Check(Pa_SetStreamFinishedCallback(m_Stream, Finished), "finished callback")) { Detach(); return false; }

	const PaStreamInfo *info = Pa_GetStreamInfo(m_Stream);
	if (!info || info->sampleRate <= 0) { Detach(); return false; }

	m_Opt.Samplerate = (unsigned)info->sampleRate;
	m_InputLatency = info->inputLatency;
	m_OutputLatency = info->outputLatency;
	// Choose the shortest clock-query interval, avoiding scheduler delays.
	double best = 1;
	for (unsigned n = 0; n < 8; ++n)
	{
		const double before = AudioMonotonicTime();
		const double native = Pa_GetStreamTime(m_Stream);
		const double after = AudioMonotonicTime();
		if (after - before < best)
		{
			best = after - before;
			m_ClockOffset = (before + after) * 0.5 - native;
		}
	}

	m_Frame = 0;
	__sync_lock_test_and_set(&m_Attached, 1);
	cerr << "PortAudio: attached (callback) dest=" << m_Device
	     << " sr=" << opt.Samplerate
	     << " buf=" << opt.BufferSize
	     << " ch=" << m_Channels << endl;
	return true;
}

bool PortAudioClient::Start()
{
	if (!IsAttached() || !m_Stream) return false;

	if (m_Started) return true;

	if (!Check(Pa_StartStream(m_Stream), "start")) { Detach(); return false; }

	m_Started = true;
	return true;
}

void PortAudioClient::Detach()
{
	if (m_Stream)
	{
		const PaError active = Pa_IsStreamActive(m_Stream);
		if (active == 1)
		{
			const PaError err = Pa_AbortStream(m_Stream);
			if (err != paNoError && err != paStreamIsStopped)
				Check(err, "stop");
		}
		Check(Pa_CloseStream(m_Stream), "close");
		m_Stream = NULL;
	}
	if (m_Initialized)
	{
		Check(Pa_Terminate(), "terminate");
		m_Initialized = false;
	}
	__sync_lock_test_and_set(&m_Attached, 0);
	m_Started = m_HasInput = m_HasOutput = false;
	m_Input = NULL;
	m_Output = NULL;
	m_ProcessFrames = 0;
	m_Timing.Valid = false;
}

int PortAudioClient::Process(const void *input, void *output, unsigned long frames,
	const PaStreamCallbackTimeInfo *time, PaStreamCallbackFlags, void *context)
{
	PortAudioClient &client = *static_cast<PortAudioClient *>(context);
	if (output) memset(output, 0, frames * client.m_Opt.OutChannels * sizeof(float));

	if (!client.IsAttached()) return paAbort;

	client.m_Input = static_cast<const float *>(input);
	client.m_Output = static_cast<float *>(output);
	client.m_ProcessFrames = frames;
	client.m_Timing.Frame = client.m_Frame;
	client.m_Timing.Frames = frames;
	client.m_Timing.SampleRate = client.m_Opt.Samplerate;
	client.m_Timing.CallbackTime = time->currentTime + client.m_ClockOffset;
	client.m_Timing.InputTime = time->inputBufferAdcTime + client.m_ClockOffset;
	client.m_Timing.OutputTime = time->outputBufferDacTime + client.m_ClockOffset;
	client.m_Timing.Valid = client.m_Opt.Samplerate &&
		(!client.m_HasOutput || time->outputBufferDacTime > 0) &&
		(!client.m_HasInput || time->inputBufferAdcTime > 0);
	if (client.m_Run) client.m_Run(client.m_Context, frames);

	client.m_Frame += frames;
	client.m_Input = NULL;
	client.m_Output = NULL;
	client.m_ProcessFrames = 0;
	return paContinue;
}

bool PortAudioClient::GetCycleTiming(AudioCycleTiming &timing) const
{
	if (!m_ProcessFrames || !m_Timing.Valid) return false;

	timing = m_Timing;
	return true;
}

bool PortAudioClient::Write(const float *interleaved, unsigned int frames)
{
	if (!m_Output || !interleaved || frames != m_ProcessFrames) return false;

	memcpy(m_Output, interleaved, size_t(frames) * m_Opt.OutChannels * sizeof(float));
	return true;
}

bool PortAudioClient::Read(float *interleaved, unsigned int frames)
{
	if (!m_Input || !interleaved || frames != m_ProcessFrames) return false;

	memcpy(interleaved, m_Input, size_t(frames) * m_Opt.InChannels * sizeof(float));
	return true;
}


void PortAudioClient::Finished(void *context)
{
	PortAudioClient *client = static_cast<PortAudioClient *>(context);
	__sync_lock_test_and_set(&client->m_Attached, 0);
}

// * Backend module entry

static void *CreatePortAudio(void *) { return static_cast<AudioClient *>(PortAudioClient::Get()); }
static void DestroyPortAudio(void *) { PortAudioClient::PackUpAndGoHome(); }
static const BackendDescriptor PortAudioBackend = { SPIRAL_AUDIO_PLUGIN_ABI, "audio", "portaudio", CreatePortAudio, DestroyPortAudio };

extern "C" const BackendDescriptor *SpiralPlugin_GetAudioBackend() { return &PortAudioBackend; }
