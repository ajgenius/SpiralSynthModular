// SPDX-License-Identifier: GPL-2.0-or-later
// The unchanged MidiPlugin over a backend from the registry. The midi
// modules are loaded from the plugin root like the host does, the
// preference names the backend, and the plugin's Note and Trigger
// outputs follow the packets the backend delivers.
//   midi-plugin-test <pluginroot>            dummy: loads, runs, silent
//   midi-plugin-test <pluginroot> coremidi   live: a CoreMIDI client
//                                            plays into "SpiralSynth"
#include "SpiralSynthModular.h"
#include "DeviceClassRegistry.h"
#include "MidiBackend.h"
#include "Midi.h"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <unistd.h>
#ifdef __APPLE__
#include <CoreMIDI/MIDIServices.h>
#endif

using namespace spiralcore;

static const int MIDI_PLUGIN_ID = 0x0002;

static float Out(SpiralPlugin *plugin, unsigned port)
{
	Sample *s = NULL;
	assert(plugin->GetOutput(port, &s) && s);
	return (*s)[0];
}

#ifdef __APPLE__
static MIDIEndpointRef Destination(const char *name)
{
	for (ItemCount n = 0; n < MIDIGetNumberOfDestinations(); ++n)
	{
		MIDIEndpointRef endpoint = MIDIGetDestination(n);
		CFStringRef cf = NULL;
		char buffer[256] = "";
		if (!MIDIObjectGetStringProperty(endpoint, kMIDIPropertyDisplayName, &cf) && cf)
		{
			CFStringGetCString(cf, buffer, sizeof(buffer), kCFStringEncodingUTF8);
			CFRelease(cf);
		}
		if (!strcmp(buffer, name)) return endpoint;
	}
	return 0;
}
static void Play(const unsigned char *bytes, unsigned size)
{
	MIDIEndpointRef synth = 0;
	for (int n = 0; n < 200 && !(synth = Destination("SpiralSynth")); ++n) usleep(10000);
	assert(synth);
	MIDIClientRef client = 0;
	MIDIPortRef out = 0;
	assert(!MIDIClientCreate(CFSTR("midi-plugin-test"), NULL, NULL, &client));
	assert(!MIDIOutputPortCreate(client, CFSTR("out"), &out));
	Byte buffer[128];
	MIDIPacketList *list = (MIDIPacketList *)buffer;
	MIDIPacket *current = MIDIPacketListInit(list);
	current = MIDIPacketListAdd(list, sizeof(buffer), current, 0, size, bytes);
	assert(current && !MIDISend(out, synth, list));
	MIDIClientDispose(client);
}
#endif

int main(int argc, char **argv)
{
	if (argc < 2) return 77;
	const std::string backend = argc > 2 ? argv[2] : "dummy";
#ifndef __APPLE__
	if (backend == "coremidi") return 77;
#endif
	alarm(30);
	std::string root = argv[1];
	if (root[root.size()-1] != '/') root += '/';

	assert(MidiBackendRegistry::Get()->LoadModules(root) >= (backend == "dummy" ? 0u : 1u));
	assert(MidiBackendRegistry::Get()->Find(backend));
	// What the host does with the preference, before any plugin exists.
	MidiDevice::SetBackendName(backend);

	DeviceClassRegistry::Get()->LoadModules(root);
	const DeviceClass *midi = DeviceClassRegistry::Get()->Find(MIDI_PLUGIN_ID);
	assert(midi && midi->Name == "Midi");
	HostInfo info = HostInfo();
	info.BUFSIZE = 64;
	info.SAMPLERATE = 44100;
	info.MIDIFILE = "";
	SpiralPlugin *plugin = midi->Create();
	plugin->Initialise(&info);
	assert(MidiDevice::Get() && MidiDevice::Get()->GetBackendName() == backend);
	// The backend's worker opens its transport on its own time; only it
	// touches the MIDI API until then. Its first Poll after connecting
	// is a reset, which this Execute absorbs.
	for (int n = 0; n < 300 && MidiDevice::Get()->GetStatus() == "Starting MIDI"; ++n) usleep(10000);
	printf("midi plugin: %s\n", MidiDevice::Get()->GetStatus().c_str());
	plugin->Execute();
	assert(Out(plugin, 1) == 0.0f);

	if (backend == "coremidi")
	{
#ifdef __APPLE__
		const unsigned char on[] = { 0x90, 69, 127 };
		Play(on, sizeof(on));
		const float silent = Out(plugin, 0);
		float trigger = 0, note = silent;
		for (int n = 0; n < 200 && trigger == 0.0f; ++n)
		{
			usleep(10000);
			plugin->Execute();
			trigger = Out(plugin, 1);
			note = Out(plugin, 0);
		}
		printf("midi plugin: note on 69 -> trigger %.3f note cv %.4f\n", trigger, note);
		assert(trigger == 1.0f && note != silent);
		const unsigned char off[] = { 0x80, 69, 0 };
		Play(off, sizeof(off));
		for (int n = 0; n < 200 && trigger != 0.0f; ++n)
		{
			usleep(10000);
			plugin->Execute();
			trigger = Out(plugin, 1);
		}
		printf("midi plugin: note off -> trigger %.3f\n", trigger);
		assert(trigger == 0.0f);
#endif
	}

	delete plugin;
	assert(!MidiDevice::Get());
	DeviceClassRegistry::PackUpAndGoHome();
	MidiBackendRegistry::PackUpAndGoHome();
	printf("midi plugin on the %s backend PASS\n", backend.c_str());
	return 0;
}
