// SPDX-License-Identifier: GPL-2.0-or-later
#include "DeviceClassRegistry.h"
#include "SpiralPlugin.h"
#include <cassert>

int main(int argc, char **argv)
{
	if (argc != 3)
		return 77;

	spiralcore::DeviceClassRegistry *registry = spiralcore::DeviceClassRegistry::Get();
	assert(registry->LoadModules(argv[1]) == 0);
	assert(!registry->Find(9));
	assert(registry->LoadModules(argv[2]) > 0);
	const spiralcore::DeviceClass *current = registry->Find(9);
	assert(current);
	SpiralPlugin *device = current->CreateInstance();
	assert(device);
	delete device;
	spiralcore::DeviceClassRegistry::PackUpAndGoHome();
}
