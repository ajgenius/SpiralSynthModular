// SPDX-License-Identifier: GPL-2.0-or-later
// The loader finds a kind's modules under <root>/<subdirectory>, resolves
// the kind's entry symbol and lets the kind accept or refuse; a module
// without the symbol is dropped, a missing root loads nothing.
#include <config.h>
#include "PluginManager.h"
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

using namespace Spiral::Core;

struct ProbeKind : public PluginKind
{
	std::vector<std::string> Seen;
	bool Refuse, Busy;
	unsigned Released;
	ProbeKind() : Refuse(false), Busy(false), Released(0) {}
	bool CanUnload() const { return !Busy; }
	void Release(void *entry, const std::string &)
	{
		assert(entry);
		++Released;
	}
	const char *Name() const { return "probe"; }
	const char *Subdirectory() const { return "probe"; }
	const char *Suffix() const { return "_Probe"; }
	const char *EntrySymbol() const { return "SpiralProbePlugin_GetTable"; }
	bool Accept(void *entry, const std::string &path)
	{
		const char *(*table)() = (const char *(*)())entry;
		Seen.push_back(std::string(table()) + " " + path);
		return !Refuse;
	}
};

// A kind that claims a host-dependent variant beside its own suffix.
struct VariantKind : public ProbeKind
{
	bool Claims(const std::string &file) const
	{
		static const std::string native = std::string("_Native") + PLUGIN_MODULE_EXT;
		return PluginKind::Claims(file)
			|| (file.size() > native.size() && file.compare(file.size() - native.size(), native.size(), native) == 0);
	}
};

int main(int argc, char **argv)
{
	const char *given = argc == 2 ? argv[1] : getenv("PLUGIN_MANAGER_ROOT");
	if (!given) return 77;

	const std::string root = given;
	ProbeKind probe;
	PluginManager *loader = PluginManager::Get();
	loader->RegisterKind(&probe);
	loader->RegisterKind(&probe);

	assert(loader->Load(probe, "/nonexistent") == 0);

	// probe/probe_Probe.so and probe/sub/sub_Probe.so load; stranger_Probe.so has no entry.
	const unsigned loaded = loader->LoadAll(root);
	for (size_t n = 0; n < probe.Seen.size(); ++n) printf("accepted %s\n", probe.Seen[n].c_str());
	assert(loaded == 2 && probe.Seen.size() == 2);

	probe.Refuse = true;
	probe.Seen.clear();
	assert(loader->Load(probe, root) == 0 && probe.Seen.size() == 2);

	probe.Busy = true;
	assert(!loader->UnloadAll() && probe.Released == 0);
	assert(!loader->UnregisterKind(&probe));
	PluginManager::PackUpAndGoHome();
	assert(PluginManager::Get() == loader);

	probe.Busy = false;
	assert(loader->UnregisterKind(&probe) && probe.Released == 2);
	assert(loader->LoadAll(root) == 0);
	assert(loader->UnloadAll());

	// probe/variant_Native.so is a stranger to the plain kind and a third
	// module to the one that claims the variant.
	VariantKind variant;
	assert(loader->Load(variant, root) == 3 && variant.Seen.size() == 3);
	loader->RegisterKind(&variant);
	assert(loader->UnregisterKind(&variant) && variant.Released == 3);
	PluginManager::PackUpAndGoHome();
	printf("PASS\n");
	return 0;
}
