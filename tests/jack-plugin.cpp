// Exercise the client adapter without property-state or SSMJ dependencies.
#include "SpiralPlugin.h"
#include "JackPlugin.h"
#include "OutputPlugin.h"
#include <cassert>
#include <cstdio>
#include <dlfcn.h>
#include <pthread.h>
#include <unistd.h>

struct Engine
{
	pthread_mutex_t Gate;
	static void Run(void *context, AudioDriver *driver, unsigned frames)
	{
		Engine *engine=static_cast<Engine *>(context);
		if (!frames || pthread_mutex_trylock(&engine->Gate)) return;

		driver->ProcessAudio();
		driver->Execute();
		pthread_mutex_unlock(&engine->Gate);
	}

};

struct Capture
{
	JackClient Client;
	std::vector<float> Buffer, Source;
	volatile unsigned Matches;
	Capture() : Matches(0) {}

	static void Run(void *context, unsigned frames)
	{
		Capture *capture=static_cast<Capture *>(context);
		if (!frames || frames*2>capture->Buffer.size()) return;

		assert(capture->Client.Write(&capture->Source[0],frames));
		if (!capture->Client.Read(&capture->Buffer[0],frames)) return;

		for (unsigned n=0; n<frames*2; ++n)
			if (capture->Buffer[n]!=0.25f) return;

		__sync_fetch_and_add(&capture->Matches,1);
	}

};

int main(int argc, char **argv)
{
	if (argc!=2 && argc!=3) return 77;

	const bool output=argc==3;
	alarm(20);
	void *module=dlopen((std::string(argv[1])+(output ? "/dsp/OutputPlugin/OutputPlugin_DSP.so" : "/dsp/JackPlugin/JackPlugin_DSP.so")).c_str(),RTLD_NOW|RTLD_GLOBAL);
	if (!module) { puts(dlerror()); return 1; }

	SpiralPlugin *(*create)()=(SpiralPlugin *(*)())dlsym(module,"SpiralPlugin_CreateInstance");
	assert(create);
	Capture capture;
	spiralcore::AudioClientOptions options;
	options.InChannels=2; options.OutChannels=2;
	char name[64];
	snprintf(name,sizeof(name),"plugin-test-%ld",long(getpid()));
	assert(capture.Client.Attach(name,options));
	capture.Buffer.resize(capture.Client.GetBufferSize()*2);
	capture.Source.assign(capture.Buffer.size(),-0.375f);
	capture.Client.SetCallback(Capture::Run,&capture);
	assert(capture.Client.Start());
	HostInfo info=HostInfo();
	info.BUFSIZE=capture.Client.GetBufferSize();
	info.SAMPLERATE=capture.Client.GetSampleRate();
	info.AUDIOCLIENT="jack";
	info.OUTPUTFILE=std::string(name)+"-output";
	Engine engine;
	pthread_mutex_init(&engine.Gate,NULL);
	for (unsigned cycle=0; cycle<3; ++cycle)
	{
		SpiralPlugin *plugin=create();
		plugin->SetBlockingCallback(NULL);
		plugin->Initialise(&info);
		plugin->SetParent(&engine);
		AudioDriver *driver=dynamic_cast<AudioDriver *>(plugin);
		assert(driver && driver->IsCallbackDriver());
		driver->SetAudioCycleCallback(Engine::Run);
		ChannelHandler *channel=plugin->GetChannelHandler();
		pthread_mutex_lock(&engine.Gate);
		if (!output)
		{
		channel->Set("NumInputs",6); channel->Set("NumOutputs",10);
		channel->SetCommand(JackPlugin::SET_PORT_COUNT);
		plugin->UpdateChannelHandler(); plugin->ExecuteCommands();
		}

		Sample source(info.BUFSIZE); source.Set(0.25f);
		for (unsigned n=0; n<(output ? 2U : 6U); ++n) assert(plugin->SetInput(n,&source));

		Sample *received[2];
		if (!output)
			for (unsigned n=0; n<2; ++n) assert(plugin->GetOutput(n,&received[n]));

		if (output) channel->Set("Volume",1.0f);

		channel->SetCommand(output ? int(OutputPlugin::SET_VOLUME) : int(JackPlugin::ATTACH));
		plugin->UpdateChannelHandler(); plugin->ExecuteCommands();
		driver->ServiceAudio();
		pthread_mutex_unlock(&engine.Gate);
		std::vector<std::string> inputs,outputs;
		capture.Client.GetPortNames(inputs,outputs);
		std::string prefix=output ? info.OUTPUTFILE+":" : "SSM"+std::string(1,char('0'+cycle))+":";

		unsigned ins=0,outs=0;
		for (size_t n=0;n<inputs.size();++n) if (inputs[n].find(prefix)==0) ++ins;

		for (size_t n=0;n<outputs.size();++n) if (outputs[n].find(prefix)==0) ++outs;

		assert(ins==(output ? 0U : 10U) && outs==(output ? 2U : 6U));
		capture.Client.ConnectInput(0,prefix+"Out0");
		capture.Client.ConnectInput(1,prefix+"Out1");
		if (!output)
		{
			capture.Client.ConnectOutput(0,prefix+"In0");
			capture.Client.ConnectOutput(1,prefix+"In1");
		}

		unsigned before=__sync_fetch_and_add(&capture.Matches,0);
		for (unsigned wait=0; wait<100 && __sync_fetch_and_add(&capture.Matches,0)<before+3; ++wait) usleep(10000);

		assert(__sync_fetch_and_add(&capture.Matches,0)>=before+3);
		pthread_mutex_lock(&engine.Gate);
		if (!output)
		{
			// A blocking Output holds the host gate while waiting for its device.
			// JACK must retain capture without entering the graph during that wait.
			for (unsigned n=0; n<2; ++n) received[n]->Set(0);

			usleep(150000);
			driver->Execute();
			for (unsigned n=0; n<2; ++n)
				for (unsigned frame=0; frame<info.BUFSIZE; ++frame)
					assert((*received[n])[frame]==-0.375f);

		}

		plugin->Kill(); delete plugin;
		pthread_mutex_unlock(&engine.Gate);
		printf("JACK %s cycle %u: %s PASS\n",output ? "Output" : "plugin",cycle+1,
			output ? "playback" : "playback and capture with the host gate held");
	}

	capture.Client.Detach();
	pthread_mutex_destroy(&engine.Gate);
	return 0;
}
