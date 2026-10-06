// SPDX-License-Identifier: GPL-2.0-or-later
#include "AudioBackend.h"
#include "AlsaClient.h"
using namespace spiralcore;

static void *Create(void *)
{
	return static_cast<AudioClient *>(new AlsaClient);
}

static void Destroy(void *client)
{
	delete static_cast<AudioClient *>(client);
}

extern "C" const BackendDescriptor *SpiralPlugin_GetAudioBackend()
{
	static const BackendDescriptor descriptor = {SPIRAL_AUDIO_PLUGIN_ABI, "audio", "alsa", Create, Destroy};
	return &descriptor;
}
