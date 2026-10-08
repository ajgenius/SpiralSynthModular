// SPDX-License-Identifier: GPL-2.0-or-later
#include "LADSPAPlugin.h"
#include <cassert>

int main()
{
	HostInfo host = HostInfo();
	host.BUFSIZE = 256;
	host.SAMPLERATE = 44100;
	const unsigned counts[] = {0, 3, 8, 10};
	for (unsigned n = 0; n < sizeof(counts) / sizeof(*counts); ++n)
	{
		LADSPAPlugin plugin;
		plugin.Initialise(&host);
		spiralcore::Description saved;
		saved.Value(9).Value(2).Value(false).Value(7654321).Value(counts[n]).Value(counts[n]);
		for (unsigned i = 0; i < counts[n]; ++i) saved.Value(0);
		for (unsigned i = 0; i < counts[n]; ++i) saved.Value(2);
		for (unsigned i = 0; i < counts[n]; ++i) saved.Value(true);
		for (unsigned i = 0; i < counts[n]; ++i) saved.Value(0.25f);
		spiralcore::Description::Reader input(saved);
		plugin.Apply(input);
		assert(!input.Failed() && plugin.GetPluginInfo().NumInputs == 8);
		for (unsigned repeat = 0; repeat < 2; ++repeat)
		{
			plugin.Reset();
			plugin.Execute();
			spiralcore::Description result;
			plugin.Describe(result);
			assert(result.Values().size() == 6 + 4 * 8);
			assert(result.Values()[4] == "8");
			for (unsigned i = 0; i < 8; ++i)
				assert(result.Values()[6 + 3 * 8 + i] == (i < counts[n] ? "0.25" : "1"));
		}
	}
}
