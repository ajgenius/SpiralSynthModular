// SPDX-License-Identifier: GPL-2.0-or-later
// Backends compiled into libspiralcore, registered with the same
// descriptor a module would export. Each moves to Plugins/Audio as a
// module in its own change and leaves this file; when only the dummy is
// left it registers itself and this file goes.
#include "AudioBackend.h"
#include "config.h"

#ifdef HAVE_OUTPUT_ALSA
#include "AlsaClient.h"
#endif
#ifdef HAVE_OUTPUT_OSS
#include "OSSClient.h"
#endif
#ifdef HAVE_JACK_CLIENT
#include "JackClient.h"
#endif
#ifdef HAVE_CORE_AUDIO_CLIENT
#include "CoreAudioClient.h"
#endif

using namespace spiralcore;

#ifdef HAVE_OUTPUT_ALSA
static void *CreateAlsa(void *) { return static_cast<AudioClient *>(AlsaClient::Get()); }
static void DestroyAlsa(void *) { AlsaClient::PackUpAndGoHome(); }
static const BackendDescriptor AlsaBackend = { SPIRAL_AUDIO_PLUGIN_ABI, "audio", "alsa", CreateAlsa, DestroyAlsa };
#endif

#ifdef HAVE_OUTPUT_OSS
static void *CreateOSS(void *) { return static_cast<AudioClient *>(OSSClient::Get()); }
static void DestroyOSS(void *) { OSSClient::PackUpAndGoHome(); }
static const BackendDescriptor OSSBackend = { SPIRAL_AUDIO_PLUGIN_ABI, "audio", "oss", CreateOSS, DestroyOSS };
#endif

#ifdef HAVE_JACK_CLIENT
static void *CreateJack(void *) { return static_cast<AudioClient *>(new JackClient); }
static void DestroyJack(void *client) { delete static_cast<AudioClient *>(client); }
static const BackendDescriptor JackBackend = { SPIRAL_AUDIO_PLUGIN_ABI, "audio", "jack", CreateJack, DestroyJack };
#endif

// Registration order is the fallback preference when no backend is named.
#ifdef HAVE_CORE_AUDIO_CLIENT
static void *CreateCoreAudio(void *) { return static_cast<AudioClient *>(new CoreAudioClient); }
static void DestroyCoreAudio(void *client) { delete static_cast<AudioClient *>(client); }
static const BackendDescriptor CoreAudioBackend = { SPIRAL_AUDIO_PLUGIN_ABI, "audio", "coreaudio", CreateCoreAudio, DestroyCoreAudio };
#endif

namespace spiralcore { void RegisterBuiltinAudioBackends(AudioBackendRegistry *registry); }

void spiralcore::RegisterBuiltinAudioBackends(AudioBackendRegistry *registry)
{
#ifdef HAVE_CORE_AUDIO_CLIENT
	registry->Register(&CoreAudioBackend);
#endif
#ifdef HAVE_OUTPUT_ALSA
	registry->Register(&AlsaBackend);
#endif
#ifdef HAVE_OUTPUT_OSS
	registry->Register(&OSSBackend);
#endif
#ifdef HAVE_JACK_CLIENT
	registry->Register(&JackBackend);
#endif
	registry->Register(DummyBackend());
}
