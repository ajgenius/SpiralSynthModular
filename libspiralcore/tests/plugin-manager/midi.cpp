// SPDX-License-Identifier: GPL-2.0-or-later
#include "MidiBackend.h"
using namespace spiralcore;
class ProbeMidi : public MidiBackend
{
public:
	std::vector<std::string> Ports(bool) { return std::vector<std::string>(1, "probe"); }
	void Select(const std::string &, const std::string &) {}
	bool Poll(MidiPacket &packet) { packet = MidiPacket(0x90, 60, 100); return true; }
	bool Send(const MidiPacket &) { return true; }
	std::string Status() { return "probe"; }
};
static void *Create(void *) { return static_cast<MidiBackend *>(new ProbeMidi); }
static void Destroy(void *backend) { delete static_cast<MidiBackend *>(backend); }
extern "C" const BackendDescriptor *SpiralPlugin_GetMidiBackend()
{
	static const BackendDescriptor descriptor = { SPIRAL_MIDI_PLUGIN_ABI, "midi", "probe", Create, Destroy };
	return &descriptor;
}
