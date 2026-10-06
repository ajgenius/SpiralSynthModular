// SPDX-License-Identifier: GPL-2.0-or-later
#include "AudioBackend.h"
#include <cassert>
#include <cctype>
#include <cstdio>
#include <string>
#include <vector>
using namespace spiralcore;

int main(int argc, char **argv)
{
	assert(argc >= 2);
	AudioBackendRegistry *registry = AudioBackendRegistry::Get();
	std::vector<std::string> names;
	for (int n = 2; n < argc; ++n)
	{
		std::string name = argv[n];
		for (size_t i = 0; i < name.size(); ++i)
			name[i] = std::tolower(static_cast<unsigned char>(name[i]));

		names.push_back(name);
		assert(!registry->Find(name));
	}

	for (unsigned pass = 0; pass < 3; ++pass)
	{
		assert(registry->LoadModules(argv[1]) == names.size());
		for (size_t n = 0; n < names.size(); ++n)
		{
			AudioClient *first = registry->Create(names[n]);
			AudioClient *second = registry->Create(names[n]);
			assert(first && second && first != second);
			assert(!PluginLoader::Get()->UnloadAll());

			registry->Destroy(names[n], first);
			assert(!second->IsAttached());
			assert(!PluginLoader::Get()->UnloadAll());
			registry->Destroy(names[n], second);
		}

		assert(PluginLoader::Get()->UnloadAll());
		for (size_t n = 0; n < names.size(); ++n)
			assert(!registry->Find(names[n]));

		assert(registry->Find("dummy"));
	}

	AudioBackendRegistry::PackUpAndGoHome();
	PluginLoader::PackUpAndGoHome();
	std::printf("%lu native modules: independent instances, unload refusal and reload PASS\n",
		static_cast<unsigned long>(names.size()));
}
