// SPDX-License-Identifier: GPL-2.0-or-later
#include <ladspa.h>
#include <cstdlib>

static LADSPA_Handle Create(const LADSPA_Descriptor *, unsigned long) { return malloc(1); }
static void Connect(LADSPA_Handle, unsigned long, LADSPA_Data *) {}
static void Run(LADSPA_Handle, unsigned long) {}
static void Destroy(LADSPA_Handle handle) { free(handle); }

extern "C" const LADSPA_Descriptor *ladspa_descriptor(unsigned long index)
{
	static LADSPA_PortDescriptor ports[8];
	static const char *names[8];
	static LADSPA_PortRangeHint hints[8];
	static LADSPA_Descriptor descriptor = {};
	for (unsigned i = 0; i < 8; ++i)
	{
		ports[i] = LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		names[i] = "Control";
		hints[i].HintDescriptor = LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_1;
		hints[i].LowerBound = 0;
		hints[i].UpperBound = 2;
	}

	descriptor.UniqueID = 7654321;
	descriptor.Label = "port_count_fixture";
	descriptor.Name = "Port count fixture";
	descriptor.Maker = "SSM tests";
	descriptor.Copyright = "GPL-2.0-or-later";
	descriptor.PortCount = 8;
	descriptor.PortDescriptors = ports;
	descriptor.PortNames = names;
	descriptor.PortRangeHints = hints;
	descriptor.instantiate = Create;
	descriptor.connect_port = Connect;
	descriptor.run = Run;
	descriptor.cleanup = Destroy;
	return index ? NULL : &descriptor;
}
