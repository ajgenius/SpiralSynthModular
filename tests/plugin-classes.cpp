#include "SpiralSynthModular.h"
#include "DeviceClassRegistry.h"
#include "PanelRegistry.h"
#include "SpiralPluginGUI.h"
#include "OutputPlugin.h"
#include "PanelWindow.h"
#include "libs/libSpiralPanels/include/Panel.h"
#include "PatchProject.h"
#include "PatchFile.h"
#include <cassert>
#include <cstdio>

using namespace Spiral::Core;

static const DeviceClass *Named(const char *name)
{
	const std::vector<DeviceClass*> &devices=DeviceClassRegistry::Get()->Classes();
	for (size_t n=0; n<devices.size(); ++n)
		if (devices[n]->Name==name) return devices[n];

	return NULL;
}

static void OpenRecordedRoll(const char *path, HostInfo &host)
{
	std::string error;
	Spiral::File::Project project(path);
	PatchDescription patch;
	assert(project.OpenPackage("", error));
	assert(Spiral::File::PatchFile::ReadText(project.Patch().Bytes(), patch, error));
	const DeviceDescription *device = patch.FindDevice("1");
	assert(device);
	Description description;
	Spiral::File::PatchFile::Describe(*device, description);
	const DeviceClass *type = Named("PianoRoll");
	assert(type);
	SpiralPlugin *roll = type->CreateInstance();
	assert(roll);
	roll->Initialise(&host);
	Description::Reader saved(description);
	roll->Apply(saved);
	assert(!saved.Failed());
	roll->UpdateChannelHandler();
	Spiral::Panels::PanelWindow *window = Spiral::Panels::PanelWindow::Create(roll, *type);
	assert(window && window->GetPanel());
	Spiral::Panels::Panel &panel = *window->GetPanel();
	panel.Refresh();
	Spiral::Data::RowSource *rows = panel.Rows("notes");
	assert(rows && rows->Count() == 18000 && rows->FieldCount() == 6);
	Spiral::Data::ControlValue key, end;
	assert(rows->GetAt(17999, rows->FindField("key"), key));
	assert(rows->GetAt(17999, rows->FindField("end"), end));
	assert(key.number == 71 && end.number > 899 && end.number < 900);
	assert(panel.FindProperty("RECORD STATUS") && panel.FindControl("record-status"));
	const unsigned long seen = rows->TableRevision();
	panel.Refresh();
	assert(rows->TableRevision() == seen);
	assert(panel.Command("CLEAR"));
	roll->UpdateChannelHandler();
	roll->ExecuteCommands();
	roll->UpdateChannelHandler();
	panel.Refresh();
	assert(rows->Count() == 0 && rows->TableRevision() != seen);
	delete window;
	delete roll;
	std::puts("Installed Piano Roll panel shows all 18000 rows, reads the final release and clears to zero");
}

int main(int argc, char **argv)
{
	if (argc < 2 || argc > 3) return 77;

	Spiral::Panels::PanelWindow::Provide();
	const unsigned devices=DeviceClassRegistry::Get()->LoadModules(argv[1]);
	printf("%u devices\n", devices);
	assert(devices>1);
	assert(DeviceClassRegistry::Get()->Classes().size()==devices);
	const DeviceClass *output=DeviceClassRegistry::Get()->Find(0);
	assert(output && output->Name=="Output" && output->Module.empty() && !output->Panel.empty());
	assert(PanelRegistry::Get()->Find(0));
	assert(!Named("Output") || Named("Output")==output);

	const std::vector<PanelClass*> &panels=PanelRegistry::Get()->Classes();
	for (size_t n=0; n<panels.size(); ++n)
	{
		const DeviceClass *device=DeviceClassRegistry::Get()->Find(panels[n]->ForDevice);
		assert(device && panels[n]->Toolkit=="fltk" && (!device->Panel.empty() || device->PanelGenerated));
		assert(PanelRegistry::Get()->Find(device->ID)==panels[n]);
	}

	const std::vector<DeviceClass*> &all=DeviceClassRegistry::Get()->Classes();
	for (size_t n=0; n<all.size(); ++n)
		if (!PanelRegistry::Get()->Find(all[n]->ID))
		{
			printf("device %s has no panel\n", all[n]->Name.c_str());
			assert(all[n]->Panel.empty() && !all[n]->PanelGenerated);
		}

	assert(DeviceClassRegistry::Get()->LoadModules(argv[1])==0);
	assert(DeviceClassRegistry::Get()->Classes().size()==devices);

	const DeviceClass *amp=Named("Amp");
	assert(amp && amp->ID==9 && amp->Category=="Amps/Mixers" && amp->Icon);
	const PanelClass *panel=PanelRegistry::Get()->Find(amp->ID);
	assert(panel);
	HostInfo info=HostInfo();
	info.BUFSIZE=256;
	info.SAMPLERATE=44100;
	info.GUI_COLOUR=FL_GRAY;
	SpiralPlugin *plugin=amp->CreateInstance();
	assert(plugin);
	plugin->Initialise(&info);
	Fl_Double_Window window(300,300,"plugin classes test");
	SpiralGUIType *gui=panel->Open(plugin);
	window.end();
	assert(gui);
	delete gui;
	delete plugin;

	const DeviceClass *clock = DeviceClassRegistry::Get()->Find(55);
	assert(clock && clock->Name == "Clock");
	plugin = clock->CreateInstance();
	assert(plugin);
	plugin->Initialise(&info);
	panel = PanelRegistry::Get()->Find(55);
	assert(panel);
	window.begin();
	gui = panel->Open(plugin);
	window.end();
	assert(gui);
	delete gui;
	delete plugin;

	if (argc == 3)
		OpenRecordedRoll(argv[2], info);

	DeviceClass builtin=*amp;
	builtin.ID=500;
	builtin.Module="";
	assert(DeviceClassRegistry::Get()->Register(builtin));
	assert(!DeviceClassRegistry::Get()->Register(builtin));
	assert(DeviceClassRegistry::Get()->Find(500)->Create==amp->Create);

	PanelRegistry::PackUpAndGoHome();
	DeviceClassRegistry::PackUpAndGoHome();
	puts("every device with a panel opens it PASS");
	return 0;
}
