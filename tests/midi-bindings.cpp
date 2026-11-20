// SPDX-License-Identifier: GPL-2.0-or-later
#include "SpiralSynthModular.h"
#include "SpiralInfo.h"
#include "Midi.h"
#include "FltkView.h"
#include "PatchFile.h"
#include "Plugins/Devices/PianoRollPlugin/PianoRollPlugin.h"
#include "PanelControl.h"
#include <FL/Fl_Valuator.H>
#include <cassert>
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <deque>
#include <pthread.h>
#include <unistd.h>

using namespace Spiral::Core;

void Describe(PatchDescription &patch, SynthModular &o, const string &sidecars);

// Heard from the test, polled from the engine thread.
struct Script : public MidiBackend
{
	pthread_mutex_t lock;
	std::deque<MidiPacket> in;
	Script() { pthread_mutex_init(&lock, NULL); }
	~Script() { pthread_mutex_destroy(&lock); }
	void Hear(const MidiPacket &p) { pthread_mutex_lock(&lock); in.push_back(p); pthread_mutex_unlock(&lock); }
	std::vector<std::string> Ports(bool) { return std::vector<std::string>(1, "surface"); }
	void Select(const std::string &, const std::string &) {}
	bool Poll(MidiPacket &p)
	{
		pthread_mutex_lock(&lock);
		const bool any = !in.empty();
		if (any) { p = in.front(); in.pop_front(); }
		pthread_mutex_unlock(&lock);
		return any;
	}
	bool Send(const MidiPacket &) { return true; }
	std::string Status() { return "script"; }
};
static Script *script = NULL;
static void *Create(void *) { return static_cast<MidiBackend *>(script = new Script); }
static void Destroy(void *p) { delete static_cast<MidiBackend *>(p); if (p == script) script = NULL; }

struct Host
{
	SynthModular Synth;
	volatile int Stop;
	Host() : Stop(0) { Synth.CreateWindow(); }

	static void *Run(void *context)
	{
		Host *host = static_cast<Host *>(context);
		while (!__sync_fetch_and_add(&host->Stop, 0))
			host->Synth.Update();
		return NULL;
	}
};

static void Hear(unsigned char status, unsigned char controller, unsigned char value)
{
	script->Hear(MidiPacket(status, controller, value));
}

// The engine polls on its own; collecting here as well makes sure this
// pass has heard everything said before it.
static void Pass(SynthModular &synth)
{
	MidiDevice::Get()->Collect();
	synth.ServiceMidiInput();
}

static int learnedChannel = 0, learnedController = -1;
static void Learned(int channel, int controller, void *)
{
	learnedChannel = channel;
	learnedController = controller;
}

static void CountKnob(Fl_Widget *, void *data)
{
	++*static_cast<int *>(data);
}

struct TapLog
{
	Fl_Valuator *Knob;
	std::vector<double> Values;
};

static void CountTap(Fl_Widget *, void *data)
{
	TapLog *log = static_cast<TapLog *>(data);
	log->Values.push_back(log->Knob->value());
}

static Fl_Widget *Widget(SynthModular &synth, const std::string &device, const char *control)
{
	Spiral::Panels::PanelControl *target = synth.MidiControl(device, control);
	assert(target && target->Drivable());
	return Spiral::Panels::FltkView::WidgetOf(*target);
}

