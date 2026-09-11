// Copyright (C) 2003 David Griffiths <dave@pawfal.org>
// SSM blocking Pa_WriteStream adaptation (PortAudio 2.0 / v19).

#include <cctype>
#include <cstdlib>
#include <cstring>
#include <iostream>

#include "PortAudioClient.h"

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
	m_Device("default")
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
			if (info && (input ? info->maxInputChannels : info->maxOutputChannels) >= m_Channels)
				return (PaDeviceIndex)value;
		}
		return paNoDevice;
	}

	PaDeviceIndex partial = paNoDevice;
	for (PaDeviceIndex i = 0; i < count; ++i)
	{
		const PaDeviceInfo *info = Pa_GetDeviceInfo(i);
		if (!info || !info->name) continue;
		if ((input ? info->maxInputChannels : info->maxOutputChannels) < m_Channels)
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
	params.channelCount = m_Channels;
	params.sampleFormat = paFloat32;
	params.suggestedLatency = input ? info->defaultLowInputLatency
	                                : info->defaultLowOutputLatency;
	params.hostApiSpecificStreamInfo = NULL;
	return true;
}

bool PortAudioClient::Attach(const string &device, const AudioClientOptions &opt)
{
	Detach();
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

	/* NULL callback — Pa_WriteStream / Pa_ReadStream block, pacing the engine. */
	PaError err = Pa_OpenStream(&m_Stream, in, out,
	                            opt.Samplerate, opt.BufferSize,
	                            paClipOff | paDitherOff,
	                            NULL, NULL);
	if (!Check(err, "open")) { Detach(); return false; }
	if (!Check(Pa_StartStream(m_Stream), "start")) { Detach(); return false; }

	m_Attached = true;
	cerr << "PortAudio: attached (blocking) dest=" << m_Device
	     << " sr=" << opt.Samplerate
	     << " buf=" << opt.BufferSize
	     << " ch=" << m_Channels << endl;
	return true;
}

void PortAudioClient::Detach()
{
	if (m_Stream)
	{
		const PaError active = Pa_IsStreamActive(m_Stream);
		if (active == 1)
		{
			const PaError err = Pa_StopStream(m_Stream);
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
	m_Attached = m_HasInput = m_HasOutput = false;
}

bool PortAudioClient::Write(const float *interleaved, unsigned int nframes)
{
	if (!m_Attached || !m_HasOutput || !m_Stream || !interleaved) return false;
	PaError err = Pa_WriteStream(m_Stream, interleaved, nframes);
	if (err != paNoError && err != paOutputUnderflowed)
		return Check(err, "write");
	return true;
}

bool PortAudioClient::Read(float *interleaved, unsigned int nframes)
{
	if (!m_Attached || !m_HasInput || !m_Stream || !interleaved) return false;
	PaError err = Pa_ReadStream(m_Stream, interleaved, nframes);
	if (err != paNoError && err != paInputOverflowed)
		return Check(err, "read");
	return true;
}
