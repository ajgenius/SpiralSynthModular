// SPDX-License-Identifier: GPL-2.0-or-later
// GVerb is the plugin Examples/Tutorial7-LADSPA.ssm asks for by name, and the
// copy bundled with the macOS application is the only one most Macs will ever
// have. Two things have to hold, and loading proves neither on its own: the
// device has to find it on the search path, and the reverb has to keep making
// sound after its input stops. A plugin that instantiates and returns silence
// would pass a smoke test and fail a listener.
#include "LADSPAPlugin.h"
#include <ladspa.h>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dirent.h>
#include <dlfcn.h>
#include <string>

namespace {

const unsigned long GVERB_ID = 1216;
const unsigned long BLOCK = 256;
const unsigned long RATE = 44100;

double Energy(const float *buffer, unsigned long n)
{
	double sum = 0.0;
	for (unsigned long i = 0; i < n; ++i) sum += (double)buffer[i] * buffer[i];
	return sum;
}

// Run the module directly, which is the only way to see what it does with a
// signal: the device wrapper's own buffers are not reachable from here.
bool TailOutlastsInput(const std::string &module, double &driven, double &tail)
{
	void *handle = dlopen(module.c_str(), RTLD_NOW | RTLD_LOCAL);
	if (!handle) { printf("  dlopen: %s\n", dlerror()); return false; }

	LADSPA_Descriptor_Function describe =
		(LADSPA_Descriptor_Function)dlsym(handle, "ladspa_descriptor");
	if (!describe) { printf("  no ladspa_descriptor\n"); return false; }

	const LADSPA_Descriptor *d = describe(0);
	if (!d || d->UniqueID != GVERB_ID) { printf("  wrong descriptor\n"); return false; }

	LADSPA_Handle instance = d->instantiate(d, RATE);
	if (!instance) { printf("  instantiate failed\n"); return false; }

	// Connect by port role rather than by index: the descriptor is upstream's
	// to order, not ours to assume.
	float *audio_in = new float[BLOCK];
	float *audio_out[8];
	unsigned outputs = 0;
	float controls[32];
	for (unsigned long p = 0; p < d->PortCount; ++p)
	{
		const LADSPA_PortDescriptor pd = d->PortDescriptors[p];
		if (LADSPA_IS_PORT_AUDIO(pd) && LADSPA_IS_PORT_INPUT(pd))
			d->connect_port(instance, p, audio_in);
		else if (LADSPA_IS_PORT_AUDIO(pd) && LADSPA_IS_PORT_OUTPUT(pd))
		{
			assert(outputs < 8);
			audio_out[outputs] = new float[BLOCK];
			d->connect_port(instance, p, audio_out[outputs++]);
		}
		else
		{
			// Control ports keep the plugin's own default where it states one.
			const LADSPA_PortRangeHint &hint = d->PortRangeHints[p];
			float value = 0.5f;
			if (LADSPA_IS_HINT_DEFAULT_LOW(hint.HintDescriptor)) value = hint.LowerBound;
			else if (LADSPA_IS_HINT_DEFAULT_HIGH(hint.HintDescriptor)) value = hint.UpperBound;
			else if (LADSPA_IS_HINT_BOUNDED_BELOW(hint.HintDescriptor) &&
			         LADSPA_IS_HINT_BOUNDED_ABOVE(hint.HintDescriptor))
				value = hint.LowerBound + (hint.UpperBound - hint.LowerBound) * 0.5f;
			controls[p] = value;
			d->connect_port(instance, p, &controls[p]);
		}
	}
	if (!outputs) { printf("  no audio output ports\n"); return false; }
	if (d->activate) d->activate(instance);

	// A short burst, the way a key press drives the example, then silence.
	driven = 0.0;
	for (unsigned block = 0; block < 8; ++block)
	{
		for (unsigned long i = 0; i < BLOCK; ++i)
		{
			const double t = (double)(block * BLOCK + i) / RATE;
			audio_in[i] = (float)(0.5 * sin(2.0 * M_PI * 440.0 * t));
		}
		d->run(instance, BLOCK);
		driven += Energy(audio_out[0], BLOCK);
	}

	memset(audio_in, 0, BLOCK * sizeof(float));
	double early = 0.0, late = 0.0;
	for (unsigned block = 0; block < 40; ++block)
	{
		d->run(instance, BLOCK);
		const double e = Energy(audio_out[0], BLOCK);
		if (block < 8) early += e;
		if (block >= 32) late += e;
	}
	tail = early;

	if (d->deactivate) d->deactivate(instance);
	d->cleanup(instance);
	delete [] audio_in;
	for (unsigned n = 0; n < outputs; ++n) delete [] audio_out[n];
	dlclose(handle);

	printf("  driven %.6f, tail %.6f, decaying to %.6f\n", driven, early, late);
	// A reverb goes on sounding after the input stops, and fades while it does.
	return driven > 0.0 && early > 0.0 && late < early;
}

// Every module shipped alongside it has to load too. A module whose symbols
// are only resolved at load time links cleanly on macOS and fails here, which
// is the last place to find out before a user does.
unsigned ModulesThatWillNotLoad(const std::string &directory)
{
	DIR *d = opendir(directory.c_str());
	if (!d) { printf("  cannot read %s\n", directory.c_str()); return 1; }

	unsigned failures = 0, loaded = 0;
	for (struct dirent *e = readdir(d); e; e = readdir(d))
	{
		const std::string name = e->d_name;
		if (name.size() < 4 || name.compare(name.size() - 3, 3, ".so") != 0) continue;
		const std::string path = directory + "/" + name;
		void *handle = dlopen(path.c_str(), RTLD_NOW | RTLD_LOCAL);
		if (!handle) { printf("  will not load: %s\n", dlerror()); ++failures; }
		else { ++loaded; dlclose(handle); }
	}
	closedir(d);
	printf("  %u modules load, %u do not\n", loaded, failures);
	return failures;
}

}

int main(int argc, char **argv)
{
	const char *given = argc == 2 ? argv[1] : getenv("LADSPA_BUNDLE_DIR");
	if (!given) return 77;
	const std::string directory = given;
	setenv("LADSPA_PATH", directory.c_str(), 1);

	// The device has to find GVerb through its own search, as it would when
	// the example names it.
	HostInfo host = HostInfo();
	host.BUFSIZE = BLOCK;
	host.SAMPLERATE = RATE;

	LADSPAPlugin plugin;
	plugin.Initialise(&host);

	spiralcore::Description saved;
	saved.Value(9).Value(0).Value(false).Value(GVERB_ID).Value(0).Value(0);
	spiralcore::Description::Reader state(saved);
	plugin.Apply(state);
	assert(!state.Failed());
	printf("  device selected id %lu with %lu input ports\n",
	       plugin.GetUniqueID(), plugin.GetInputPortCount());
	assert(plugin.GetUniqueID() == GVERB_ID);
	assert(plugin.GetInputPortCount() > 0);

	double driven = 0.0, tail = 0.0;
	assert(TailOutlastsInput(directory + "/gverb_1216.so", driven, tail));
	assert(ModulesThatWillNotLoad(directory) == 0);

	printf("PASS\n");
	return 0;
}
