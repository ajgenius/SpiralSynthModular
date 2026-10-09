// Backend modules alone link hardware API libraries.
#include "BackendPath.h"
#include "AudioBackend.h"
#include "MidiBackend.h"
#include <cstdlib>
namespace Spiral
{
namespace Core
{
	static void LoadPaths(PluginKind &kind, const std::string &root)
	{
		const char *env = std::getenv("SSM_BACKEND_PATH");
		const std::string paths = env ? env : root;
		size_t start = 0, end;
		do
		{
			end = paths.find(':', start);
			const std::string path = paths.substr(start, end - start);
			if (!path.empty())
				PluginManager::Get()->Load(kind, path);
			start = end + 1;
		} while (end != std::string::npos);
	}
	void LoadAudioBackends(const std::string &root)
	{
		LoadPaths(*AudioBackendRegistry::Get(), root);
	}
	void LoadMidiBackends(const std::string &root)
	{
		LoadPaths(*MidiBackendRegistry::Get(), root);
	}
} // namespace Core
} // namespace Spiral
