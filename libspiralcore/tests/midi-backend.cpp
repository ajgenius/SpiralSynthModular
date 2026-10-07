// SPDX-License-Identifier: GPL-2.0-or-later
// The midi registry hands out backends by name: the dummy is built in
// and last, a descriptor registered at runtime creates and destroys
// through its own functions, a wrong ABI, wrong kind or duplicate name
// is refused, a module arrives through the manager and leaves with it.
#include "MidiBackend.h"
#include <cassert>
#include <cstdio>
#include <vector>
#include <cstdlib>

using namespace spiralcore;

static unsigned created = 0, destroyed = 0;
static void *Create(void *) { ++created; return static_cast<MidiBackend *>(new DummyMidiBackend); }
static void Destroy(void *backend) { ++destroyed; delete static_cast<MidiBackend *>(backend); }

int main()
{
	MidiBackendRegistry *registry = MidiBackendRegistry::Get();
	std::vector<std::string> names = registry->Names();
	assert(names.size() == 1 && names[0] == "dummy");

	const BackendDescriptor null = { SPIRAL_MIDI_PLUGIN_ABI, "midi", "null", Create, Destroy };
	registry->Register(&null);
	assert(registry->Find("null") == &null);
	// The dummy stays last however late the others register.
	names = registry->Names();
	assert(names.size() == 2 && names[0] == "null" && names[1] == "dummy");

	const BackendDescriptor wrong = { SPIRAL_MIDI_PLUGIN_ABI + 1, "midi", "wrong", Create, Destroy };
	registry->Register(&wrong);
	assert(!registry->Find("wrong"));

	const BackendDescriptor audio = { SPIRAL_MIDI_PLUGIN_ABI, "audio", "audio", Create, Destroy };
	registry->Register(&audio);
	assert(!registry->Find("audio"));

	const BackendDescriptor duplicate = { SPIRAL_MIDI_PLUGIN_ABI, "midi", "null", Create, Destroy };
	registry->Register(&duplicate);
	assert(registry->Find("null") == &null);

	MidiBackend *backend = registry->Create("null");
	assert(backend && created == 1);
	registry->Destroy("null", backend);
	assert(destroyed == 1);
	assert(!registry->Create("nonesuch"));

	// The dummy: no ports, nothing to poll, sends are accepted.
	MidiBackend *dummy = registry->Create("dummy");
	assert(dummy && dummy->Ports(false).empty() && dummy->Ports(true).empty());
	MidiPacket packet;
	assert(!dummy->Poll(packet) && dummy->Send(MidiPacket(0x90, 60, 100)));
	printf("dummy: %s\n", dummy->Status().c_str());
	registry->Destroy("dummy", dummy);

	assert(registry->LoadModules("/nonexistent/path") == 0);
	const char *root = std::getenv("PLUGIN_LOADER_ROOT");
	assert(root && registry->LoadModules(root) == 1);
	MidiBackend *module = registry->Create("probe");
	assert(module && module->Poll(packet) && packet.Status == 0x90 && packet.Data1 == 60);
	assert(module->Ports(false).size() == 1);
	// A live backend keeps its module loaded.
	assert(!PluginLoader::Get()->UnloadAll());
	MidiBackendRegistry::PackUpAndGoHome();
	assert(MidiBackendRegistry::Get() == registry);
	registry->Destroy("probe", module);
	assert(PluginLoader::Get()->UnloadAll());
	assert(!registry->Find("probe") && registry->Find("dummy"));
	assert(registry->LoadModules(root) == 1);
	MidiBackendRegistry::PackUpAndGoHome();
	assert(!registry->Find("probe"));
	PluginLoader::PackUpAndGoHome();
	printf("PASS\n");
	return 0;
}
