// SPDX-License-Identifier: GPL-2.0-or-later
// The registry hands out clients by name: builtins are present, a
// descriptor registered at runtime creates and destroys through its own
// functions, a wrong ABI, wrong kind or duplicate name is refused, and
// unknown names yield NULL.
#include "AudioBackend.h"
#include <cassert>
#include <cstdio>
#include <vector>

using namespace spiralcore;

struct NullClient : public AudioClient
{
	bool Attach(const std::string &, const AudioClientOptions &) { return false; }
	void Detach() {}
	bool IsAttached() const { return false; }
	bool Write(const float *, unsigned int) { return false; }
	bool Read(float *, unsigned int) { return false; }
};

static unsigned created = 0, destroyed = 0;
static void *Create(void *) { ++created; return static_cast<AudioClient *>(new NullClient); }
static void Destroy(void *client) { ++destroyed; delete static_cast<AudioClient *>(client); }

int main()
{
	AudioBackendRegistry *registry = AudioBackendRegistry::Get();
	std::vector<std::string> names = registry->Names();
	printf("builtin backends:");
	for (size_t n = 0; n < names.size(); ++n) printf(" %s", names[n].c_str());
	printf("\n");

	const BackendDescriptor null = { SPIRAL_AUDIO_PLUGIN_ABI, "audio", "null", Create, Destroy };
	registry->Register(&null);
	assert(registry->Find("null") == &null);

	const BackendDescriptor wrong = { SPIRAL_AUDIO_PLUGIN_ABI + 1, "audio", "wrong", Create, Destroy };
	registry->Register(&wrong);
	assert(!registry->Find("wrong"));

	const BackendDescriptor midi = { SPIRAL_AUDIO_PLUGIN_ABI, "midi", "midi", Create, Destroy };
	registry->Register(&midi);
	assert(!registry->Find("midi"));

	const BackendDescriptor duplicate = { SPIRAL_AUDIO_PLUGIN_ABI, "audio", "null", Create, Destroy };
	registry->Register(&duplicate);
	assert(registry->Find("null") == &null);

	AudioClient *client = registry->Create("null");
	assert(client && created == 1);
	registry->Destroy("null", client);
	assert(destroyed == 1);
	assert(!registry->Create("nonesuch"));
	assert(registry->LoadModules("/nonexistent/path") == 0);
	PluginLoader::PackUpAndGoHome();

	AudioBackendRegistry::PackUpAndGoHome();
	printf("PASS\n");
	return 0;
}
