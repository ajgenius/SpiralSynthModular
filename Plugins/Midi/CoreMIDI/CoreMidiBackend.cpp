/*
 *  Copyleft (C) 2000 David Griffiths <dave@pawfal.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

// The Mac MIDI port (Michel Pollet, 2003) as a loadable backend: a
// virtual "SpiralSynth" destination and source as before, and an input
// port that follows whichever CoreMIDI source is selected, so a keyboard
// reaches the synth without a routing app in between.
#include "MidiBackend.h"
#include "NativeMidi.h"
#include <CoreMIDI/MIDIServices.h>
#include <CoreFoundation/CoreFoundation.h>
#include <sstream>

using namespace spiralcore;

#define midi_ReadSize			4096

static std::string Name(MIDIObjectRef object)
{
	CFStringRef name = NULL;
	if (MIDIObjectGetStringProperty(object, kMIDIPropertyDisplayName, &name) || !name) return "";
	char buffer[256];
	std::string result;
	if (CFStringGetCString(name, buffer, sizeof(buffer), kCFStringEncodingUTF8)) result = buffer;
	CFRelease(name);
	return result;
}

class CoreMidiTransport : public MidiTransport
{
	MIDIClientRef					mMIDIClient;
	MIDIEndpointRef					mMIDISource;
	MIDIEndpointRef					mMIDIDestination;
	MIDIPortRef						mInputPort, mOutputPort;
	MIDIEndpointRef					mConnectedSource, mConnectedDestination;
	unsigned char					m_ReadBuffer[midi_ReadSize];
	volatile int					m_ReadFillIndex;
	volatile int					m_ReadReadIndex;
	MidiByteStream					m_Bytes;
	std::string						m_Source, m_Destination;

	static void sMIDIRead(const MIDIPacketList *pktlist, void *readProcRefCon, void *srcConnRefCon);
	MIDIEndpointRef Find(const std::string &name, bool output) const;

public:
	CoreMidiTransport() :
		mMIDIClient(0), mMIDISource(0), mMIDIDestination(0), mInputPort(0), mOutputPort(0),
		mConnectedSource(0), mConnectedDestination(0), m_ReadFillIndex(0), m_ReadReadIndex(0) {}

	void Open();
	void Close();
	bool Alive() const { return mMIDIClient != 0; }
	std::vector<std::string> Ports(bool output);
	bool Connect(const std::string &source, const std::string &destination);
	bool Connected() { return Alive(); }
	bool Read(MidiPacket &packet);
	bool Write(const MidiPacket &packet);
	std::string Status() const;
};

void CoreMidiTransport::Open()
{
	m_ReadFillIndex = m_ReadReadIndex = 0;

	OSStatus err = 0;

	mMIDISource					= 0;
	mMIDIClient					= 0;
	mMIDIDestination			= 0;

	err = MIDIClientCreate(CFSTR("org.pawpal.ssm"), NULL, NULL, &mMIDIClient);
	if (err) printf("MIDIClientCreate failed returned %d\n", (int)err);

	if (!err) {
		err = MIDISourceCreate(mMIDIClient, CFSTR("SpiralSynth"), &mMIDISource);
		if (err) printf("MIDISourceCreate failed returned %d\n", (int)err);
	}

	if (!err) {
		err = MIDIDestinationCreate(mMIDIClient, CFSTR("SpiralSynth"), sMIDIRead, this, &mMIDIDestination);
		MIDIObjectSetIntegerProperty(mMIDIDestination, kMIDIPropertyUniqueID, 'SSmP');
	}

	if (!err) {
		err = MIDIInputPortCreate(mMIDIClient, CFSTR("SpiralSynth In"), sMIDIRead, this, &mInputPort);
		if (err) printf("MIDIInputPortCreate failed returned %d\n", (int)err);
	}

	if (!err) {
		err = MIDIOutputPortCreate(mMIDIClient, CFSTR("SpiralSynth Out"), &mOutputPort);
		if (err) printf("MIDIOutputPortCreate failed returned %d\n", (int)err);
	}
}


void CoreMidiTransport::Close()
{
	if (mConnectedSource && mInputPort)
		MIDIPortDisconnectSource(mInputPort, mConnectedSource);
	mConnectedSource = 0;
	mConnectedDestination = 0;
	if (mInputPort)
		MIDIPortDispose(mInputPort);
	if (mOutputPort)
		MIDIPortDispose(mOutputPort);
	mInputPort = mOutputPort = 0;
	if (mMIDIDestination)
		MIDIEndpointDispose(mMIDIDestination);
	if (mMIDISource)
		MIDIEndpointDispose(mMIDISource);
	mMIDISource = mMIDIDestination = 0;
	if (mMIDIClient)
		MIDIClientDispose(mMIDIClient);
	mMIDIClient = 0;
}

// The virtual source is the first output: what the synth sends is
// always there for other apps, with or without a destination of its own.
std::vector<std::string> CoreMidiTransport::Ports(bool output)
{
	std::vector<std::string> names;
	if (output && mMIDISource) names.push_back("SpiralSynth");
	const ItemCount count = output ? MIDIGetNumberOfDestinations() : MIDIGetNumberOfSources();
	for (ItemCount n = 0; n < count; ++n)
	{
		MIDIEndpointRef endpoint = output ? MIDIGetDestination(n) : MIDIGetSource(n);
		// Our own virtual endpoints are not ports to connect to.
		if (endpoint == mMIDISource || endpoint == mMIDIDestination) continue;
		names.push_back(Name(endpoint));
	}
	return names;
}

MIDIEndpointRef CoreMidiTransport::Find(const std::string &name, bool output) const
{
	if (name.empty()) return 0;
	const ItemCount count = output ? MIDIGetNumberOfDestinations() : MIDIGetNumberOfSources();
	for (ItemCount n = 0; n < count; ++n)
	{
		MIDIEndpointRef endpoint = output ? MIDIGetDestination(n) : MIDIGetSource(n);
		// Never our own endpoints: that would be a loop.
		if (endpoint == mMIDISource || endpoint == mMIDIDestination) continue;
		if (Name(endpoint) == name) return endpoint;
	}
	return 0;
}

// The virtual destination always listens; a named source is connected
// to the input port beside it, a named destination gets the output
// in addition to the virtual source.
bool CoreMidiTransport::Connect(const std::string &source, const std::string &destination)
{
	if (!Alive()) return false;
	if (mConnectedSource) MIDIPortDisconnectSource(mInputPort, mConnectedSource);
	mConnectedSource = Find(source, false);
	if (mConnectedSource) MIDIPortConnectSource(mInputPort, mConnectedSource, NULL);
	mConnectedDestination = Find(destination, true);
	m_Source = mConnectedSource ? source : "";
	m_Destination = mConnectedDestination || destination == "SpiralSynth" ? destination : "";
	return true;
}

bool CoreMidiTransport::Read(MidiPacket &packet)
{
	while (m_ReadReadIndex != m_ReadFillIndex)
	{
		int r = m_ReadReadIndex;
		const unsigned char byte = m_ReadBuffer[r];
		r++;
		m_ReadReadIndex = r % midi_ReadSize;
		if (m_Bytes.Push(byte, packet)) return true;
	}
	return false;
}

bool CoreMidiTransport::Write(const MidiPacket &packet)
{
	if (!Alive()) return false;
	unsigned char bytes[3] = { packet.Status, packet.Data1, packet.Data2 };
	Byte buffer[64];
	MIDIPacketList *list = (MIDIPacketList *)buffer;
	MIDIPacket *current = MIDIPacketListInit(list);
	current = MIDIPacketListAdd(list, sizeof(buffer), current, 0, MidiSize(packet.Status), bytes);
	if (!current) return false;
	if (mMIDISource) MIDIReceived(mMIDISource, list);
	if (mConnectedDestination && mOutputPort) MIDISend(mOutputPort, mConnectedDestination, list);
	return true;
}

std::string CoreMidiTransport::Status() const
{
	if (!Alive()) return "CoreMIDI: no client";
	std::ostringstream status;
	status << "CoreMIDI: SpiralSynth";
	if (!m_Source.empty()) status << " <- " << m_Source;
	if (!m_Destination.empty()) status << " -> " << m_Destination;
	return status.str();
}

void CoreMidiTransport::sMIDIRead(const MIDIPacketList *pktlist, void *readProcRefCon, void *srcConnRefCon)
{
	CoreMidiTransport & t = *((CoreMidiTransport*)readProcRefCon);

	const MIDIPacket *packet = &pktlist->packet[0];
	for (int i = 0; i < (int)pktlist->numPackets; i++) {
		const MIDIPacket & p = *packet;

		for (int b = 0; b < p.length; b++) {
			int d = t.m_ReadFillIndex;
			t.m_ReadBuffer[d] = p.data[b];
			d++;
			t.m_ReadFillIndex = d % midi_ReadSize;
		}
		packet = MIDIPacketNext(packet);
	}
}

// * Backend module entry

static void *Create(void *) { return static_cast<MidiBackend *>(new NativeMidiBackend(new CoreMidiTransport)); }
static void Destroy(void *backend) { delete static_cast<MidiBackend *>(backend); }

extern "C" const BackendDescriptor *SpiralPlugin_GetMidiBackend()
{
	static const BackendDescriptor d = {SPIRAL_MIDI_PLUGIN_ABI, "midi", "coremidi", Create, Destroy};
	return &d;
}
