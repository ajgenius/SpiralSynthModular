// Exercise backend changes through the real host control and callback loops.
#include "SpiralSynthModular.h"
#include "AudioTransportHub.h"
#include "SpiralInfo.h"
#include <cassert>
#include <cstdio>
#include <sstream>
#include <pthread.h>
#include <unistd.h>

struct Host
{
	SynthModular Synth;
	volatile int Stop;
	Host() : Stop(0) { Synth.CreateWindow(); }

	static void *Run(void *context)
	{
		Host *host=static_cast<Host *>(context);
		while (!__sync_fetch_and_add(&host->Stop,0))
		{
			host->Synth.Update();
		}

		return NULL;
	}

};

int main(int argc, char **argv)
{
	if (argc!=2) return 77;

	alarm(30);
	SpiralInfo::AUDIOCLIENT="portaudio";
	SpiralInfo::OUTPUTFILE="default";
	Host host;
	host.Synth.LoadPlugins(argv[1]);
	pthread_t thread;
	assert(!pthread_create(&thread,NULL,Host::Run,&host));
	std::stringstream patch;
	patch << "SpiralSynthModular File Ver 4\n0 0 700 600 0 0 0 0\nSectionList 2\n"
		"Device 0 Plugin 3\n20 20 10 Controller 0 0 0\n5 1\n4 Zero -1 1 0\n"
		"Device 1 Plugin 0\n200 20 6 Output 0 0 0\n\n"
		"-1 0 2\n0 0 0 0 1 0 0 0\n0 0 0 0 1 0 1 0\n";
	host.Synth.StreamPatchIn(patch,false,false);
	usleep(200000);
	for (unsigned cycle=0; cycle<3; ++cycle)
	{
		SpiralInfo::AUDIOCLIENT="coreaudio";
		host.Synth.UpdateHostInfo();
		usleep(300000);
		AudioTransportHub *hub=AudioTransportHub::Get();
		unsigned streaming=0;
		const unsigned underruns=hub->Underruns();
		for (unsigned n=0; n<100; ++n)
		{
			if (hub->GetMode()==AudioTransportHub::OUTPUT && hub->IsCallbackDriven()) ++streaming;

			usleep(10000);
		}

		printf("CoreAudio host switch %u: callback transport streaming %u/100 polls, %u underruns\n",cycle+1,streaming,hub->Underruns()-underruns);
		assert(streaming>=95);
		SpiralInfo::AUDIOCLIENT="portaudio";
		host.Synth.UpdateHostInfo();
		usleep(200000);
		assert(hub->GetMode()==AudioTransportHub::OUTPUT && !hub->IsCallbackDriven());
	}

	host.Synth.ClearUp();
	__sync_lock_test_and_set(&host.Stop,1);
	pthread_join(thread,NULL);
	return 0;
}
