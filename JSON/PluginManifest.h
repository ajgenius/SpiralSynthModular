// Host plugin metadata. Reading does not open a module. GPL-2.0-or-later.
#ifndef SSM_PLUGIN_MANIFEST_H
#define SSM_PLUGIN_MANIFEST_H
#include <string>
#include <vector>

struct PluginManifest {
	int id;
	std::string name, type, category, version, hostVersion, hostABI;
	std::string module, registration, guiName, guiABI, guiVersion;
	std::vector<std::string> authors;
	PluginManifest() : id(-1) {}
	bool Read(const std::string &path, std::string &error);
	bool Compatible(const std::string &hostVersion, const std::string &hostABI,
			const std::string &guiName, const std::string &guiABI,
			std::string &error) const;
};
#endif
