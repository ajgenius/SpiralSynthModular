// Exercise the real plugin with the deterministic JACK client fixture.
// The host reads ALWAYS drivers before rendering the graph, then calls Execute.
#define JACK_CLIENT_TEST_FIXTURE
#include "../libspiralcore/tests/jack-client.cpp"
#include "JackPlugin.h"
#include <cstdio>

static void CheckEqualPeriods()
{
	HostInfo host = HostInfo();
	host.BUFSIZE = 4;
	host.SAMPLERATE = 48000;
	JackPlugin plugin;
	plugin.Initialise(&host);
	plugin.Attach();
	jack_client_t *native = clients["SSM0"];
	assert(native && native->Active);

	Sample signal(host.BUFSIZE);
	for (unsigned channel = 0; channel < 4; ++channel)
		assert(plugin.SetInput(channel, &signal));

	// A changing signal detects duplicated and stale periods which a constant
	// signal cannot reveal. One native period must deliver one graph period.
	for (unsigned cycle = 1; cycle <= 64; ++cycle)
	{
		plugin.ProcessAudio();
		signal.Set(float(cycle));
		plugin.Execute();
		native->Process(4, native->ProcessContext);
		for (unsigned channel = 4; channel < 8; ++channel)
			for (unsigned frame = 0; frame < 4; ++frame)
			{
				const float actual = native->Ports[channel]->Buffer[frame];
				if (actual != float(cycle))
				{
					printf("period %u: expected %.0f, received %.0f\n", cycle, float(cycle), actual);
					assert(actual == float(cycle));
				}

			}

		assert(plugin.GetCaptureFrame() == (cycle - 1) * 4);
	}

	puts("JACK transfers one changing period per graph cycle PASS");
}

static void CheckDifferentPeriods(unsigned frames)
{
	HostInfo host = HostInfo();
	host.BUFSIZE = frames;
	host.SAMPLERATE = 48000;
	JackPlugin a, b;
	a.Initialise(&host);
	b.Initialise(&host);
	a.Attach();
	b.Attach();
	assert(clients.size() == 2);
	jack_client_t *nativeA = clients.begin()->second;
	jack_client_t *nativeB = clients.rbegin()->second;
	Sample signal(frames);
	a.SetInput(0, &signal);
	b.SetInput(0, &signal);

	unsigned written = 0;
	unsigned played = 0;
	for (unsigned cycle = 0; cycle < 96; ++cycle)
	{
		a.ProcessAudio();
		b.ProcessAudio();
		for (unsigned frame = 0; frame < frames; ++frame)
			signal.Set(frame, float(++written));

		a.Execute();
		b.Execute();
		while (written - played >= 4)
		{
			nativeA->Process(4, nativeA->ProcessContext);
			nativeB->Process(4, nativeB->ProcessContext);
			for (unsigned frame = 0; frame < 4; ++frame)
			{
				const float expected = float(++played);
				assert(nativeA->Ports[4]->Buffer[frame] == expected);
				assert(nativeB->Ports[4]->Buffer[frame] == expected);
			}
		}
	}

	printf("Two JACK devices, host %u/native 4: %u ordered frames PASS\n", frames, played);
}

int main()
{
	CheckEqualPeriods();
	CheckDifferentPeriods(2);
	CheckDifferentPeriods(6);
	return 0;
}
