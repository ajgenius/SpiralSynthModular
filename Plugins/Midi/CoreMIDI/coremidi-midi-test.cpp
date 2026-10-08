// SPDX-License-Identifier: GPL-2.0-or-later
// Live: a CoreMIDI client of our own sends to the backend's virtual
// "SpiralSynth" destination and the packets come out of Poll, in order,
// with running status and a clock tick in between; a send from the
// backend appears on its virtual source. Skips without --live.
#include "MidiBackend.h"
#include <CoreMIDI/MIDIServices.h>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <unistd.h>

extern "C" const spiralcore::BackendDescriptor *SpiralPlugin_GetMidiBackend();
using namespace spiralcore;

static MIDIEndpointRef Named(const char *name, bool destination)
{
	const ItemCount count = destination ? MIDIGetNumberOfDestinations() : MIDIGetNumberOfSources();
	for (ItemCount n = 0; n < count; ++n)
	{
		MIDIEndpointRef endpoint = destination ? MIDIGetDestination(n) : MIDIGetSource(n);
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

static unsigned received = 0;
static unsigned char last[3];
static void Receive(const MIDIPacketList *list, void *, void *)
{
	const MIDIPacket *packet = &list->packet[0];
	for (unsigned n = 0; n < list->numPackets; ++n)
	{
		memcpy(last, packet->data, packet->length < 3 ? packet->length : 3);
		++received;
		packet = MIDIPacketNext(packet);
	}
}

int main(int argc, char **argv)
{
	if (argc != 2 || std::string(argv[1]) != "--live") return 77;

	const BackendDescriptor *d = SpiralPlugin_GetMidiBackend();
	assert(d && std::string(d->Kind) == "midi" && std::string(d->Name) == "coremidi");
	MidiBackend *backend = static_cast<MidiBackend *>(d->Create(NULL));
	assert(backend);
	// No source named: the virtual destination listens anyway. The
	// virtual source is the output.
	backend->Select("", "SpiralSynth");

	MIDIEndpointRef synth = 0, source = 0;
	for (int n = 0; n < 200 && !(synth && source); ++n)
	{
		usleep(10000);
		synth = Named("SpiralSynth", true);
		source = Named("SpiralSynth", false);
	}
	assert(synth && source);
	printf("%s\n", backend->Status().c_str());
	std::vector<std::string> outs = backend->Ports(true);
	assert(!outs.empty() && outs[0] == "SpiralSynth");

	MIDIClientRef client = 0;
	MIDIPortRef out = 0, in = 0;
	assert(!MIDIClientCreate(CFSTR("coremidi-midi-test"), NULL, NULL, &client));
	assert(!MIDIOutputPortCreate(client, CFSTR("out"), &out));
	assert(!MIDIInputPortCreate(client, CFSTR("in"), Receive, NULL, &in));
	assert(!MIDIPortConnectSource(in, source, NULL));

	// Note on, clock, running-status note on, note off, as a keyboard would.
	const unsigned char bytes[] = { 0x90, 60, 100, 0xf8, 64, 90, 0x80, 60, 0 };
	Byte buffer[128];
	MIDIPacketList *list = (MIDIPacketList *)buffer;
	MIDIPacket *current = MIDIPacketListInit(list);
	current = MIDIPacketListAdd(list, sizeof(buffer), current, 0, sizeof(bytes), bytes);
	assert(current && !MIDISend(out, synth, list));

	MidiPacket p;
	std::vector<MidiPacket> got;
	for (int n = 0; n < 200 && got.size() < 4; ++n)
	{
		while (backend->Poll(p))
			if (p.Status != 0xff) got.push_back(p);
		usleep(10000);
	}
	for (size_t n = 0; n < got.size(); ++n) printf("poll %zu: %02x %d %d\n", n, got[n].Status, got[n].Data1, got[n].Data2);
	assert(got.size() == 4);
	assert(got[0].Status == 0x90 && got[0].Data1 == 60 && got[0].Data2 == 100);
	assert(got[1].Status == 0xf8);
	assert(got[2].Status == 0x90 && got[2].Data1 == 64 && got[2].Data2 == 90);
	assert(got[3].Status == 0x80 && got[3].Data1 == 60);

	// The other way: what the backend sends shows up on its virtual source.
	printf("%s\n", backend->Status().c_str());
	bool sent = false;
	int tries = 0;
	for (; tries < 100 && !(sent = backend->Send(MidiPacket(0x91, 67, 77))); ++tries) usleep(10000);
	printf("send %s after %d retries\n", sent ? "ok" : "refused", tries);
	assert(sent);
	for (int n = 0; n < 200 && !received; ++n) usleep(10000);
	assert(received == 1 && last[0] == 0x91 && last[1] == 67 && last[2] == 77);

	MIDIPortDisconnectSource(in, source);
	MIDIClientDispose(client);
	d->Destroy(backend);
	puts("CoreMIDI backend receives on its destination and sends on its source PASS");
	return 0;
}
