#include "AudioTransportHub.h"
#include "OutputPlugin.h"
#include "AudioBackend.h"
#include "DummyClient.h"
#include <cassert>
#include <cstdio>
#include "AudioTiming.h"

// Register a paced stand-in before loading any native modules. Count backend
// lifetimes independently of allocator address reuse.
static unsigned created = 0, destroyed = 0;
static void *CreateTestClient(void *)
{
	++created;
	return new Spiral::Core::DummyClient;
}

static void DestroyTestClient(void *client)
{
	delete static_cast<Spiral::Core::DummyClient *>(client);
	++destroyed;
}

static const Spiral::Core::BackendDescriptor *RegisteredJack()
{
	return Spiral::Core::AudioBackendRegistry::Get()->Find("jack");
}

static void RegisterTestBackends()
{
	static const Spiral::Core::BackendDescriptor alsa = {
		SPIRAL_AUDIO_PLUGIN_ABI, "audio", "alsa", CreateTestClient, DestroyTestClient
	};
	static const Spiral::Core::BackendDescriptor jack = {
		SPIRAL_AUDIO_PLUGIN_ABI, "audio", "jack", CreateTestClient, DestroyTestClient
	};
	Spiral::Core::AudioBackendRegistry *registry = Spiral::Core::AudioBackendRegistry::Get();
	if (RegisteredJack())
		registry->Release(reinterpret_cast<void *>(&RegisteredJack), "");
	registry->Register(&alsa);
	registry->Register(&jack);
}

static void ReplacementLifetime(const char *driver)
{
	HostInfo host = HostInfo();
	host.AUDIOCLIENT = driver;
	created = destroyed = 0;
	host.OUTPUTFILE = "first";
	host.BUFSIZE = 64;
	host.SAMPLERATE = 48000;
	AudioTransportHub *hub = AudioTransportHub::Get();
	hub->SetHost(&host);

	OutputPlugin oldOutput, newOutput, changedOutput;
	oldOutput.Initialise(&host);
	oldOutput.ServiceAudio();
	assert(hub->GetMode() == AudioTransportHub::OUTPUT && created == 1);

	hub->BeginPatchReplacement();
	oldOutput.Kill();
	assert(destroyed == 0);
	newOutput.Initialise(&host);
	hub->Reconfigure();
	hub->EndPatchReplacement();
	assert(hub->GetMode() == AudioTransportHub::OUTPUT && created == 1 && destroyed == 0);

	hub->BeginPatchReplacement();
	newOutput.Kill();
	host.OUTPUTFILE = "second";
	changedOutput.Initialise(&host);
	hub->Reconfigure();
	hub->EndPatchReplacement();
	assert(hub->GetMode() == AudioTransportHub::OUTPUT && created == 2 && destroyed == 1);

	hub->BeginPatchReplacement();
	changedOutput.Kill();
	assert(destroyed == 1);
	hub->EndPatchReplacement();
	assert(hub->GetMode() == AudioTransportHub::NO_MODE && destroyed == 2);
	hub->SetHost(NULL);
}

int main()
{
	RegisterTestBackends();
	ReplacementLifetime("alsa");
	ReplacementLifetime("jack");
	HostInfo host = HostInfo();
	host.BUFSIZE = 960;
	host.SAMPLERATE = 48000;
	AudioTransportHub *hub = AudioTransportHub::Get();
	hub->SetHost(&host);
	const double start = Spiral::Core::AudioMonotonicTime();
	for (unsigned n = 0; n < 10; ++n)
	{
		assert(hub->WaitPeriod());
		hub->BeginPeriod();
		hub->CommitPeriod();
	}

	const double elapsed = Spiral::Core::AudioMonotonicTime() - start;
	printf("No Output: 9600 frames at 48 kHz in %.4f seconds\n", elapsed);
	assert(hub->Frame() == 9600);
	// The hub's own clock, and its allowance for a period that is due within
	// 10 microseconds: the wall clock can be slewed, and nine periods of waiting
	// come to exactly the lower limit.
	assert(elapsed >= 0.18 - 0.00002 && elapsed < 0.5);
	hub->SetHost(NULL);
	return 0;
}
