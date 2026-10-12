// SPDX-License-Identifier: GPL-2.0-or-later
#include <windows.h>
#include <audioclient.h>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#include "WASAPIClient.h"

using namespace Spiral::Core;

struct Signal
{
	WASAPIClient *Client;
	std::vector<float> Samples;
	unsigned Rate, Channels;
	uint64_t Frames;
	volatile LONG Calls, Failed;
	Signal(WASAPIClient *client, unsigned rate, unsigned channels) : Client(client),
		Samples(client->GetBufferSize() * channels), Rate(rate), Channels(channels),
		Frames(0), Calls(0), Failed(0) {}

	static void Process(void *context, unsigned frames)
	{
		Signal &s = *static_cast<Signal *>(context);
		AudioCycleTiming timing;
		if (frames * s.Channels > s.Samples.size() || !s.Client->GetCycleTiming(timing)
			|| !timing.Valid || !timing.Estimated || timing.Frame != s.Frames
			|| timing.Frames != frames || timing.OutputTime < timing.CallbackTime)
		{
			InterlockedExchange(&s.Failed, 1);
			return;
		}

		for (unsigned f = 0; f < frames; ++f)
		{
			for (unsigned c = 0; c < s.Channels; ++c)
			{
				const double hz = c ? 660 : 440;
				s.Samples[f * s.Channels + c] = s.Frames + f < s.Rate / 2
					? float(0.25 * std::sin(6.283185307179586 * hz * (s.Frames + f) / s.Rate)) : 0;
			}
		}

		if (s.Client->Write(&s.Samples[0], frames + 1) || !s.Client->Write(&s.Samples[0], frames)
			|| s.Client->Read(&s.Samples[0], frames))
			InterlockedExchange(&s.Failed, 1);

		s.Frames += frames;
		InterlockedIncrement(&s.Calls);
	}
};

int main(int argc, char **argv)
{
	const bool unavailable = argc > 1 && std::strcmp(argv[1], "--unavailable") == 0;
	const bool required = argc > 1 && std::strcmp(argv[1], "--output") == 0;
	WASAPIClient client;
	AudioClientOptions options;
	options.Samplerate = argc > 2 ? std::atoi(argv[2]) : 48000;
	options.OutChannels = argc > 3 ? std::atoi(argv[3]) : 2;
	options.InChannels = 1;
	if (client.Attach("default", options) || client.Start()) return 1;

	options.InChannels = 0;
	if (client.Attach("not-a-windows-endpoint", options) || client.IsAttached()) return 1;

	// Warm the COM runtime before measuring persistent process handles.
	if (client.Attach("default", options)) client.Detach();

	DWORD before = 0;
	const bool measureHandles = GetProcessHandleCount(GetCurrentProcess(), &before) != FALSE && before != 0;
	for (unsigned pass = 0; pass < 3; ++pass)
	{
		if (!client.Attach("default", options))
		{
			const HRESULT error = client.LastError();
			std::printf("no output: HRESULT 0x%08lx\n", static_cast<unsigned long>(error));
			const bool absent = error == HRESULT_FROM_WIN32(ERROR_NOT_FOUND)
				|| error == HRESULT_FROM_WIN32(RPC_S_SERVER_UNAVAILABLE)
				|| error == AUDCLNT_E_SERVICE_NOT_RUNNING;
			if (!absent || required || client.IsAttached() || client.Start()) return 1;

			if (!unavailable) return 77;

			continue;
		}

		if (unavailable || client.GetSampleRate() != options.Samplerate || !client.GetBufferSize()) return 1;

		Signal signal(&client, options.Samplerate, options.OutChannels);
		client.SetCallback(Signal::Process, &signal);
		Sleep(30);
		if (signal.Calls || client.Write(&signal.Samples[0], client.GetBufferSize())) return 1;

		if (!client.Start() || !client.Start()) return 1;

		Sleep(750);
		if (!client.IsAttached()) return 1;

		client.Detach();
		const LONG calls = signal.Calls;
		Sleep(20);
		if (signal.Calls != calls || signal.Failed || !calls || signal.Frames < options.Samplerate / 2
			|| client.IsAttached() || client.Start()) return 1;

		std::printf("PASS output: %lu Hz, %u channels, %ld callbacks\n",
			static_cast<unsigned long>(options.Samplerate), options.OutChannels, calls);
		client.SetCallback(0, 0);
		client.Detach();
	}

	DWORD after = 0;
	if (measureHandles)
	{
		if (!GetProcessHandleCount(GetCurrentProcess(), &after) || after > before + 2) return 1;

		std::printf("handles before %lu after %lu\n", static_cast<unsigned long>(before), static_cast<unsigned long>(after));
	}
	else
	{
		std::puts("process handle counts unavailable on this runtime");
	}

	std::puts(unavailable ? "PASS native unavailable/retry/cleanup" : "PASS WASAPI lifecycle");
	return 0;
}
