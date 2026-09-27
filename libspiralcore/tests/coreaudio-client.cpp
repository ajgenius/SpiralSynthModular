#include "CoreAudioClient.h"
#include <cassert>
#include <cstring>
#include <cstdio>
#include <unistd.h>

struct Probe
{
	spiralcore::CoreAudioClient Client;
	std::vector<float> Silence;
	volatile unsigned Cycles, Stops, Errors;
	Probe() : Silence(131072,0), Cycles(0), Stops(0), Errors(0) {}

	static void Run(void *context, unsigned frames)
	{
		Probe *probe=static_cast<Probe *>(context);
		if (!frames) { __sync_fetch_and_add(&probe->Stops,1); return; }

		if (frames*2>probe->Silence.size() || !probe->Client.Write(&probe->Silence[0],frames))
			__sync_fetch_and_add(&probe->Errors,1);

		__sync_fetch_and_add(&probe->Cycles,1);
	}

};

int main(int argc, char **argv)
{
	Probe probe;
	spiralcore::AudioClientOptions options;
	options.InChannels=options.OutChannels=0;
	assert(!probe.Client.Attach("default",options));
	options.OutChannels=2;
	assert(!probe.Client.Attach("ssm-invalid-device-uid",options));
	assert(!probe.Client.Start());
	assert(!probe.Client.Write(&probe.Silence[0],512));
	assert(!probe.Client.Read(&probe.Silence[0],512));
	probe.Client.Detach(); probe.Client.Detach();
	if (argc!=2 || strcmp(argv[1],"--live")) return 0;

	alarm(20);
	probe.Client.SetCallback(Probe::Run,&probe);
	for (unsigned n=0; n<3; ++n)
	{
		assert(probe.Client.Attach("default",options));
		assert(probe.Client.IsCallbackDriven());
		assert(probe.Client.GetSampleRate() && probe.Client.GetBufferSize());
		unsigned before=__sync_fetch_and_add(&probe.Cycles,0);
		assert(probe.Client.Start());
		for (unsigned wait=0; wait<200 && __sync_fetch_and_add(&probe.Cycles,0)<before+8; ++wait) usleep(10000);

		assert(__sync_fetch_and_add(&probe.Cycles,0)>=before+8);
		probe.Client.Detach();
		unsigned stopped=__sync_fetch_and_add(&probe.Cycles,0);
		usleep(50000);
		assert(__sync_fetch_and_add(&probe.Cycles,0)==stopped);
		assert(!probe.Client.IsAttached() && probe.Stops==n+1 && !probe.Errors);
		printf("CoreAudio cycle %u: native callbacks, stereo writes and teardown PASS\n",n+1);
	}

	return 0;
}
