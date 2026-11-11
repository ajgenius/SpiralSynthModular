#include "AudioTransportHub.h"
#include <cassert>
#include <cstdio>
#include "AudioTiming.h"

int main()
{
	HostInfo host = HostInfo();
	host.BUFSIZE = 960;
	host.SAMPLERATE = 48000;
	AudioTransportHub *hub = AudioTransportHub::Get();
	hub->SetHost(&host);
	const double start = Spiral::Core::AudioMonotonicTime();
	for (unsigned n = 0; n < 10; ++n)
	{
		assert(hub->WaitPeriod());
		hub->BeginPeriod();
		hub->CommitPeriod();
	}

	const double elapsed = Spiral::Core::AudioMonotonicTime() - start;
	printf("No Output: 9600 frames at 48 kHz in %.4f seconds\n", elapsed);
	assert(hub->Frame() == 9600);
	// The hub's own clock, and its allowance for a period that is due within
	// 10 microseconds: the wall clock can be slewed, and nine periods of waiting
	// come to exactly the lower limit.
	assert(elapsed >= 0.18 - 0.00002 && elapsed < 0.5);
	hub->SetHost(NULL);
	return 0;
}
