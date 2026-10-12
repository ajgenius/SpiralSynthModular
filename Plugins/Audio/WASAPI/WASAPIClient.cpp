// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0601
#endif
#include <windows.h>
#include <mmdeviceapi.h>
#include <mmreg.h>
#include <audioclient.h>
#include <process.h>
#include <cstring>
#include <vector>
#include "WASAPIClient.h"

using namespace Spiral::Core;

struct WASAPIClient::Platform
{
	HANDLE Thread, Ready, Begin, Stop, Event;
	unsigned ThreadId;
	volatile LONG Attached, Error;
	bool Started;
	AudioClientOptions Options;
	std::wstring Device;
	UINT32 Capacity, ProcessFrames;
	double Latency;
	uint64_t Frame;
	float *Output;
	AudioCycleTiming Timing;
	void (*Run)(void *, unsigned);
	void *Context;

	// Used and released exclusively on Thread, including every failure path.
	IMMDeviceEnumerator *Enumerator;
	IMMDevice *Endpoint;
	IAudioClient *Client;
	IAudioRenderClient *Render;

	Platform() : Thread(0), Ready(0), Begin(0), Stop(0), Event(0), ThreadId(0),
		Attached(0), Error(S_OK), Started(false), Capacity(0), ProcessFrames(0),
		Latency(0), Frame(0), Output(0), Run(0), Context(0),
		Enumerator(0), Endpoint(0), Client(0), Render(0) {}

	bool Check(HRESULT result)
	{
		if (SUCCEEDED(result)) return true;

		InterlockedExchange(&Error, result);
		return false;
	}

	bool Prepare()
	{
		if (!Check(CoCreateInstance(__uuidof(MMDeviceEnumerator), 0, CLSCTX_ALL,
			__uuidof(IMMDeviceEnumerator), reinterpret_cast<void **>(&Enumerator)))) return false;

		HRESULT result = Device.empty()
			? Enumerator->GetDefaultAudioEndpoint(eRender, eConsole, &Endpoint)
			: Enumerator->GetDevice(Device.c_str(), &Endpoint);
		if (!Check(result)) return false;

		if (!Check(Endpoint->Activate(__uuidof(IAudioClient), CLSCTX_ALL, 0,
			reinterpret_cast<void **>(&Client)))) return false;

		// The Windows shared engine converts this explicit float format to the
		// endpoint's mix format. The host continues to see the requested rate.
		WAVEFORMATEX format;
		std::memset(&format, 0, sizeof(format));
		format.wFormatTag = WAVE_FORMAT_IEEE_FLOAT;
		format.nChannels = Options.OutChannels;
		format.nSamplesPerSec = Options.Samplerate;
		format.wBitsPerSample = 32;
		format.nBlockAlign = format.nChannels * sizeof(float);
		format.nAvgBytesPerSec = format.nSamplesPerSec * format.nBlockAlign;
		const DWORD flags = AUDCLNT_STREAMFLAGS_EVENTCALLBACK
			| AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM | AUDCLNT_STREAMFLAGS_SRC_DEFAULT_QUALITY;
		const REFERENCE_TIME duration = REFERENCE_TIME(1e7 * Options.BufferSize / Options.Samplerate);
		if (!Check(Client->Initialize(AUDCLNT_SHAREMODE_SHARED, flags, duration, 0, &format, 0)))
			return false;

		if (!Check(Client->SetEventHandle(Event)) || !Check(Client->GetBufferSize(&Capacity)))
			return false;

		if (!Capacity) return Check(E_FAIL);

		REFERENCE_TIME latency = 0;
		if (!Check(Client->GetStreamLatency(&latency))) return false;

		Latency = latency / 1e7;
		return Check(Client->GetService(__uuidof(IAudioRenderClient), reinterpret_cast<void **>(&Render)));
	}

