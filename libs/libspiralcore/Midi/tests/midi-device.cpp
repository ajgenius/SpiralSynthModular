// SPDX-License-Identifier: GPL-2.0-or-later
#include "Midi.h"
#include <cassert>
#include <cstdio>
#include <deque>

using namespace Spiral::Core;

struct Script : public MidiBackend
{
	std::deque<MidiPacket> in;
	std::vector<MidiPacket> out;
	std::vector<std::string> Ports(bool) { return std::vector<std::string>(1, "script"); }
	void Select(const std::string &, const std::string &) {}
	bool Poll(MidiPacket &p) { if (in.empty()) return false; p = in.front(); in.pop_front(); return true; }
	bool Send(const MidiPacket &p) { out.push_back(p); return true; }
	std::string Status() { return "script"; }
};
static Script *script = NULL;
static void *Create(void *) { return static_cast<MidiBackend *>(script = new Script); }
static void Destroy(void *p) { delete static_cast<MidiBackend *>(p); if (p == script) script = NULL; }

int main()
{
	const BackendDescriptor d = { SPIRAL_MIDI_PLUGIN_ABI, "midi", "script", Create, Destroy };
	MidiBackendRegistry::Get()->Register(&d);

	MidiDevice::SetBackendName("script");
	MidiDevice::Init("midi-device-test", MidiDevice::READ);
	MidiDevice *device = MidiDevice::Get();
	assert(device && script && device->GetBackendName() == "script");
	printf("%s\n", device->GetStatus().c_str());

	script->in.push_back(MidiPacket(0x90, 60, 100));
	script->in.push_back(MidiPacket(0x91, 62, 90));
	script->in.push_back(MidiPacket(0x90, 60, 0));
	script->in.push_back(MidiPacket(0xb0, 7, 127));
	script->in.push_back(MidiPacket(0xe0, 0, 64));
	for (int n = 0; n < 6; ++n) script->in.push_back(MidiPacket(0xf8));
	script->in.push_back(MidiPacket(0xff));

	assert(device->GetClock() == 1.0f);
	MidiEvent e = device->GetEvent(0);
	assert(e.GetType() == MidiEvent::ON && e.GetNote() == 60 && e.GetVolume() == 100);
	e = device->GetEvent(0);
	assert(e.GetType() == MidiEvent::OFF && e.GetNote() == 60);
	e = device->GetEvent(0);
	assert(e.GetType() == MidiEvent::PARAMETER && e.GetNote() == 7 && e.GetVolume() == 127);
	e = device->GetEvent(0);
	assert(e.GetType() == MidiEvent::PITCHBEND && e.GetVolume() == 64);
	assert(device->GetEvent(0).GetType() == MidiEvent::NONE);
	e = device->GetEvent(1);
	assert(e.GetType() == MidiEvent::ON && e.GetNote() == 62);
	assert(device->GetEvent(1).GetType() == MidiEvent::NONE);
	assert(device->GetClock() == -1.0f);

	device->SendEvent(3, MidiEvent(MidiEvent::ON, 64, 99));
	device->SendEvent(3, MidiEvent(MidiEvent::OFF, 64, 0));
	assert(script->out.size() == 2);
	assert(script->out[0].Status == 0x93 && script->out[0].Data1 == 64 && script->out[0].Data2 == 99);
	assert(script->out[1].Status == 0x83 && script->out[1].Data1 == 64);

	MidiDevice::SetBackendName("dummy");
	assert(device->GetBackendName() == "dummy" && !script);
	assert(device->GetEvent(0).GetType() == MidiEvent::NONE);
	MidiDevice::SetBackendName("script");
	assert(script && device->GetBackendName() == "script");

	MidiDevice::SetBackendName("nonesuch");
	assert(device->GetBackendName() == "script");

	MidiDevice::PackUpAndGoHome();
	assert(!script);
	MidiBackendRegistry::PackUpAndGoHome();
	puts("MidiDevice on a registry backend PASS");
	return 0;
}
