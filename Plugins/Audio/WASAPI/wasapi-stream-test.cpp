// SPDX-License-Identifier: GPL-2.0-or-later
#include <windows.h>
#include <cmath>
#include <cstdio>
#include "WASAPIClient.h"
#include "AudioStream.h"
#include "PresentationClock.h"
using namespace Spiral::Core;

int main()
{
	WASAPIClient client;
	AudioClientOptions options;
	options.Samplerate = 48000;
	if (!client.Attach("default", options)) return 77;

	AudioStream stream;
	if (!stream.Configure(&client, 0, 2, 128, options.Samplerate)) return 1;

	client.SetCallback(AudioStream::Callback, &stream);
	if (!stream.Start()) { client.Detach(); return 1; }

	PresentationClock clock;
	AudioCycleTiming timing;
	AudioStamp stamp;
	float samples[256];
	unsigned blocks = 0;
	bool failed = false;
	bool warmed = false;
	unsigned warmErrors = 0;
	const double start = AudioMonotonicTime();
	while (AudioMonotonicTime() - start < 1.2)
	{
		if (!warmed && AudioMonotonicTime() - start > 0.4)
		{
			warmErrors = stream.Errors();
			warmed = true;
		}

		stream.Timing(timing);
		if (clock.Prepare(AudioMonotonicTime(), 128, options.Samplerate,
			timing.Valid ? &timing : 0, 0.1, stamp))
		{
			stream.SetGeneration(stamp.Generation);
			for (unsigned n = 0; n < 128; ++n)
			{
				samples[2*n] = float(0.25 * std::sin(6.283185307179586 * 440 * (stamp.Frame + n) / options.Samplerate));
				samples[2*n+1] = float(0.25 * std::sin(6.283185307179586 * 660 * (stamp.Frame + n) / options.Samplerate));
			}

			if (!stream.Playback(samples, 128, stamp)) failed = true;

			clock.Commit(128);
			++blocks;
		}
		else
		{
			Sleep(1);
		}
	}

	client.Detach();
	std::printf("engine blocks %u, transport errors %u (warmup %u)\n", blocks, stream.Errors(), warmErrors);
	if (failed || stream.Failed() || blocks < 300 || !timing.Valid || !warmed
		|| stream.Errors() > warmErrors + 2) return 1;

	std::puts("PASS production AudioStream -> WASAPI");
	return 0;
}
