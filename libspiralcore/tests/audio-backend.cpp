// SPDX-License-Identifier: GPL-2.0-or-later
// The registry hands out clients by name: builtins are present, a
// descriptor registered at runtime creates and destroys through its own
// functions, a wrong ABI, wrong kind or duplicate name is refused, and
// unknown names yield NULL.
#include "AudioBackend.h"
#include <cassert>
#include <cstdio>
#include <vector>
#include <cstdlib>

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

	for (size_t n = 0; n < names.size(); ++n)
	{
		AudioClient *first = registry->Create(names[n]);
		AudioClient *second = registry->Create(names[n]);
		assert(first && second && first != second);
		registry->Destroy(names[n], first);
		registry->Destroy(names[n], second);
	}

	const BackendDescriptor null = { SPIRAL_AUDIO_PLUGIN_ABI, "audio", "null", Create, Destroy };
	registry->Register(&null);
	assert(registry->Find("null") == &null);
	// The dummy stays last however late the others register.
	names = registry->Names();
	assert(names.size() >= 2 && names.back() == "dummy" && names[names.size() - 2] == "null");

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
	const char *root = std::getenv("PLUGIN_LOADER_ROOT");
	assert(root && registry->LoadModules(root) == 1);
	AudioClient *module = registry->Create("probe");
	assert(module && module->IsAttached());
	assert(!PluginLoader::Get()->UnloadAll());
	AudioBackendRegistry::PackUpAndGoHome();
	assert(AudioBackendRegistry::Get() == registry && module->IsAttached());
	registry->Destroy("probe", module);
	assert(PluginLoader::Get()->UnloadAll());
	assert(!registry->Find("probe") && registry->Find("dummy"));
	assert(registry->LoadModules(root) == 1);
	PluginLoader::PackUpAndGoHome();
	assert(!registry->Find("probe"));
	assert(AudioBackendRegistry::Get() == registry);
	assert(PluginLoader::Get()->LoadAll(root) == 1);

	// The built-in dummy paces at the nominal rate with no device:
	// 50 periods of 256 frames at 48 kHz take about 0.267 s.
	AudioClient *dummy = registry->Create("dummy");
	assert(dummy);
	AudioClientOptions options;
	options.BufferSize = 256; options.Samplerate = 48000; options.InChannels = 2; options.OutChannels = 2;
	assert(dummy->Attach("", options) && dummy->IsAttached() && !dummy->IsCallbackDriven());
	std::vector<float> block(256 * 2, 0.5f);
	const double start = AudioMonotonicTime();
	unsigned cycles = 0;
	while (cycles < 50)
	{
		const int ready = dummy->WaitForCycle(10);
		assert(ready >= 0);
		if (!ready) continue;

		AudioCycleTiming timing;
		assert(dummy->GetCycleTiming(timing) && timing.Valid && timing.Estimated && timing.Frames == 256);
		assert(dummy->Write(&block[0], 256) && dummy->Read(&block[0], 256) && block[0] == 0.f);
		++cycles;
	}
	const double elapsed = AudioMonotonicTime() - start;
	printf("dummy: 50 periods in %.4f s (nominal %.4f)\n", elapsed, 50 * 256 / 48000.0);
	assert(elapsed > 0.24 && elapsed < 0.40);
	dummy->Detach();
	registry->Destroy("dummy", dummy);

	AudioBackendRegistry::PackUpAndGoHome();
	assert(PluginLoader::Get()->LoadAll(root) == 0);
	PluginLoader::PackUpAndGoHome();
	printf("PASS\n");
	return 0;
}
