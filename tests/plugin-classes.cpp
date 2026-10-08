// Devices and editors are two kinds on the loader. Editors load first,
// with no device module in the process, since none may need one; every
// editor names a device that exists; a device may have no editor; a
// second scan of the same root adds nothing; Amp makes a device and an
// editor for it.
#include "SpiralSynthModular.h"
#include "DeviceClassRegistry.h"
#include "EditorClassRegistry.h"
#include "SpiralPluginGUI.h"
#include <cassert>
#include <cstdio>

using namespace spiralcore;

static const DeviceClass *Named(const char *name)
{
	const std::vector<DeviceClass*> &devices=DeviceClassRegistry::Get()->Classes();
	for (size_t n=0; n<devices.size(); ++n)
		if (devices[n]->Name==name) return devices[n];

	return NULL;
}

int main(int argc, char **argv)
{
	if (argc != 2) return 77;

	const unsigned editors=EditorClassRegistry::Get()->LoadModules(argv[1]);
	const unsigned devices=DeviceClassRegistry::Get()->LoadModules(argv[1]);
	printf("%u editors, %u devices\n", editors, devices);
	assert(editors>0 && devices>0);
	assert(EditorClassRegistry::Get()->Classes().size()==editors);
	assert(DeviceClassRegistry::Get()->Classes().size()==devices);

	const std::vector<EditorClass*> &editing=EditorClassRegistry::Get()->Classes();
	for (size_t n=0; n<editing.size(); ++n)
	{
		const DeviceClass *device=DeviceClassRegistry::Get()->Find(editing[n]->ForDevice);
		if (!device) printf("editor %s names no device\n", editing[n]->Module.c_str());
		assert(device && editing[n]->Toolkit=="fltk");
		assert(EditorClassRegistry::Get()->Find(device->ID)==editing[n]);
	}

	unsigned alone=0;
	const std::vector<DeviceClass*> &all=DeviceClassRegistry::Get()->Classes();
	for (size_t n=0; n<all.size(); ++n)
		if (!EditorClassRegistry::Get()->Find(all[n]->ID))
		{
			printf("device %s has no editor\n", all[n]->Name.c_str());
			++alone;
		}
	assert(alone==devices-editors);

	// The same root again: every ID is taken, nothing is kept.
	assert(DeviceClassRegistry::Get()->LoadModules(argv[1])==0);
	assert(EditorClassRegistry::Get()->LoadModules(argv[1])==0);
	assert(DeviceClassRegistry::Get()->Classes().size()==devices);

	const DeviceClass *amp=Named("Amp");
	assert(amp && amp->ID==9 && amp->Category=="Amps/Mixers" && amp->Icon);
	const EditorClass *editor=EditorClassRegistry::Get()->Find(amp->ID);
	assert(editor);
	HostInfo info=HostInfo();
	info.BUFSIZE=256;
	info.SAMPLERATE=44100;
	info.GUI_COLOUR=FL_GRAY;
	SpiralPlugin *plugin=amp->CreateInstance();
	assert(plugin);
	plugin->Initialise(&info);
	Fl_Double_Window window(300,300,"plugin classes test");
	SpiralGUIType *gui=editor->CreateEditor(plugin);
	window.end();
	assert(gui);
	delete gui;
	delete plugin;

	// A builtin registers the same struct with no module.
	DeviceClass builtin=*amp;
	builtin.ID=500;
	builtin.Module="";
	assert(DeviceClassRegistry::Get()->Register(builtin));
	assert(!DeviceClassRegistry::Get()->Register(builtin));
	assert(DeviceClassRegistry::Get()->Find(500)->Create==amp->Create);

	EditorClassRegistry::PackUpAndGoHome();
	DeviceClassRegistry::PackUpAndGoHome();
	puts("devices and editors are two kinds on the loader PASS");
	return 0;
}
