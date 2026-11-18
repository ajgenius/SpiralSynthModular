// SPDX-License-Identifier: GPL-2.0-or-later
#include "ClockPlugin.h"
#include "Description.h"
#include <cassert>
#include <cstdio>
#include <dlfcn.h>
#include <unistd.h>

using namespace Spiral::Core;

int main()
{
	const char *module = "Plugins/Devices/MatrixPlugin/.libs/MatrixPlugin_DSP.so";
	if (access(module, F_OK))
		return 77;

	void *library = dlopen(module, RTLD_NOW | RTLD_LOCAL);
	assert(library);
	typedef SpiralPlugin *(*Factory)();
	Factory create = reinterpret_cast<Factory>(dlsym(library, "SpiralPlugin_CreateInstance"));
	assert(create);
	Device *matrix = dynamic_cast<Device *>(create());
	assert(matrix);
	HostInfo host = {};
	host.SAMPLERATE = 800;
	host.BUFSIZE = 25;
	host.ROLLING = true;
	ClockPlugin clock;
	clock.Initialise(&host);
	matrix->Initialise(&host);
	matrix->Reset();

	// The original five-field Clock persisted BPM in quarter notes.
	Description saved;
	saved.Value(1).Value(80).Value(6).Value(8).Value(1).Value(1);
	Description::Reader reader(saved);
	clock.Apply(reader);
	assert(!reader.Failed());
	Description legacy;
	clock.Describe(legacy);
	assert(legacy.Values().size() == 8 && legacy.Values()[6] == "0");
	// Public patch files also store the original five settings by property name.
	Description namedLegacy;
	for (unsigned i = 0; i < 6; ++i)
		namedLegacy.Value(legacy.Values()[i]).Key(legacy.Keys()[i], legacy.Classes()[i]);
	Description::Reader oldPatch(namedLegacy);
	clock.Apply(oldPatch);
	assert(!oldPatch.Failed());
	SignedProperty dotted(DefaultLinearFlags, 1, 0, 5, 1, 1);
	clock.SynchronizeProperty("TEMPONOTE", &dotted);
	Description compound;
	clock.Describe(compound);
	ClockPlugin restored;
	Description::Reader reload(compound);
	restored.Apply(reload);
	assert(!reload.Failed());
	Description copy;
	restored.Describe(copy);
	assert(compound.Values() == copy.Values());

	Sample *steps = 0, *bar = 0, *wrap = 0;
	assert(clock.GetOutput(2, &steps) && clock.GetOutput(1, &bar));
	assert(matrix->SetInput(4, steps) && matrix->GetOutput(18, &wrap));
	std::string key = "STEP";
	for (host.FRAME = 0; host.FRAME <= 6400; host.FRAME += host.BUFSIZE)
	{
		clock.Execute();
		matrix->Execute();
		SignedProperty *step = static_cast<SignedProperty *>(matrix->DuplicateProperty(key));
		assert(step && step->Value.AsSigned == int(host.FRAME / 100 % 64));
		delete step;
		for (int n = 0; n < host.BUFSIZE; ++n)
		{
			const unsigned frame = host.FRAME + n;
			assert(((*bar)[n] > 0) == (frame % 1200 == 0));
			assert(((*wrap)[n] > 0) == (frame == 6400));
		}
	}
	delete matrix;
	dlclose(library);
	std::puts("6/8, dotted quarter = 80, all 64 Matrix steps, legacy and current clock settings PASS");
}
