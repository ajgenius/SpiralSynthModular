// SPDX-License-Identifier: GPL-2.0-or-later
#include "DeviceClassRegistry.h"
#include "SpiralPlugin.h"
#include <cassert>

int main(int argc, char **argv)
{
	if (argc != 3)
		return 77;

	Spiral::Core::DeviceClassRegistry *registry = Spiral::Core::DeviceClassRegistry::Get();
	assert(registry->LoadModules(argv[1]) == 0);
	assert(!registry->Find(9));
	assert(registry->LoadModules(argv[2]) > 0);
	const Spiral::Core::DeviceClass *current = registry->Find(9);
	assert(current);
	SpiralPlugin *device = current->CreateInstance();
	assert(device);
	delete device;
	Spiral::Core::DeviceClassRegistry::PackUpAndGoHome();
}
