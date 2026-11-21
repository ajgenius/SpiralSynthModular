// SPDX-License-Identifier: GPL-2.0-or-later
#include "SpiralSynthModular.h"
#include "SpiralInfo.h"
#include "Midi.h"
#include "EventBuffer.h"
#include <cmath>
#include "FltkView.h"
#include "PatchFile.h"
#include "Plugins/Devices/PianoRollPlugin/PianoRollPlugin.h"
#include "Plugins/Devices/MidiPlugin/MidiPlugin.h"
#include "PanelControl.h"
#include <FL/Fl_Valuator.H>
#include <FL/Fl_Choice.H>
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
	std::vector<std::string> Ports(bool)
	{
		std::vector<std::string> ports(1, "surface");
		ports.push_back("second surface");
		return ports;
	}
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

static void CheckSourceMenu()
{
	const std::string original = MidiDevice::GetDeviceName();
	Fl_MidiBindings page(0, 0, 320, 240);
	Fl_Choice *source = NULL;
	for (int i = 0; i < page.children(); ++i)
		if (Fl_Choice *choice = dynamic_cast<Fl_Choice *>(page.child(i)))
			source = choice;
	assert(source);

	MidiDevice::SetDeviceName("surface");
	page.Refresh();
	assert(std::string(source->text()) == "surface");
	MidiDevice::SetDeviceName("second surface");
	page.Refresh();
	assert(std::string(source->text()) == "second surface");
	MidiDevice::SetDeviceName("offline A");
	page.Refresh();
	MidiDevice::SetDeviceName("offline B");
	page.Refresh();
	assert(source->value() == 0 && std::string(source->tooltip()).find("offline B") != std::string::npos);
	MidiDevice::SetDeviceName(original);
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

// Keep references to the actual filter instances so the test can read the CV
// buffers connected by the host, including the two hidden voices.
static std::vector<SpiralPlugin *> filters;
static SpiralPlugin *CreateFilter()
{
	SpiralPlugin *filter = DeviceClassRegistry::Get()->Find(11)->CreateInstance();
	filters.push_back(filter);
	return filter;
}

// Called with the worker frozen. Wait until a device has seen the new
// format through the host's settings path, then inspect the graph at rest.
static void SetFormat(SynthModular &synth, SpiralPlugin &midi, int rate, int frames)
{
	SpiralInfo::FOLLOW_AUDIO_FORMAT = false;
	SpiralInfo::SAMPLERATE = rate;
	SpiralInfo::BUFSIZE = frames;
	synth.UpdateHostInfo();

	for (unsigned tries = 0; tries < 100; ++tries)
	{
		synth.ThawAll();
		usleep(2000);
		synth.FreezeAll();

		if (midi.GetHostInfo()->SAMPLERATE == rate && midi.GetHostInfo()->BUFSIZE == frames)
			return;
	}
	assert(false);
}

static void Chords(SynthModular &synth)
{
	synth.ClearUp();
	DeviceClass probe = *DeviceClassRegistry::Get()->Find(11);
	probe.ID = 10011;
	probe.Name = "Chord test filter";
	probe.Create = CreateFilter;
	probe.Build = NULL;
	probe.Panel.clear();
	probe.PanelGenerated = false;
	assert(DeviceClassRegistry::Get()->Register(probe));
	synth.AddDevice(2, 50, 50);
	synth.AddDevice(probe.ID, 150, 50);
	synth.FreezeAll();
	SpiralPlugin *source = synth.cb_FindDevice(0, &synth);
	char label[256] = "Expression";
	source->GetChannelHandler()->Set("CC", 11);
	source->GetChannelHandler()->SetData("Name", label);
	source->GetChannelHandler()->SetCommand(MidiPlugin::ADDCONTROL);
	source->UpdateChannelHandler();
	source->ExecuteCommands();
	synth.ThawAll();
	PatchDescription patch;
	Describe(patch, synth, "");
	patch.Voices = 3;
	for (int port = 0; port < 3; ++port)
	{
		WireDescription &wire = patch.AddWire();
		wire.FromDevice = "0";
		wire.ToDevice = "1";
		wire.ToPort = std::string(1, '0' + port);
		wire.FromPort = port == 2 ? "6" : wire.ToPort;
	}
	synth.ClearUp();
	filters.clear();
	synth.Apply(patch, "");
	synth.FreezeAll();
	assert(filters.size() == 3);
	SpiralPlugin *midi = synth.cb_FindDevice(0, &synth);
	assert(midi);
	const unsigned keys[] = {60, 64, 67};
	const unsigned velocities[] = {127, 80, 40};
	for (unsigned i = 0; i < 3; ++i)
		Hear(0x90, keys[i], velocities[i]);
	midi->Execute();
	const EventBuffer *events = midi->GetEventOutput(midi->GetPluginInfo().NumOutputs - 1);
	assert(events && events->Count() == 3);
	for (unsigned i = 0; i < 3; ++i)
	{
		const Sample *pitch = filters[i]->GetInput(0), *gate = filters[i]->GetInput(1);
		assert(pitch && gate && events->At(i).NoteNum == int(keys[i]));
		assert(std::fabs((*pitch)[1] - (events->At(i).Frequency / MAX_FREQ * 2 - 1)) < 1e-6f);
		assert((*gate)[1] == velocities[i] / 127.f);
		assert(filters[i]->GetInput(2) && filters[i]->GetInput(2) == filters[0]->GetInput(2));
	}
	// Rebuilding the same-width patch while the notes are held recreates
	// the downstream filters. Their new wires must still carry the chord.
	synth.ThawAll();
	filters.resize(1);
	synth.RebuildVoices();
	synth.FreezeAll();
	midi->Execute();
	assert(filters.size() == 3);
	for (unsigned i = 0; i < 3; ++i)
		assert((*filters[i]->GetInput(1))[1] == velocities[i] / 127.f);
	Hear(0xb0, 11, 96);
	midi->Execute();
	Sample *clock = NULL;
	assert(midi->GetOutput(5, &clock) && (*clock)[1] == MidiDevice::Get()->GetClock());
	for (unsigned i = 0; i < 3; ++i)
		assert((*filters[i]->GetInput(2))[1] == 96 / 127.f);
	Hear(0x80, 64, 0);
	midi->Execute();
	assert((*filters[0]->GetInput(1))[1] == 1 && (*filters[1]->GetInput(1))[1] == 0);
	assert((*filters[2]->GetInput(1))[1] == 40 / 127.f);

	Hear(0x90, 60, 70); // A second C occupies the free voice.
	midi->Execute();
	assert((*filters[1]->GetInput(1))[1] == 70 / 127.f);
	Hear(0x80, 60, 0);
	midi->Execute();
	assert((*filters[0]->GetInput(1))[1] == 0 && (*filters[1]->GetInput(1))[1] == 70 / 127.f);
	Hear(0xb0, 64, 127);
	Hear(0xb0, 123, 0);
	midi->Execute();
	assert((*filters[1]->GetInput(1))[1] == 70 / 127.f && (*filters[2]->GetInput(1))[1] > 0);
	float pitches[3];
	for (unsigned v = 0; v < 3; ++v)
		pitches[v] = (*filters[v]->GetInput(0))[1];
	const int rates[] = {48000, 48000, 44100};
	const int sizes[] = {midi->GetHostInfo()->BUFSIZE, 128, 512};
	for (unsigned change = 0; change < 3; ++change)
	{
		SetFormat(synth, *midi, rates[change], sizes[change]);
		midi->Execute();
		assert(events->Count() == 0);
		const float expected[] = {0, 70 / 127.f, 40 / 127.f};
		for (unsigned v = 0; v < 3; ++v)
		{
			const Sample *gate = filters[v]->GetInput(1);
			assert(gate->GetLength() == sizes[change]);
			for (int frame = 0; frame < gate->GetLength(); ++frame)
			{
				assert((*gate)[frame] == expected[v]);
				assert((*filters[v]->GetInput(0))[frame] == pitches[v]);
			}
		}
	}

	Hear(0xb0, 64, 0);
	midi->Execute();
	for (unsigned i = 0; i < 3; ++i)
		assert((*filters[i]->GetInput(1))[1] == 0);

	Hear(0x90, 72, 100);
	midi->Execute();
	Device *device = dynamic_cast<Device *>(midi);
	assert(device);
	SignedProperty channel(DefaultLinearFlags, 1, 0, 15, 1, 1);
	device->SynchronizeProperty("DEVICENUM", &channel);
	midi->Execute();
	assert(events->Count() == 1 && events->At(0).Type == Event::Reset);
	for (unsigned i = 0; i < 3; ++i)
		assert((*filters[i]->GetInput(1))[1] == 0);
	synth.ThawAll();

	synth.ClearUp();
	filters.clear();
	patch.Voices = 1;
	synth.Apply(patch, "");
	synth.FreezeAll();
	assert(filters.size() == 1);
	Hear(0x90, 60, 127);
	Hear(0x90, 67, 40);
	midi = synth.cb_FindDevice(0, &synth);
	midi->Execute();
	assert((*filters[0]->GetInput(1))[0] == 0 && (*filters[0]->GetInput(1))[1] == 40 / 127.f);
	SetFormat(synth, *midi, 48000, 64);
	midi->Execute();
	assert((*filters[0]->GetInput(1))[0] == 40 / 127.f && (*filters[0]->GetInput(1))[63] == 40 / 127.f);
	Hear(0x80, 67, 0);
	midi->Execute();
	assert((*filters[0]->GetInput(1))[0] == 0);
	Hear(0x90, 67, 100);
	midi->Execute();
	midi->Reset();
	midi->Execute();
	assert((*filters[0]->GetInput(1))[1] == 0);
	synth.ThawAll();
	puts("MIDI pitch and trigger on real patch wires: three voices, overlapping keys, sustain and mono PASS");
}

// Compare the last sample in the old format with the first in the new one
// while a constant-amplitude square wave passes through an unfinished attack.
static void HeldEnvelope(unsigned id)
{
	HostInfo host = HostInfo();
	host.SAMPLERATE = 48000;
	host.BUFSIZE = 64;
	const DeviceClass *kind = DeviceClassRegistry::Get()->Find(id);
	assert(kind);
	SpiralPlugin *envelope = kind->CreateInstance();
	envelope->Initialise(&host);
	envelope->Reset();
	if (id == 5)
	{
		FloatProperty attack(DefaultLinearFlags, .2f, 0, 1, .001f, .001f);
		dynamic_cast<Device *>(envelope)->SynchronizeProperty("Attack", &attack);
	}

	Sample gate(128), tone(128);
	gate.Set(1);
	for (int frame = 0; frame < 128; ++frame)
		tone.Set(frame, frame % 32 < 16 ? .5f : -.5f);
	assert(envelope->SetInput(0, &gate) && envelope->SetInput(1, &tone));
	for (unsigned block = 0; block < 48; ++block)
		envelope->Execute();
	Sample *output = NULL;
	assert(envelope->GetOutput(1, &output));
	const float before = std::fabs((*output)[63]);
	assert(before > .01f);

	host.SAMPLERATE = 44100;
	host.BUFSIZE = 128;
	LiveAudioFormat::Apply(*envelope);
	envelope->Execute();
	assert(output->GetLength() == 128 && std::fabs(std::fabs((*output)[0]) - before) < .001f);
	delete envelope;
}

static void HeldEventInput()
{
	HostInfo host = HostInfo();
	host.BUFSIZE = 64;
	host.SAMPLERATE = 48000;
	SpiralPlugin *decoder = DeviceClassRegistry::Get()->Find(289)->CreateInstance();
	decoder->Initialise(&host);
	EventBuffer events;
	events.Clear(64);
	Event note;
	note.Type = Event::NoteOn;
	note.ID = 37;
	note.Volume = .75f;
	assert(events.Append(0, note));
	decoder->SetEventInput(0, &events);
	decoder->Execute();

	host.BUFSIZE = 128;
	host.SAMPLERATE = 44100;
	LiveAudioFormat::Apply(*decoder);
	events.Clear(128);
	decoder->Execute();
	Sample *gate = NULL;
	assert(decoder->GetOutput(2, &gate) && gate->GetLength() == 128);
	assert(decoder->GetEventOutput(0)->Count() == 0 && (*gate)[0] == .75f && (*gate)[127] == .75f);
	note.Type = Event::NoteOff;
	assert(events.Append(7, note));
	decoder->Execute();
	assert((*gate)[6] == .75f && (*gate)[7] == 0);
	delete decoder;
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

	HeldEventInput();
	HeldEnvelope(5);
	HeldEnvelope(33);
	CheckSourceMenu();
	Chords(synth);
	Pass(synth);
	synth.ClearUp();
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

	// Disconnect while Play is held. The first press after the transport
	// reset is a new strike even when both arrive within one GUI poll.
	Hear(0xb1, 71, 127);
	Hear(0xff, 0, 0);
	Hear(0xb1, 71, 127);
	Hear(0xb1, 71, 0);
	Pass(synth);
	assert(taps.Values.size() == 5);

	// A lost release during a full GUI queue also rearms the next tap.
	Hear(0xb1, 71, 127);
	Pass(synth);
	synth.FreezeAll();
	for (int i = 0; i < 1023; ++i)
		Hear(0xb1, 79, 0);
	Hear(0xb1, 71, 0);
	Pass(synth);
	Hear(0xb1, 71, 127);
	Hear(0xb1, 71, 0);
	Pass(synth);
	synth.ThawAll();
	assert(taps.Values.size() == 7);

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