	bool Process()
	{
		UINT32 padding = 0;
		if (!Check(Client->GetCurrentPadding(&padding))) return false;

		if (padding > Capacity) return Check(E_FAIL);

		const UINT32 frames = Capacity - padding;
		if (!frames) return true;

		BYTE *buffer = 0;
		if (!Check(Render->GetBuffer(frames, &buffer))) return false;

		Output = reinterpret_cast<float *>(buffer);
		ProcessFrames = frames;
		std::memset(Output, 0, size_t(frames) * Options.OutChannels * sizeof(float));
		Timing.Frame = Frame;
		Timing.Frames = frames;
		Timing.SampleRate = Options.Samplerate;
		Timing.Step = 1.0 / Options.Samplerate;
		Timing.CallbackTime = AudioMonotonicTime();
		// Queue depth plus the reported stream latency is an estimate, not a
		// hardware presentation timestamp. Keep that distinction in the contract.
		Timing.OutputTime = Timing.CallbackTime + padding / double(Options.Samplerate) + Latency;
		Timing.Estimated = Timing.Valid = true;

		bool ok = true;
		try
		{
			if (Run) Run(Context, frames);
		}
		catch (...)
		{
			ok = Check(E_FAIL);
		}

		Output = 0;
		ProcessFrames = 0;
		Timing.Valid = false;
		const HRESULT released = Render->ReleaseBuffer(frames, ok ? 0 : AUDCLNT_BUFFERFLAGS_SILENT);
		Frame += frames;
		return Check(released) && ok;
	}

	static unsigned __stdcall Work(void *context)
	{
		Platform &p = *static_cast<Platform *>(context);
		// All COM calls stay in this apartment, independent of the host's GUI
		// apartment. STA also meets Windows 8's first-IAudioClient requirement.
		const HRESULT initialized = CoInitializeEx(0, COINIT_APARTMENTTHREADED);
		bool running = false;
		if (p.Check(initialized) && p.Prepare())
		{
			InterlockedExchange(&p.Attached, 1);
			SetEvent(p.Ready);
			HANDLE commands[] = {p.Stop, p.Begin};
			if (WaitForMultipleObjects(2, commands, FALSE, INFINITE) == WAIT_OBJECT_0 + 1)
			{
				// Prime after the host requests Start, never during Attach.
				running = p.Process() && p.Check(p.Client->Start());
				if (!running) InterlockedExchange(&p.Attached, 0);

				SetEvent(p.Ready);
				HANDLE events[] = {p.Stop, p.Event};
				while (running)
				{
					const DWORD ready = WaitForMultipleObjects(2, events, FALSE, 2000);
					if (ready == WAIT_OBJECT_0) break;

					if (ready != WAIT_OBJECT_0 + 1)
					{
						p.Check(HRESULT_FROM_WIN32(ready == WAIT_TIMEOUT ? ERROR_TIMEOUT : GetLastError()));
						break;
					}

					if (!p.Process()) break;
				}
			}
		}

		InterlockedExchange(&p.Attached, 0);
		if (running) p.Client->Stop();

		if (p.Render) p.Render->Release();

		if (p.Client) p.Client->Release();

		if (p.Endpoint) p.Endpoint->Release();

		if (p.Enumerator) p.Enumerator->Release();

		p.Render = 0;
		p.Client = 0;
		p.Endpoint = 0;
		p.Enumerator = 0;
		if (SUCCEEDED(initialized)) CoUninitialize();

		SetEvent(p.Ready);
		return 0;
	}
};

WASAPIClient::WASAPIClient() : m_Platform(new Platform) {}
WASAPIClient::~WASAPIClient() { Detach(); delete m_Platform; }

