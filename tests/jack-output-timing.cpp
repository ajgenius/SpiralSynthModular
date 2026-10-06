// Check callback output continuity while the real host polls many controls.
#include "SpiralSynthModular.h"
#include "AudioTransportHub.h"
#include "SpiralInfo.h"
#include "SpiralPluginGUI.h"
#include "JackClient.h"
#include <cstring>
#include <cassert>
#include <cstdio>
#include <sstream>
#include <pthread.h>
#include <unistd.h>

struct Host
{
	SynthModular Synth;
	Fl_Group *Window;
	volatile int Stop;
	Host() : Stop(0) { Window=Synth.CreateWindow(); }

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

struct Probe
{
	spiralcore::JackClient Client;
	std::vector<float> Buffer;
	volatile unsigned Good, Silent;
	Probe() : Good(0), Silent(0) {}

	static void Run(void *context, unsigned frames)
	{
		Probe *probe=static_cast<Probe *>(context);
		if (!frames || frames*2>probe->Buffer.size()) return;

		if (!probe->Client.Read(&probe->Buffer[0],frames)) return;

		bool silent=true;
		for (unsigned n=0; n<frames*2; ++n) if (probe->Buffer[n]!=0) silent=false;

		__sync_fetch_and_add(silent ? &probe->Silent : &probe->Good,1);
	}

};

int main(int argc, char **argv)
{
	if (argc!=2) return 77;

	alarm(30);
	SpiralInfo::AUDIOCLIENT="jack";
	char name[64];
	snprintf(name,sizeof(name),"output-host-%ld",long(getpid()));
	SpiralInfo::OUTPUTFILE=name;
	Probe probe;
	spiralcore::JackClient &observer=probe.Client;
	spiralcore::AudioClientOptions options;
	options.InChannels=2; options.OutChannels=0;
	assert(observer.Attach(std::string(name)+"-observer",options));
	probe.Buffer.resize(observer.GetBufferSize()*2);
	observer.SetCallback(Probe::Run,&probe);
	assert(observer.Start());
	Host host;
	host.Synth.LoadPlugins(argv[1]);
	pthread_t thread;
	assert(!pthread_create(&thread,NULL,Host::Run,&host));
	std::stringstream patch;
	// Many idle control channels stress polling without adding DSP cost.
	patch << "SpiralSynthModular File Ver 4\n0 0 700 600 0 0 0 0\nSectionList 130\n"
		"Device 0 Plugin 3\n20 20 10 Controller 0 0 0\n5 1\n4 Test -1 1 0.25\n"
		"Device 1 Plugin 0\n200 20 6 Output 0 0 0\n\n";
	for (unsigned n=2; n<130; ++n)
		patch << "Device " << n << " Plugin 3\n20 20 4 Idle 0 0 0\n5 1\n4 Zero -1 1 0\n";

	patch << "-1 0 2\n0 0 0 0 1 0 0 0\n0 0 0 0 1 0 1 0\n";
	host.Synth.StreamPatchIn(patch,false,false);
	usleep(300000);
	observer.ConnectInput(0,std::string(name)+":Out0");
	observer.ConnectInput(1,std::string(name)+":Out1");
	usleep(300000);
	const unsigned good=__sync_fetch_and_add(&probe.Good,0);

	const unsigned silent=__sync_fetch_and_add(&probe.Silent,0);
	for (unsigned n=0; n<500; ++n)
	{
		host.Synth.UpdatePluginGUIs();
		usleep(10000);
	}

	const unsigned received=__sync_fetch_and_add(&probe.Good,0)-good;

	const unsigned gaps=__sync_fetch_and_add(&probe.Silent,0)-silent;
	printf("JACK constant output: %u signal blocks, %u silent gaps\n",received,gaps);
	assert(received>100 && gaps==0);

	// The engine frame is jack's transport frame: locate moves it, start rolls it.
	AudioTransportHub *hub=AudioTransportHub::Get();
	hub->Stop(); usleep(100000);
	hub->Locate(100000); usleep(100000);
	const unsigned long located=hub->Frame();
	hub->Start(); usleep(200000);
	const unsigned long rolled=hub->Frame();
	const bool rolling=hub->Rolling();
	printf("JACK transport: located %lu, rolled to %lu, rolling %d\n",located,rolled,int(rolling));
	assert(located==100000 && rolled>located+4000 && rolling);
	host.Synth.ClearUp();
	__sync_lock_test_and_set(&host.Stop,1);
	pthread_join(thread,NULL);
	return 0;
}
