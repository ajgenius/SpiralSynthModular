#include "OutputPlugin.h"
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

	static void Run(void *context, AudioDriver *driver, unsigned frames)
	{
		Engine *engine=static_cast<Engine *>(context);
		if (!frames || pthread_mutex_trylock(&engine->Gate)) return;

		if (frames==unsigned(engine->Info.BUFSIZE))
		{
			driver->ProcessAudio();
			driver->Execute();
			__sync_fetch_and_add(&engine->Cycles,1);
		}

		pthread_mutex_unlock(&engine->Gate);
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
		plugin.SetBlockingCallback(NULL);
		plugin.SetParent(&engine);
		plugin.SetAudioCycleCallback(Engine::Run);
		plugin.SetChangeBufferAndSampleRateCallback(Engine::Format);
		plugin.Initialise(&engine.Info);
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
		printf("CoreAudio Output cycle %u: format negotiation, callbacks and explicit close PASS\n",cycle+1);
	}

	return 0;
}