bool WASAPIClient::Attach(const std::string &device, const AudioClientOptions &options)
{
	Detach();
	Platform &p = *m_Platform;
	InterlockedExchange(&p.Error, S_OK);
	if (options.InChannels || !options.OutChannels || options.OutChannels > 2
		|| !options.Samplerate || options.Samplerate > 384000 || !options.BufferSize
		|| options.BufferSize > options.Samplerate * 2)
		return p.Check(E_INVALIDARG);

	p.Options = options;
	p.Device.clear();
	if (!device.empty() && device != "default")
	{
		const int size = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, device.c_str(), -1, 0, 0);
		if (!size) return p.Check(E_INVALIDARG);

		std::vector<wchar_t> wide(size);
		if (!MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, device.c_str(), -1, &wide[0], size))
			return p.Check(E_INVALIDARG);

		p.Device = &wide[0];
	}

	p.Ready = CreateEventW(0, FALSE, FALSE, 0);
	p.Begin = CreateEventW(0, FALSE, FALSE, 0);
	p.Stop = CreateEventW(0, TRUE, FALSE, 0);
	p.Event = CreateEventW(0, FALSE, FALSE, 0);
	if (p.Ready && p.Begin && p.Stop && p.Event)
		p.Thread = reinterpret_cast<HANDLE>(_beginthreadex(0, 0, Platform::Work, &p, 0, &p.ThreadId));

	if (!p.Thread)
	{
		p.Check(E_OUTOFMEMORY);
		Detach();
		return false;
	}

	WaitForSingleObject(p.Ready, INFINITE);
	if (IsAttached()) return true;

	Detach();
	return false;
}

void WASAPIClient::Detach()
{
	Platform &p = *m_Platform;
	if (p.Stop) SetEvent(p.Stop);

	if (p.Thread) WaitForSingleObject(p.Thread, INFINITE);

	if (p.Thread) CloseHandle(p.Thread);

	if (p.Ready) CloseHandle(p.Ready);

	if (p.Begin) CloseHandle(p.Begin);

	if (p.Stop) CloseHandle(p.Stop);

	if (p.Event) CloseHandle(p.Event);

	p.Thread = p.Ready = p.Begin = p.Stop = p.Event = 0;
	p.ThreadId = 0;
	p.Started = false;
	p.Capacity = p.ProcessFrames = 0;
	p.Latency = 0;
	p.Frame = 0;
	p.Output = 0;
	p.Timing = AudioCycleTiming();
}

bool WASAPIClient::IsAttached() const { return InterlockedCompareExchange(&m_Platform->Attached, 0, 0) != 0; }
long WASAPIClient::LastError() const { return InterlockedCompareExchange(&m_Platform->Error, 0, 0); }
unsigned long WASAPIClient::GetBufferSize() const { return m_Platform->Capacity; }
unsigned long WASAPIClient::GetSampleRate() const { return IsAttached() ? m_Platform->Options.Samplerate : 0; }
double WASAPIClient::GetOutputLatency() const { return m_Platform->Latency; }

void WASAPIClient::SetCallback(void (*run)(void *, unsigned), void *context)
{
	Platform &p = *m_Platform;
	if (p.Started) return;

	p.Run = run;
	p.Context = context;
}

bool WASAPIClient::Start()
{
	Platform &p = *m_Platform;
	if (!IsAttached()) return false;

	if (!p.Started)
	{
		p.Started = true;
		SetEvent(p.Begin);
		WaitForSingleObject(p.Ready, INFINITE);
	}

	return IsAttached();
}

bool WASAPIClient::Write(const float *interleaved, unsigned frames)
{
	Platform &p = *m_Platform;
	if (GetCurrentThreadId() != p.ThreadId || !interleaved || !p.Output || frames != p.ProcessFrames)
		return false;

	std::memcpy(p.Output, interleaved, size_t(frames) * p.Options.OutChannels * sizeof(float));
	return true;
}

bool WASAPIClient::GetCycleTiming(AudioCycleTiming &timing) const
{
	Platform &p = *m_Platform;
	if (GetCurrentThreadId() != p.ThreadId || !p.Timing.Valid) return false;

	timing = p.Timing;
	return true;
}
