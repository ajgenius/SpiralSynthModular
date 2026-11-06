// SPDX-License-Identifier: GPL-2.0-or-later
#include "GUI/Panels/Panel.h"
#include "GUI/Panels/PanelControl.h"
#include "MixerPlugin.h"
#include "JSON.h"
#include <FL/Fl_Button.H>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <dirent.h>
#include <string>
#include <vector>

using namespace SpiralPanels;

namespace
{
	std::string Root()
	{
		const char *src = getenv("SRCDIR");
		return src ? src : ".";
	}

	std::vector<std::string> PanelFiles()
	{
		std::vector<std::string> files;
		const std::string devices = Root() + "/Plugins/Devices";
		DIR *dir = opendir(devices.c_str());
		assert(dir);
		while (dirent *entry = readdir(dir))
		{
			if (entry->d_name[0] == '.')
				continue;
			const std::string path = devices + "/" + entry->d_name + "/prebuilt-ui.json";
			if (FILE *f = fopen(path.c_str(), "r"))
			{
				fclose(f);
				files.push_back(path);
			}
		}
		closedir(dir);
		return files;
	}

	void Block(MixerPlugin &mixer)
	{
		do
		{
			mixer.UpdateChannelHandler();
			mixer.ExecuteCommands();
		} while (mixer.GetChannelHandler()->IsCommandWaiting());
	}

	int Channels(MixerPlugin &mixer)
	{
		int count = 0;
		mixer.GetChannelHandler()->Peek("Channels", 0, &count, sizeof(count));
		return count;
	}
}

int main()
{
	int failures = 0;

	const std::vector<std::string> files = PanelFiles();
	assert(!files.empty());
	for (size_t i = 0; i < files.size(); ++i)
	{
		std::string error;
		Spumoni::JSON *document = Spumoni::ParseJSON(files[i].c_str(), &error);
		if (!document)
		{
			std::printf("FAIL %s: %s\n", files[i].c_str(), error.c_str());
			++failures;
			continue;
		}
		Panel loaded(0, 0, NULL);
		if (!loaded.Build(*document, &error))
		{
			std::printf("FAIL %s: %s\n", files[i].c_str(), error.c_str());
			++failures;
			delete document;
			continue;
		}
		const std::vector<std::string> &reports = loaded.Unsupported();
		bool refused = false;
		for (size_t r = 0; r < reports.size(); ++r)
			if (reports[r].find("unknown property") != std::string::npos)
				std::printf("     %s: %s (no device here to describe it)\n", files[i].c_str(), reports[r].c_str());
			else if (reports[r].find(": ") != std::string::npos)
			{
				std::printf("FAIL %s: %s\n", files[i].c_str(), reports[r].c_str());
				refused = true;
			}
			else
				std::printf("     %s: no control type %s\n", files[i].c_str(), reports[r].c_str());
		if (refused)
			++failures;
		else
			std::printf("ok   %s\n", files[i].c_str());
		delete document;
	}

	{
		MixerPlugin mixer;
		std::string error;
		Spumoni::JSON *document = Spumoni::ParseJSON((Root() + "/Plugins/Devices/MixerPlugin/prebuilt-ui.json").c_str(), &error);
		assert(document);
		Panel loaded(0, 0, &mixer);
		if (!loaded.Build(*document, &error))
			std::printf("mixer: %s\n", error.c_str());
		assert(error.empty());
		loaded.Refresh();
		const int before = Channels(mixer);
		PanelControl *add = loaded.FindControl("add");
		PanelControl *remove = loaded.FindControl("remove");
		assert(add && remove && add->Widget() && remove->Widget());
		assert(add->Handles("pressed") && remove->Handles("pressed"));

		add->Widget()->do_callback();
		Block(mixer);
		if (Channels(mixer) != before + 1)
		{
			std::printf("FAIL mixer add: channels %d -> %d\n", before, Channels(mixer));
			++failures;
		}
		else
			std::printf("ok   mixer add: channels %d -> %d\n", before, Channels(mixer));

		remove->Widget()->do_callback();
		Block(mixer);
		if (Channels(mixer) != before)
		{
			std::printf("FAIL mixer remove: channels back to %d, not %d\n", Channels(mixer), before);
			++failures;
		}
		else
			std::printf("ok   mixer remove: channels back to %d\n", before);
		delete document;
	}

	std::printf("%d failures\n", failures);
	return failures ? 1 : 0;
}
