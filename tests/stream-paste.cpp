// SPDX-License-Identifier: GPL-2.0-or-later
// Exercise the actual Copy/Paste menu callbacks while the audio loop runs.
#include "SpiralSynthModular.h"
#include "SpiralInfo.h"
#include "StreamPlugin.h"
#include "DeviceClassRegistry.h"
#include <cassert>
#include <cstdio>
#include <pthread.h>
#include <sstream>
#include <unistd.h>

template<class T> static T *Find(Fl_Widget *widget)
{
	if (T *found = dynamic_cast<T *>(widget))
		return found;

	Fl_Group *group = dynamic_cast<Fl_Group *>(widget);
	if (group)
		for (int i = 0; i < group->children(); ++i)
			if (T *found = Find<T>(group->child(i)))
				return found;

	return NULL;
}

// Slow file/sidecar work makes the publication window deterministic. The
// actual Stream Apply runs first; the live audio loop must not touch it yet.
class SlowStream : public StreamPlugin
{
public:
	virtual void Apply(spiralcore::Description::Reader &reader)
	{
		StreamPlugin::Apply(reader);
		usleep(30000);
	}
};

static SpiralPlugin *CreateStream() { return new SlowStream; }
static const char *icon[] = { "1 1 1 1", "a c #000000", "a" };

struct Host
{
	SynthModular Synth;
	volatile int Stop;
	Host() : Stop(0) {}
	static void *Run(void *context)
	{
		Host *host = static_cast<Host *>(context);
		while (!__sync_fetch_and_add(&host->Stop, 0))
			host->Synth.Update();

		return NULL;
	}
};

static void Check(SynthModular &synth, const std::string &path, unsigned expected)
{
	spiralcore::Description description;
	Describe(description, synth);
	unsigned count = 0;
	for (size_t i = 3; i < description.Values().size(); ++i)
	{
		if (description.Values()[i] != path)
			continue;

		assert(description.Values()[i-3] == "1.021");
		assert(description.Values()[i-2] == "2");
		++count;
	}

	assert(count == expected);
}

int main(int argc, char **argv)
{
	if (argc != 3)
		return 77;

	alarm(60);
	SpiralInfo::AUDIOCLIENT = "dummy";
	SpiralInfo::OUTPUTFILE = "default";
	Host host;
	Fl_Widget *window = host.Synth.CreateWindow();
	spiralcore::DeviceClass stream;
	stream.ID = 281;
	stream.Name = "Stream";
	stream.Icon = icon;
	stream.Create = CreateStream;
	assert(spiralcore::DeviceClassRegistry::Get()->Register(stream));
	host.Synth.LoadPlugins(argv[1]);
	pthread_t thread;
	assert(!pthread_create(&thread, NULL, Host::Run, &host));
	std::string path(argv[2]);
	std::ostringstream source;
	source << "SpiralSynthModular File Ver 4\n0 0 700 600 0 0 0 0\nSectionList 2\n"
		"Device 0 Plugin 0\n0 0 6 Output 0 0 0\n"
		"Device 1 Plugin 281\n20 20 6 Stream 0 0 0\n1 1.021 2 "
		<< path.size() << " " << path << " 0 0 0 1\n-1 0 0\n";
	std::stringstream input(source.str());
	host.Synth.StreamPatchIn(input, false, false);
	Check(host.Synth, path, 1);
	Fl_Canvas *canvas = Find<Fl_Canvas>(window);
	Fl_Menu_Bar *menu = Find<Fl_Menu_Bar>(window);
	assert(canvas && menu);
	Fl_Canvas::AppendSelection(1, canvas);
	menu->find_item("Edit/Copy")->do_callback(menu, &host.Synth);
	for (unsigned i = 0; i < 20; ++i)
	{
		if (i == 10) host.Synth.PauseAudio();
		menu->find_item("Edit/Paste")->do_callback(menu, &host.Synth);
		usleep(5000);
		Check(host.Synth, path, i + 2);
	}

	std::stringstream merge(source.str());
	host.Synth.StreamPatchIn(merge, false, true);
	usleep(5000);
	Check(host.Synth, path, 22);
	host.Synth.ClearUp();
	__sync_lock_test_and_set(&host.Stop, 1);
	pthread_join(thread, NULL);
	remove("___temp.ssmcopytmp");
	std::puts("Stream filename, gain and pitch survived live and paused copy/paste and merge");
}
