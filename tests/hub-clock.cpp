// The engine must keep its format and frame count without an Output device.
#include "AudioTransportHub.h"
#include <cassert>
#include <cstdio>
#include <sys/time.h>

static double Now()
{
	struct timeval now;
	gettimeofday(&now, NULL);
	return now.tv_sec + now.tv_usec / 1000000.0;
}

int main()
{
	HostInfo host = HostInfo();
	host.BUFSIZE = 960;
	host.SAMPLERATE = 48000;
	AudioTransportHub *hub = AudioTransportHub::Get();
	hub->SetHost(&host);
	const double start = Now();
	for (unsigned n = 0; n < 10; ++n)
	{
		assert(hub->WaitPeriod());
		hub->BeginPeriod();
		hub->CommitPeriod();
	}

	const double elapsed = Now() - start;
	printf("No Output: 9600 frames at 48 kHz in %.4f seconds\n", elapsed);
	assert(hub->Frame() == 9600);
	assert(elapsed >= 0.18 && elapsed < 0.5);
	hub->SetHost(NULL);
	return 0;
}
