#include "OutputPlugin.h"
#include "AudioTransportHub.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <pthread.h>
#include <unistd.h>

struct Engine
{
	pthread_mutex_t Gate;
	HostInfo Info;
	bool FormatChanged;
	volatile unsigned Cycles;
	Engine() : Info(), FormatChanged(false), Cycles(0) { pthread_mutex_init(&Gate,NULL); }

	~Engine() { pthread_mutex_destroy(&Gate); }

	static void Format(unsigned long frames, unsigned long rate, void *context)
	{
		Engine *engine=static_cast<Engine *>(context);
		if (engine->Info.BUFSIZE!=int(frames) || engine->Info.SAMPLERATE!=int(rate))
		{
			engine->Info.BUFSIZE=frames;
			engine->Info.SAMPLERATE=rate;
			engine->FormatChanged=true;
		}

	}

	// The engine thread: one graph pass per transport period.
	AudioDriver *Driver;
	volatile int Stop;
	static void *Run(void *context)
	{
		Engine *engine=static_cast<Engine *>(context);
		AudioTransportHub *hub=AudioTransportHub::Get();
		while (!__sync_fetch_and_add(&engine->Stop,0))
		{
			if (!hub->WaitPeriod()) continue;

			pthread_mutex_lock(&engine->Gate);
			engine->Driver->Execute();
			if (hub->GetMode()==AudioTransportHub::OUTPUT) __sync_fetch_and_add(&engine->Cycles,1);

			hub->CommitPeriod();
			pthread_mutex_unlock(&engine->Gate);
		}

		return NULL;
	}

};

int main(int argc, char **argv)
{
	if (argc!=2 || strcmp(argv[1],"--live")) return 77;

	alarm(20);
	Engine engine;
	engine.Info.AUDIOCLIENT="coreaudio";
	engine.Info.OUTPUTFILE="default";
	engine.Info.BUFSIZE=512;
	engine.Info.SAMPLERATE=44100;
	for (unsigned cycle=0; cycle<3; ++cycle)
	{
		OutputPlugin plugin;
		plugin.SetParent(&engine);
		plugin.SetChangeBufferAndSampleRateCallback(Engine::Format);
		plugin.Initialise(&engine.Info);
		engine.Driver=&plugin; engine.Stop=0;
		pthread_t thread;
		assert(!pthread_create(&thread,NULL,Engine::Run,&engine));
		unsigned before=__sync_fetch_and_add(&engine.Cycles,0);
		for (unsigned wait=0; wait<200 && __sync_fetch_and_add(&engine.Cycles,0)<before+8; ++wait)
		{
			pthread_mutex_lock(&engine.Gate);
			plugin.ServiceAudio();
			if (engine.FormatChanged) { engine.FormatChanged=false; plugin.Reset(); }

			pthread_mutex_unlock(&engine.Gate);
			usleep(10000);
		}

		assert(__sync_fetch_and_add(&engine.Cycles,0)>=before+8);
		assert(plugin.GetMode()==OutputPlugin::OUTPUT);
		pthread_mutex_lock(&engine.Gate);
		plugin.GetChannelHandler()->SetCommand(OutputPlugin::CLOSE);
		plugin.UpdateChannelHandler(); plugin.ExecuteCommands();
		pthread_mutex_unlock(&engine.Gate);
		unsigned stopped=__sync_fetch_and_add(&engine.Cycles,0);
		usleep(1100000);
		plugin.ServiceAudio();
		assert(plugin.GetMode()==OutputPlugin::CLOSED);
		assert(__sync_fetch_and_add(&engine.Cycles,0)==stopped);
		__sync_lock_test_and_set(&engine.Stop,1);
		pthread_join(thread,NULL);
		printf("CoreAudio Output cycle %u: format negotiation, callbacks and explicit close PASS\n",cycle+1);
	}

	return 0;
}
