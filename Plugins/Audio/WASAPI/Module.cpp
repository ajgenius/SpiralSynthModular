// SPDX-License-Identifier: GPL-2.0-or-later
#include "WASAPIClient.h"
#include "AudioBackend.h"

using namespace Spiral::Core;

static void *CreateWASAPI(void *) { return static_cast<AudioClient *>(new WASAPIClient); }
static void DestroyWASAPI(void *client) { delete static_cast<AudioClient *>(client); }
static const BackendDescriptor WASAPIBackend =
{
	SPIRAL_AUDIO_PLUGIN_ABI, "audio", "wasapi", CreateWASAPI, DestroyWASAPI
};

extern "C" __declspec(dllexport) const BackendDescriptor *SpiralPlugin_GetAudioBackend()
{
	return &WASAPIBackend;
}
