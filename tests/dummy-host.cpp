// The built-in dummy backend runs the real host with no audio device:
// Output opens on it, the engine advances at the nominal rate, and a
// switch to a backend that is not there falls back without stopping.
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
			host->Synth.Update();

		return NULL;
	}

};

int main(int argc, char **argv)
{
	if (argc!=2) return 77;

	alarm(30);
	SpiralInfo::AUDIOCLIENT="dummy";
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
	usleep(300000);
	AudioTransportHub *hub=AudioTransportHub::Get();
	assert(hub->GetMode()==AudioTransportHub::OUTPUT && !hub->IsCallbackDriven());

	const unsigned underruns=hub->Underruns();
	const unsigned long before=hub->Frame();
	usleep(500000);
	const unsigned long advanced=hub->Frame()-before;
	const double rate=advanced/0.5;
	printf("dummy host: engine advanced %lu frames in 500 ms (%.0f Hz, nominal %d), %u underruns\n",
		advanced,rate,SpiralInfo::SAMPLERATE,hub->Underruns()-underruns);
	assert(rate>SpiralInfo::SAMPLERATE*0.8 && rate<SpiralInfo::SAMPLERATE*1.2);

	SpiralInfo::AUDIOCLIENT="nonesuch";
	host.Synth.UpdateHostInfo();
	usleep(300000);
	assert(hub->GetMode()==AudioTransportHub::OUTPUT);
	printf("dummy host: unknown backend fell back, still streaming\n");

	host.Synth.ClearUp();
	__sync_lock_test_and_set(&host.Stop,1);
	pthread_join(thread,NULL);
	return 0;
}
