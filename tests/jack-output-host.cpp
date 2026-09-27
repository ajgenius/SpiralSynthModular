// Exercise the Output buttons through the real host and inspect JACK ports.
#include "SpiralSynthModular.h"
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
			usleep(1000);
		}

		return NULL;
	}

};

Fl_Button *FindButton(Fl_Widget *widget, const char *label)
{
	if (Fl_Button *button=dynamic_cast<Fl_Button *>(widget))
		if (button->label() && !strcmp(button->label(),label)) return button;

	if (Fl_DeviceGUI *device=dynamic_cast<Fl_DeviceGUI *>(widget))
		if (device->GetPluginWindow())
			if (Fl_Button *button=FindButton(device->GetPluginWindow(),label)) return button;

	if (Fl_Group *group=dynamic_cast<Fl_Group *>(widget))
		for (int n=0; n<group->children(); ++n)
			if (Fl_Button *button=FindButton(group->child(n),label)) return button;

	return NULL;
}

int main(int argc, char **argv)
{
	if (argc!=2) return 77;

	alarm(30);
	SpiralInfo::AUDIOCLIENT="jack";
	char name[64];
	snprintf(name,sizeof(name),"output-host-%ld",long(getpid()));
	SpiralInfo::OUTPUTFILE=name;
	spiralcore::JackClient observer;
	spiralcore::AudioClientOptions options;
	options.InChannels=2; options.OutChannels=0;
	assert(observer.Attach(std::string(name)+"-observer",options));
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
	const char *buttons[]={"Write","Dplx","Read","Write","Write"};

	const unsigned expectedInputs[]={0,2,2,0,0};

	const unsigned expectedOutputs[]={2,2,0,2,0};

	for (unsigned step=0; step<5; ++step)
	{
		Fl_Button *button=FindButton(host.Window,buttons[step]);
		assert(button);
		button->value(step!=4);
		// The main GUI loop polls while a real mouse press is held down.
		Fl::pushed(button);
		host.Synth.UpdatePluginGUIs();
		assert(button->value()==(step!=4));
		Fl::pushed(NULL);
		button->do_callback();
		usleep(200000);
		std::vector<std::string> inputs,outputs;
		observer.GetPortNames(inputs,outputs);
		const std::string prefix=std::string(name)+":";

		unsigned ins=0,outs=0;
		for (size_t n=0; n<inputs.size(); ++n) if (inputs[n].find(prefix)==0) ++ins;

		for (size_t n=0; n<outputs.size(); ++n) if (outputs[n].find(prefix)==0) ++outs;

		printf("Output %s: %u inputs, %u outputs\n",step==4 ? "Close" : buttons[step],ins,outs);
		assert(ins==expectedInputs[step] && outs==expectedOutputs[step]);
		assert(host.Synth.CallbackMode()==(step!=4));
	}

	host.Synth.ClearUp();
	__sync_lock_test_and_set(&host.Stop,1);
	pthread_join(thread,NULL);
	return 0;
}
