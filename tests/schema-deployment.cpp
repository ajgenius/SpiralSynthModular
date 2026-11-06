// SPDX-License-Identifier: GPL-2.0-or-later
#include "SpiralSynthModular.h"
#include "SpiralInfo.h"
#include "MacBundle.h"
#include <cassert>
#include <cstdio>
#include <pthread.h>
#include <unistd.h>

struct Host
{
	SynthModular Synth;
	volatile int Stop;
	Host() : Stop(0) { Synth.CreateWindow(); }
	static void *Run(void *context)
	{
		Host *host = static_cast<Host *>(context);
		while (!__sync_fetch_and_add(&host->Stop, 0))
			host->Synth.Update();

		return NULL;
	}
};

int main(int argc, char **argv)
{
	if (argc != 4)
		return 77;

	alarm(30);
	SpiralInfo::AUDIOCLIENT = "dummy";
	SpiralInfo::OUTPUTFILE = "default";
	Host host;
	std::string plugins = argv[1];
	host.Synth.LoadPlugins(plugins.empty() ? SpiralBundlePluginPath() : plugins);
	pthread_t thread;
	assert(!pthread_create(&thread, NULL, Host::Run, &host));
	host.Synth.LoadPatch(argv[2]);
	Spiral::Core::PatchDescription patch;
	Describe(patch, host.Synth, "");
	bool found = false;
	for (size_t o = 0; o < patch.Objects.size(); ++o)
	{
		for (size_t d = 0; d < patch.Objects[o].Devices.size(); ++d)
		{
			const Spiral::Core::DeviceDescription &device = patch.Objects[o].Devices[d];
			const Spiral::Core::PropertyDescription *names = device.FindProperty("NAMES");
			if (!names || names->Children.empty() || names->Children[0].Value() != "HistoryControl")
				continue;

			assert(device.FindProperty("MINVAL")->Children[0].Value() == "-2");
			assert(device.FindProperty("MAXVAL")->Children[0].Value() == "2");
			assert(device.FindProperty("CHANNELVAL")->Children[0].Value() == "0.375");
			found = true;
		}
	}

	assert(found);
	host.Synth.SavePatch(argv[3]);
	host.Synth.ClearUp();
	__sync_lock_test_and_set(&host.Stop, 1);
	pthread_join(thread, NULL);
	std::puts("Deployed historical Controller load/save passed");
}