int main(int argc, char **argv)
{
	alarm(30);
	const BackendDescriptor backend = { SPIRAL_MIDI_PLUGIN_ABI, "midi", "script", Create, Destroy };
	MidiBackendRegistry::Get()->Register(&backend);
	SpiralInfo::AUDIOCLIENT = "dummy";
	SpiralInfo::MIDIBACKEND = "script";

	Host host;
	SynthModular &synth = host.Synth;
	synth.LoadPlugins(argc > 1 ? argv[1] : "tests/surface-modules");
	pthread_t thread;
	assert(!pthread_create(&thread, NULL, Host::Run, &host));
	assert(MidiDevice::Get() && MidiDevice::Get()->GetBackendName() == "script");

	synth.AddDevice(11, 50, 50);
	synth.AddDevice(288, 100, 100);
	Fl_Valuator *knob = dynamic_cast<Fl_Valuator *>(Widget(synth, "0", "resonance-knob"));
	assert(knob);
	int turns = 0;
	TapLog taps;
	taps.Knob = knob;
	knob->callback(CountKnob, &turns);
	Widget(synth, "1", "play")->callback(CountTap, &taps);

	synth.LearnMidi(Learned, NULL);
	Hear(0x90, 60, 100);
	Hear(0xb1, 71, 127);
	Pass(synth);
	assert(learnedChannel == 2 && learnedController == 71 && taps.Values.empty());
	synth.MidiMap().Bind("1", MidiBindingDescription("play", learnedChannel, learnedController));
	synth.MidiMap().Bind("0", MidiBindingDescription("resonance-knob", 0, 74));

	Hear(0xb0, 71, 127); // Wrong channel.
	Hear(0xb1, 72, 127); // Wrong CC.
	Pass(synth);
	assert(taps.Values.empty());

	// Set the knob low, then move it on two channels before the first tap.
	Hear(0xb0, 74, 0);
	Pass(synth);
	turns = 0;
	Hear(0xb9, 74, 32);
	Hear(0xb0, 74, 127);
	Hear(0xb1, 71, 127);
	Hear(0xb1, 71, 100); // A held button fires once.
	Hear(0xb1, 71, 0);
	Hear(0xb0, 74, 0);
	Hear(0xb1, 71, 127);
	Hear(0xb1, 71, 0);
	Pass(synth);
	assert(turns == 2 && taps.Values.size() == 2);
	assert(taps.Values[0] == knob->maximum() && taps.Values[1] == knob->minimum());

	Hear(0xb1, 71, 127);
	Pass(synth);
	Hear(0xb1, 71, 0);
	Pass(synth);
	assert(taps.Values.size() == 3);

	PatchDescription patch, restored;
	Describe(patch, synth, "");
	std::string json, error;
	assert(Spiral::File::PatchFile::Text(patch, json, error));
	assert(Spiral::File::PatchFile::ReadText(json, restored, error));
	synth.ClearUp();
	synth.Apply(restored, "");
	synth.Apply(restored, "", true);

	// Stop the audio loop at a block boundary so we can inspect the commands
	// delivered by the real Play callbacks on both restored devices.
	synth.FreezeAll();
	std::vector<SpiralPlugin *> players;
	int knobs = 0;
	for (size_t i = 0; i < synth.MidiMap().Devices.size(); ++i)
	{
		const std::string &id = synth.MidiMap().Devices[i].Device;
		if (synth.MidiMap().Binding(id, "play"))
		{
			SpiralPlugin *player = synth.cb_FindDevice(std::atoi(id.c_str()), &synth);
			assert(player && Widget(synth, id, "play"));
			players.push_back(player);
		}
		if (synth.MidiMap().Binding(id, "resonance-knob"))
		{
			++knobs;
			assert(Widget(synth, id, "resonance-knob"));
		}
	}
	assert(players.size() == 2 && players[0] != players[1] && knobs == 2);
	for (int tap = 0; tap < 2; ++tap)
	{
		Hear(0xb1, 71, 127);
		Hear(0xb1, 71, 0);
	}
	Pass(synth);
	for (size_t i = 0; i < players.size(); ++i)
	{
		for (int tap = 0; tap < 2; ++tap)
		{
			players[i]->UpdateChannelHandler();
			assert(players[i]->GetChannelHandler()->GetCommand() == PianoRollPlugin::PLAY);
			players[i]->ExecuteCommands();
		}
		assert(!players[i]->GetChannelHandler()->CommandsQueued());
	}
	synth.ThawAll();

	__sync_lock_test_and_set(&host.Stop, 1);
	pthread_join(thread, NULL);
	puts("Learn, ordered taps, continuous CC, reload and merge PASS");
}
