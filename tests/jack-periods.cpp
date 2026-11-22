#define JACK_CLIENT_TEST_FIXTURE
#include "../libs/libspiralcore/Audio/tests/jack-client.cpp"
#include "JackPlugin.h"
#include "PatchFile.h"
#include "AudioTransportHub.h"
#include <cmath>
#include <cstdio>

static void Check(unsigned frames, bool legacy)
{
	HostInfo host = HostInfo();
	host.BUFSIZE = frames;
	host.SAMPLERATE = 48000;
	JackPlugin a, b;
	a.Initialise(&host);
	b.Initialise(&host);
	if (legacy)
	{
		Spiral::Core::Description v2;
		v2.Value(2).Value(4).Value(4);
		Spiral::Core::Description::Reader readA(v2), readB(v2);
		a.Apply(readA);
		b.Apply(readB);
	}
	a.Attach();
	b.Attach();
	assert(clients.size() == (legacy ? 2u : 1u));
	jack_client_t *nativeA = clients.begin()->second;
	jack_client_t *nativeB = clients.rbegin()->second;
	Sample signal(frames);
	a.SetInput(0, &signal);
	b.SetInput(0, &signal);
	AudioTransportHub *hub = AudioTransportHub::Get();
	cycleTimeOffset = Spiral::Core::AudioMonotonicTime();
	unsigned audible = 0, rendered = 0;
	for (cycleFrame = 0; cycleFrame < 6000; cycleFrame += 4)
	{
		nativeA->Process(4, nativeA->ProcessContext);
		if (nativeB != nativeA) nativeB->Process(4, nativeB->ProcessContext);
		if (cycleFrame > 1200)
			for (unsigned n = 0; n < 4; ++n)
			{
				const float first = nativeA->Ports[4]->Buffer[n];
				const float second = nativeB->Ports[legacy ? 4 : 12]->Buffer[n];
				assert(first == second);
				if (std::fabs(first) > 0.01) ++audible;

			}

		const double now = cycleTimeOffset + cycleFrame / 48000.0;
		while (hub->PreparePeriod(now))
		{
			hub->BeginPeriod();
			a.ProcessAudio();
			b.ProcessAudio();
			for (unsigned n = 0; n < frames; ++n)
				signal.Set(n, std::sin((rendered + n) * 0.03));

			a.Execute();
			b.Execute();
			assert(a.GetCaptureFrame() == hub->CaptureStamp().Frame);
			hub->CommitPeriod();
			rendered += frames;
		}

	}

	assert(audible > 4000);
	assert(rendered > 5800 && rendered < 6200);
	printf("%s JACK ports: host %u/native 4, %u aligned changing samples PASS\n", legacy ? "Legacy" : "Shared", frames, audible);
	a.Detach();
	b.Detach();
	hub->SetHost(NULL);
}
static void CheckSavedEndpoint(bool legacy)
{
	using namespace Spiral::Core;
	HostInfo host = HostInfo();
	host.BUFSIZE = 4;
	host.SAMPLERATE = 48000;
	std::string json, error, firstPort;
	{
		JackPlugin original;
		original.Initialise(&host);
		if (legacy)
		{
			Description old;
			old.Value(2).Value(3).Value(2);
			Description::Reader state(old);
			original.Apply(state);
		}
		original.Attach();
		assert(clients.size() == 1);
		firstPort = clients.begin()->second->Ports.front()->Name;
		PatchDescription patch;
		DeviceDescription &device = patch.AddObject().AddDevice();
		device.SetValue("patch_id", "1");
		device.SetValue("id", "31");
		device.SetValue("name", "Jack");
		Description description;
		original.Describe(description);
		Spiral::File::PatchFile::Properties(description, device);
		assert(Spiral::File::PatchFile::Text(patch, json, error));
	}
	assert(clients.empty());
	PatchDescription reopened;
	assert(Spiral::File::PatchFile::ReadText(json, reopened, error));
	const DeviceDescription *device = reopened.FindDevice("1");
	assert(device);
	Description saved;
	Spiral::File::PatchFile::Describe(*device, saved);
	JackPlugin restored;
	restored.Initialise(&host);
	restored.Attach();
	Description::Reader state(saved);
	restored.Apply(state);
	assert(!state.Failed());
	assert(clients.size() == 1 && clients.begin()->second->Ports.front()->Name == firstPort);
	assert(restored.GetInputCount() == (legacy ? 2 : 4));
	assert(restored.GetOutputCount() == (legacy ? 3 : 4));
	if (!legacy)
	{
		jack_port_t *existing = clients.begin()->second->Ports.front();
		failRegister = true;
		restored.SetNumberPorts(6, 7);
		failRegister = false;
		assert(restored.GetInputCount() == 4 && restored.GetOutputCount() == 4);
		assert(clients.begin()->second->Ports.front() == existing);
	}
	restored.Detach();
	AudioTransportHub::Get()->SetHost(NULL);
}

int main()
{
	for (unsigned legacy = 0; legacy < 2; ++legacy)
	{
		CheckSavedEndpoint(legacy);
		Check(4, legacy);
		Check(2, legacy);
		Check(6, legacy);
	}
}
