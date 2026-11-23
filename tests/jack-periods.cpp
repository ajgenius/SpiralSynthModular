#define JACK_CLIENT_TEST_FIXTURE
#include "../libs/libspiralcore/Audio/tests/jack-client.cpp"
#include "JackPlugin.h"
#include "OutputPlugin.h"
#include "AudioBackend.h"
#include "PatchFile.h"
#include "AudioTransportHub.h"
#include <cmath>
#include <cstdio>
#include <unistd.h>

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
	Description::Reader state(saved);
	restored.Apply(state);
	restored.ServiceAudio();
	AudioTransportHub::ServiceConnections();
	restored.ServiceAudio();
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
	if (legacy)
	{
		JackPlugin copy;
		copy.Initialise(&host);
		Description::Reader pasted(saved);
		copy.Apply(pasted);
		copy.ServiceAudio();
		AudioTransportHub::ServiceConnections();
		copy.ServiceAudio();
		assert(clients.size() == 2 && copy.GetJackClient()->IsAttached());
	}
	restored.Detach();
	AudioTransportHub::Get()->SetHost(NULL);
}

static void *FixtureJack(void *) { return static_cast<Spiral::Core::AudioClient *>(new JackClient); }
static void DropFixtureJack(void *client) { delete static_cast<Spiral::Core::AudioClient *>(client); }

static const Spiral::Core::BackendDescriptor *RegisteredJack()
{
	return Spiral::Core::AudioBackendRegistry::Get()->Find("jack");
}

static jack_port_t *NamedPort(jack_client_t *client, const char *name)
{
	for (size_t n = 0; n < client->Ports.size(); ++n)
		if (client->Ports[n]->Name == client->Name + ":" + name)
			return client->Ports[n];
	return NULL;
}

static void CheckStereoMode()
{
	using namespace Spiral::Core;
	AudioBackendRegistry *registry = AudioBackendRegistry::Get();
	if (registry->Find("jack")) registry->Release(reinterpret_cast<void *>(&RegisteredJack), "");
	static const BackendDescriptor backend = {SPIRAL_AUDIO_PLUGIN_ABI, "audio", "jack", FixtureJack, DropFixtureJack};
	registry->Register(&backend);

	HostInfo host = HostInfo();
	host.AUDIOCLIENT = "jack";
	host.OUTPUTFILE = "default";
	host.SAMPLERATE = 48000;
	host.BUFSIZE = 64;
	OutputPlugin stereo;
	JackPlugin stem;
	stereo.Initialise(&host);
	stereo.ServiceAudio();
	AudioTransportHub::ServiceConnections();
	stereo.ServiceAudio();
	stem.Initialise(&host);
	stem.Attach();
	assert(clients.size() == 1);

	jack_client_t *server = clients.begin()->second;
	jack_port_t *left = NamedPort(server, "Out0");
	assert(left);
	jack_connect(server, left->Name.c_str(), "monitor:left");
	AudioTransportHub *hub = AudioTransportHub::Get();
	failRegister = true;
	hub->OpenMode(AudioTransportHub::DUPLEX);
	failRegister = false;
	assert(hub->GetMode() == AudioTransportHub::OUTPUT && left == NamedPort(server, "Out0"));
	hub->OpenMode(AudioTransportHub::DUPLEX);
	assert(hub->GetMode() == AudioTransportHub::DUPLEX && left == NamedPort(server, "Out0"));
	assert(left->Connection == "monitor:left");
	assert(NamedPort(server, "In1"));
	hub->OpenMode(AudioTransportHub::OUTPUT);
	assert(!NamedPort(server, "In1") && left == NamedPort(server, "Out0"));
	assert(stem.GetJackClient()->IsAttached());
	server->Shutdown(server->ShutdownContext);
	stem.ServiceAudio();
	stereo.ServiceAudio();
	usleep(1100000);
	stem.ServiceAudio();
	stereo.ServiceAudio();
	AudioTransportHub::ServiceConnections();
	stem.ServiceAudio();
	stereo.ServiceAudio();
	assert(stem.GetJackClient()->IsAttached() && hub->GetMode() == AudioTransportHub::OUTPUT);
	stem.Detach();
	stereo.Kill();
	hub->SetHost(NULL);
}

int main()
{
	CheckStereoMode();
	for (unsigned legacy = 0; legacy < 2; ++legacy)
	{
		CheckSavedEndpoint(legacy);
		Check(4, legacy);
		Check(2, legacy);
		Check(6, legacy);
	}
}
