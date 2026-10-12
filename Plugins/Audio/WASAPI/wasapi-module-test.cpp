// SPDX-License-Identifier: GPL-2.0-or-later
#include <windows.h>
#include <cstdio>
#include <cstring>
#include "AudioBackend.h"
using namespace Spiral::Core;

int main(int argc, char **argv)
{
	const char *path = argc > 1 ? argv[1] : ".libs/WASAPI_Audio.dll";
	for (unsigned pass = 0; pass < 3; ++pass)
	{
		HMODULE module = LoadLibraryA(path);
		if (!module) return 1;

		SpiralPlugin_GetAudioBackendFn entry = reinterpret_cast<SpiralPlugin_GetAudioBackendFn>(
			reinterpret_cast<void (*)()>(GetProcAddress(module, "SpiralPlugin_GetAudioBackend")));
		if (!entry) return 1;

		const BackendDescriptor *backend = entry();
		if (!backend || backend->ABI != SPIRAL_AUDIO_PLUGIN_ABI
			|| std::strcmp(backend->Kind, "audio") || std::strcmp(backend->Name, "wasapi")) return 1;

		AudioClient *first = static_cast<AudioClient *>(backend->Create(0));
		AudioClient *second = static_cast<AudioClient *>(backend->Create(0));
		if (!first || !second || first == second || first->IsAttached() || second->IsAttached()) return 1;

		AudioClientOptions options;
		// Exercise the exported interface without assuming an audio endpoint.
		options.InChannels = 1;
		if (first->Attach("default", options) || second->Start()) return 1;

		backend->Destroy(first);
		backend->Destroy(second);
		if (!FreeLibrary(module)) return 1;
	}

	std::puts("PASS WASAPI DLL descriptor, independent instances and reload");
	return 0;
}
