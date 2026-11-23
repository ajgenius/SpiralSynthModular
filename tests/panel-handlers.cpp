// SPDX-License-Identifier: GPL-2.0-or-later
#include "libs/libSpiralPanels/include/Panel.h"
#include "libs/libSpiralPanels/include/PanelControl.h"
#include "DeviceSource.h"
#include "FltkView.h"
#include "MixerPlugin.h"
#include <config.h>
#ifdef HAVE_JACK_CLIENT
#include "libs/libspiralcore/Devices/Jack/JackPlugin.h"
#include "libs/libSpiralPanels/include/Patch.h"
#endif
#include "JSON.h"
#include <FL/Fl_Button.H>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <dirent.h>
#include <string>
#include <vector>

using namespace Spiral::Panels;

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
			const std::string path = devices + "/" + entry->d_name + "/info.json";
			Spumoni::JSONOwner info(Spumoni::ParseJSON(path.c_str()));
			if (info.get() && info->Get("ui"))
				files.push_back(path);
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

static Spumoni::JSON *UI(const std::string &path, std::string &error)
{
	Spumoni::JSONOwner info(Spumoni::ParseJSON(path.c_str(), &error));
	const Spumoni::JSON *ui = info.get() ? info->Get("ui") : NULL;

	if (info.get() && !ui)
		error = "no ui";

	return ui ? ui->Duplicate() : NULL;
}

#ifdef HAVE_JACK_CLIENT
class JackPanelFixture : public JackPlugin
{
public:
	void Publish()
	{
		static_cast<Spiral::Core::ToggleProperty *>(m_State.Properties["CONNECTED"])->Value.AsBoolean = true;
		static_cast<Spiral::Core::SignedProperty *>(m_State.Properties["INPUTS"])->Value.AsSigned = 3;
		static_cast<Spiral::Core::SignedProperty *>(m_State.Properties["OUTPUTS"])->Value.AsSigned = 7;
		GetChannelHandler()->FlushChannels();
	}
};

class RouteSource : public DeviceSource
{
public:
	explicit RouteSource(JackPlugin *device) : DeviceSource(device) {}
	std::string last;
	std::vector<std::string> arguments;
	bool Command(const std::string &name, const std::vector<std::string> &args)
	{
		last = name;
		arguments = args;
		return true;
	}
};

class RouteDialog : public Dialogs
{
public:
	int selection;
	int ChooseFrom(const std::vector<std::string> &labels)
	{
		assert(labels.size() == 2 && labels[0] == "Remove capture:mic" && labels[1] == "Add capture:line");
		return selection;
	}
	void ChooseFile(bool, const std::string &, const std::string &, Completed, void *) {}
};

static void CheckJackPanel()
{
	JackPanelFixture device;
	device.Publish();
	RouteSource *source = new RouteSource(&device);
	FltkPanel host(0, 0, source);
	Panel &panel = host.Model();
	std::string error;
	Spumoni::JSONOwner document(UI(Root() + "/libs/libspiralcore/Devices/Jack/info.json", error));
	assert(document.get() && panel.Build(*document, &error));
	panel.Refresh();
	PortListControl *ports = dynamic_cast<PortListControl *>(panel.FindControl("ports"));
	PortsView *view = ports ? dynamic_cast<PortsView *>(ports->View()) : NULL;
	assert(view && view->PortCount(0) == 3 && view->PortCount(1) == 7);
	Dialogs *original = Dialogs::Get();
	RouteDialog dialog;
	Dialogs::Set(&dialog);
	for (int choice = -1; choice <= 1; ++choice)
	{
		dialog.selection = choice;
		ports->Clicked(1, 6, false);
		assert(source->last == "LIST_ROUTES" && source->arguments[0] == "-7");
		std::string bytes = "-7 " + source->arguments[1];
		const char options[] = "\0-capture:mic\0+capture:line\0";
		bytes.append(options, sizeof(options) - 1);
		device.GetChannelHandler()->PublishBytes("RouteChoices", bytes);
		device.UpdateChannelHandler();
		panel.Refresh();
		if (choice < 0) assert(source->last == "LIST_ROUTES");
		else
		{
			assert(source->last == (choice ? "ADD_ROUTE" : "REMOVE_ROUTE"));
			assert(source->arguments[0] == "-7" && source->arguments[2] == (choice ? "capture:line" : "capture:mic"));
		}
	}
	Dialogs::Set(original);
}
#endif

int main()
{
#ifdef HAVE_JACK_CLIENT
	CheckJackPanel();
#endif
	int failures = 0;

	const std::vector<std::string> files = PanelFiles();
	assert(!files.empty());
	for (size_t i = 0; i < files.size(); ++i)
	{
		std::string error;
		Spumoni::JSON *document = UI(files[i], error);
		if (!document)
		{
			std::printf("FAIL %s: %s\n", files[i].c_str(), error.c_str());
			++failures;
			continue;
		}
		FltkPanel host(0, 0, new DeviceSource(NULL));
		Panel &loaded = host.Model();
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
		Spumoni::JSON *document = UI(Root() + "/Plugins/Devices/MixerPlugin/info.json", error);
		assert(document);
		FltkPanel host(0, 0, new DeviceSource(&mixer));
		Panel &loaded = host.Model();
		if (!loaded.Build(*document, &error))
			std::printf("mixer: %s\n", error.c_str());
		assert(error.empty());
		loaded.Refresh();
		const int before = Channels(mixer);
		PanelControl *add = loaded.FindControl("add");
		PanelControl *remove = loaded.FindControl("remove");
		assert(add && remove && FltkView::WidgetOf(*add) && FltkView::WidgetOf(*remove));
		assert(add->Handles("pressed") && remove->Handles("pressed"));

		FltkView::WidgetOf(*add)->do_callback();
		Block(mixer);
		if (Channels(mixer) != before + 1)
		{
			std::printf("FAIL mixer add: channels %d -> %d\n", before, Channels(mixer));
			++failures;
		}
		else
			std::printf("ok   mixer add: channels %d -> %d\n", before, Channels(mixer));

		FltkView::WidgetOf(*remove)->do_callback();
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
