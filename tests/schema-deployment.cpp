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
	host.Synth.LoadPlugins(plugins.empty() ? SSMBundlePluginPath() : plugins);
	pthread_t thread;
	assert(!pthread_create(&thread, NULL, Host::Run, &host));
	host.Synth.LoadPatch(argv[2]);
	spiralcore::Description result;
	Describe(result, host.Synth);
	bool found = false;
	for (size_t i = 0; i + 3 < result.Values().size(); ++i)
	{
		if (result.Values()[i] != "HistoryControl")
			continue;

		assert(result.Values()[i+1] == "-2" && result.Values()[i+2] == "2");
		assert(result.Values()[i+3] == "0.375");
		found = true;
	}

	assert(found);
	host.Synth.SavePatch(argv[3]);
	host.Synth.ClearUp();
	__sync_lock_test_and_set(&host.Stop, 1);
	pthread_join(thread, NULL);
	std::puts("Deployed historical Controller load/save passed");
}
