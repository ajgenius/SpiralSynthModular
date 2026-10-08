// Exercise the host save/load path with real sampler sidecars and a moved package.
// SPDX-License-Identifier: GPL-2.0-or-later
#include "SpiralSynthModular.h"
#include "SpiralInfo.h"
#include "PatchProject.h"
#include <cassert>
#include <cstdio>
#include <cstdlib>
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
	// Plugin root and an existing Tutorial5-MatrixSampler.ssm_files directory.
	if (argc != 3)
		return 77;

	alarm(45);
	std::string error;
	Spumoni::DiskFolder input;
	assert(input.Create("ssm-host-samples-", error));
	std::string source;
	assert(Spumoni::Path::ReadFile(PUBLIC_SAMPLER_SOURCE, source, error));
	assert(input.Write("input.ssm", source, error));
	assert(input.ImportTree(argv[2], "input.ssm_files", error));
	const std::string patch = input.Root() + "/input.ssm";
	const std::string package = input.Root() + "/saved.ssmp";
	const std::string moved = input.Root() + "/moved.ssmp";
	const std::string second = input.Root() + "/again.ssmp";

	SpiralInfo::AUDIOCLIENT = "dummy";
	SpiralInfo::OUTPUTFILE = "default";
	Host host;
	host.Synth.LoadPlugins(argv[1]);
	pthread_t thread;
	assert(!pthread_create(&thread, NULL, Host::Run, &host));
	host.Synth.LoadPatch(patch.c_str());
	host.Synth.SavePatch(package.c_str());
	Spiral::File::Project before(package);
	assert(before.OpenPackage("", error));
	std::string firstSample;
	assert(Spumoni::Path::ReadFile(before.SidecarDirectory() + "PoshSampler3_0.wav", firstSample, error));
	assert(firstSample.size() > 44);
	assert(rename(package.c_str(), moved.c_str()) == 0);
	assert(input.RemoveEntry("input.ssm"));
	assert(input.RemoveEntry("input.ssm_files"));

	host.Synth.LoadPatch(moved.c_str());
	host.Synth.SavePatch(second.c_str());
	Spiral::File::Project after(second);
	assert(after.OpenPackage("", error));
	std::string secondSample;
	assert(Spumoni::Path::ReadFile(after.SidecarDirectory() + "PoshSampler3_0.wav", secondSample, error));
	assert(secondSample == firstSample);
	host.Synth.ClearUp();
	__sync_lock_test_and_set(&host.Stop, 1);
	pthread_join(thread, NULL);
	std::puts("host package sampler relocation OK");
	return 0;
}
