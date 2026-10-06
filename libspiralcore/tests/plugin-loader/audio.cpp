// SPDX-License-Identifier: GPL-2.0-or-later
#include "AudioBackend.h"
using namespace spiralcore;
class ProbeClient : public AudioClient
{
public:
	bool Attach(const std::string &, const AudioClientOptions &) { return true; }
	void Detach() {}
	bool IsAttached() const { return true; }
	bool Write(const float *, unsigned) { return true; }
	bool Read(float *, unsigned) { return true; }
};
static void *Create(void *) { return static_cast<AudioClient *>(new ProbeClient); }
static void Destroy(void *client) { delete static_cast<AudioClient *>(client); }
extern "C" const BackendDescriptor *SpiralPlugin_GetAudioBackend()
{
	static const BackendDescriptor descriptor = { SPIRAL_AUDIO_PLUGIN_ABI, "audio", "probe", Create, Destroy };
	return &descriptor;
}
