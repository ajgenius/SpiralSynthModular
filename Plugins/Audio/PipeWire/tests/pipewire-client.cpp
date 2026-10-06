// SPDX-License-Identifier: GPL-2.0-or-later
#include "PipeWireClient.h"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <unistd.h>
using namespace spiralcore;

struct Probe
{
	PipeWireClient Client;
	std::atomic<unsigned> Calls;
	unsigned Inputs;
	float Samples[32768];
	double Previous;

	Probe():
	    Calls(0),
	    Inputs(0),
	    Previous(0)
	{
	}

	static void Run(void *context, unsigned frames)
	{
		Probe &p = *static_cast<Probe *>(context);
		AudioCycleTiming timing;
		assert(frames && frames <= 16384);
		assert(p.Client.GetCycleTiming(timing) && timing.Valid);
		assert(std::isfinite(timing.OutputTime) && timing.OutputTime > 0);
		assert(timing.OutputTime >= p.Previous);
		p.Previous = timing.OutputTime;
		if (p.Inputs)
		{
			assert(p.Client.Read(p.Samples, frames));
			for (unsigned n = 0; n < frames * p.Inputs; ++n)
				assert(std::isfinite(p.Samples[n]));
		}
		for (unsigned n = 0; n < frames * 2; ++n)
			p.Samples[n] = 0.125f;
		assert(p.Client.Write(p.Samples, frames));
		++p.Calls;
	}
};

int main()
{
	Probe probe;
	AudioClientOptions options;
	options.InChannels = 3;
	assert(!probe.Client.Attach("", options));
	for (unsigned pass = 0; pass < 3; ++pass)
	{
		options.BufferSize = 128;
		options.Samplerate = pass == 1 ? 44100 : 48000;
		options.InChannels = probe.Inputs = pass == 1 ? 2 : 0;
		options.OutChannels = 2;
		probe.Calls = 0;
		probe.Previous = 0;
		assert(probe.Client.Attach("", options));
		probe.Client.SetCallback(Probe::Run, &probe);
		assert(probe.Client.Start());
		for (unsigned n = 0; n < 500 && probe.Calls.load() < 100; ++n)
			usleep(10000);
		assert(probe.Calls.load() >= 100);
		assert(probe.Client.GetBufferSize() > options.BufferSize);
		probe.Client.Detach();
		const unsigned stopped = probe.Calls.load();
		usleep(20000);
		assert(probe.Calls.load() == stopped);
		probe.Client.Detach();
	}
	std::puts("PipeWire isolated server: timing, duplex, negotiated quantum and teardown PASS");
}
