// SPDX-License-Identifier: GPL-2.0-or-later
// Backends compiled into libspiralcore, registered with the same
// descriptor a module would export. Each moves to Plugins/Audio as a
// module in its own change and leaves this file; when only the dummy is
// left it registers itself and this file goes.
#include "AudioBackend.h"
#include "config.h"

#ifdef HAVE_JACK_CLIENT
#include "JackClient.h"
#endif

using namespace spiralcore;



#ifdef HAVE_JACK_CLIENT
static void *CreateJack(void *) { return static_cast<AudioClient *>(new JackClient); }
static void DestroyJack(void *client) { delete static_cast<AudioClient *>(client); }
static const BackendDescriptor JackBackend = { SPIRAL_AUDIO_PLUGIN_ABI, "audio", "jack", CreateJack, DestroyJack };
#endif

// Registration order is the fallback preference when no backend is named.
namespace spiralcore { void RegisterBuiltinAudioBackends(AudioBackendRegistry *registry); }

void spiralcore::RegisterBuiltinAudioBackends(AudioBackendRegistry *registry)
{
#ifdef HAVE_JACK_CLIENT
	registry->Register(&JackBackend);
#endif
	registry->Register(DummyBackend());
}
