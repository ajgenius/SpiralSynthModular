// The WaveShaper GUI is an editor of the device's channel, not of its
// class: the module loads with no DSP module in the process, and after
// a load the knobs show what the device streamed in.
#include "SpiralSynthModular.h"
#include "SpiralPluginGUI.h"
#include "Fl_Knob.H"
#include "Fl_LED_Button.H"
#include <dlfcn.h>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <sstream>

static void Collect(Fl_Group *group, std::vector<Fl_Knob *> &knobs, std::vector<Fl_LED_Button *> &leds)
{
	for (int n=0; n<group->children(); ++n)
	{
		Fl_Widget *child=group->child(n);
		if (Fl_Knob *knob=dynamic_cast<Fl_Knob *>(child)) knobs.push_back(knob);
		else if (Fl_LED_Button *led=dynamic_cast<Fl_LED_Button *>(child)) leds.push_back(led);
		else if (Fl_Group *inner=dynamic_cast<Fl_Group *>(child)) Collect(inner,knobs,leds);
	}
}

int main(int argc, char **argv)
{
	if (argc != 2) return 77;

	std::string root=argv[1];
	// No DSP module loaded yet: an editor must not need one to link.
	void *ui=dlopen((root + "/panels/WaveShaperPlugin/WaveShaperPlugin_GUI.so").c_str(), RTLD_NOW | RTLD_GLOBAL);
	if (!ui) { puts(dlerror()); return 1; }
	puts("GUI module loads with no DSP module present");

	void *dsp=dlopen((root + "/dsp/WaveShaperPlugin/WaveShaperPlugin_DSP.so").c_str(), RTLD_NOW | RTLD_GLOBAL);
	if (!dsp) { puts(dlerror()); return 1; }

	SpiralPlugin *(*create)()=(SpiralPlugin *(*)())dlsym(dsp, "SpiralPlugin_CreateInstance");
	SpiralGUIType *(*createGUI)(SpiralPlugin *)=(SpiralGUIType *(*)(SpiralPlugin *))dlsym(ui, "SpiralPlugin_CreateGUI");
	assert(create && createGUI);
	HostInfo info=HostInfo();
	info.BUFSIZE=256;
	info.SAMPLERATE=44100;
	info.GUI_COLOUR=FL_GRAY;
	SpiralPlugin *plugin=create();
	plugin->Initialise(&info);

	Fl_Double_Window window(300,300,"WaveShaper editor test");
	SpiralGUIType *gui=createGUI(plugin);
	window.end();
	assert(gui);

	// What a patch file carries: version, wave, six coefficients.
	const float coefs[6]={0.5f, 0.25f, 0.125f, 0.0625f, 0.03125f, 0.015625f};
	spiralcore::Description state;
	state.Value(1).Value(0);
	for (int n=0; n<6; ++n) state.Value(coefs[n]);
	spiralcore::Description::Reader reader(state);
	plugin->Apply(reader);
	// The host's sequence after Apply.
	plugin->GetChannelHandler()->FlushChannels();
	static_cast<SpiralPluginGUI *>(gui)->UpdateValues(plugin);

	std::vector<Fl_Knob *> knobs;
	std::vector<Fl_LED_Button *> leds;
	Collect(gui,knobs,leds);
	assert(knobs.size()==6 && leds.size()==2);
	for (int n=0; n<6; ++n)
	{
		printf("knob %d: %g (streamed %g)\n", n, knobs[n]->value(), coefs[n]);
		assert(fabs(knobs[n]->value()-coefs[n]) < 1e-6);
	}
	// Wave 0 is sines: the second LED.
	assert(leds[0]->value()==0 && leds[1]->value()==1);

	delete gui;
	delete plugin;
	puts("WaveShaper editor shows the loaded state through the channel PASS");
	return 0;
}
