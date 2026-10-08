// Real host hub, plugin and client against deterministic JACK periods.
#define JACK_CLIENT_TEST_FIXTURE
#include "../libs/libspiralcore/Audio/tests/jack-client.cpp"
#include "JackPlugin.h"
#include "AudioTransportHub.h"
#include <cmath>
#include <cstdio>

static void Check(unsigned frames)
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
	AudioTransportHub *hub = AudioTransportHub::Get();
	cycleTimeOffset = spiralcore::AudioMonotonicTime();
	unsigned audible = 0, rendered = 0;
	for (cycleFrame = 0; cycleFrame < 6000; cycleFrame += 4)
	{
		nativeA->Process(4, nativeA->ProcessContext);
		nativeB->Process(4, nativeB->ProcessContext);
		if (cycleFrame > 1200)
			for (unsigned n = 0; n < 4; ++n)
			{
				const float first = nativeA->Ports[4]->Buffer[n];
				const float second = nativeB->Ports[4]->Buffer[n];
				assert(first == second);
				if (std::fabs(first) > 0.01) ++audible;

			}

		const double now = cycleTimeOffset + cycleFrame / 48000.0;
		while (hub->PreparePeriod(now))
		{
			hub->BeginPeriod();
			a.ProcessAudio();
			b.ProcessAudio();
			for (unsigned n = 0; n < frames; ++n)
				signal.Set(n, std::sin((rendered + n) * 0.03));

			a.Execute();
			b.Execute();
			assert(a.GetCaptureFrame() == hub->CaptureStamp().Frame);
			hub->CommitPeriod();
			rendered += frames;
		}

	}

	assert(audible > 4000);
	assert(rendered > 5800 && rendered < 6200);
	printf("Two JACK clients: host %u/native 4, %u aligned changing samples PASS\n", frames, audible);
	a.Detach();
	b.Detach();
	hub->SetHost(NULL);
}
int main()
{
	Check(4);
	Check(2);
	Check(6);
}
