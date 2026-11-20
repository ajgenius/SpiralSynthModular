// SPDX-License-Identifier: GPL-2.0-or-later
#include <config.h>
#include "DeviceClassRegistry.h"
#include "SpiralPlugin.h"
#include <cassert>
#include <cstdlib>
#include <unistd.h>

int main(int argc, char **argv)
{
	if (argc != 1 && argc != 3 && argc != 4)
		return 77;

	Spiral::Core::DeviceClassRegistry *registry = Spiral::Core::DeviceClassRegistry::Get();
	const char *older = argc == 1 ? "tests/abi9-modules" : argv[1];
	const char *rebuilt = argc == 1 ? "tests/surface-modules" : argv[2];
	int id = 9;
	if (argc == 1)
	{
		assert(access("tests/abi9-modules/dsp/OldDevice_DSP" PLUGIN_MODULE_EXT, R_OK) == 0);
		id = 2;
	}
	if (argc == 4)
		id = std::atoi(argv[3]);

	assert(Spiral::Core::PluginManager::Get()->Load(*registry, older) == 0);
	assert(!registry->Find(id));
	assert(registry->LoadModules(rebuilt) > 0);
	const Spiral::Core::DeviceClass *current = registry->Find(id);
	assert(current);
	SpiralPlugin *device = current->CreateInstance();
	assert(device);
	delete device;
	Spiral::Core::DeviceClassRegistry::PackUpAndGoHome();
}
