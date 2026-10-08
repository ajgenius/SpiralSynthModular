// Verify the native JACK controls remain outside the scrolling port list.
#include "SpiralSynthModular.h"
#include "SpiralPluginGUI.h"
#include <FL/Fl_Scroll.H>
#include <FL/Fl_Pack.H>
#include <FL/fl_draw.H>
#if FL_API_VERSION >= 10300
#include <FL/Fl_Image_Surface.H>
#endif
#include <dlfcn.h>
#include <cassert>
#include <cstdio>

int main(int argc, char **argv)
{
	if (argc != 2) return 77;

	std::string root=argv[1];
	void *dsp=dlopen((root + "/dsp/JackPlugin/JackPlugin_DSP.so").c_str(), RTLD_NOW | RTLD_GLOBAL);
	if (!dsp) { puts(dlerror()); return 1; }

	void *ui=dlopen((root + "/panels/JackPlugin/JackPlugin_GUI.so").c_str(), RTLD_NOW | RTLD_GLOBAL);
	if (!ui) { puts(dlerror()); return 1; }

	SpiralPlugin *(*create)()=(SpiralPlugin *(*)())dlsym(dsp, "SpiralPlugin_CreateInstance");
	SpiralGUIType *(*createGUI)(SpiralPlugin *)=(SpiralGUIType *(*)(SpiralPlugin *))dlsym(ui, "SpiralPlugin_CreateGUI");
	assert(create && createGUI);
	HostInfo info=HostInfo();
	info.BUFSIZE=1024;
	info.SAMPLERATE=48000;
	info.GUI_COLOUR=FL_GRAY;
	SpiralPlugin *plugin=create();
	plugin->Initialise(&info);

	Fl_Double_Window window(245,250,"JACK layout test");
	SpiralGUIType *gui=createGUI(plugin);
	window.end();
	gui->position(10,10);
	window.show();
	Fl::wait(0.2);
	Fl::flush();
	Fl_Scroll *scroll=NULL;
	for (int n=0; n<gui->children(); ++n)
		if (Fl_Scroll *candidate=dynamic_cast<Fl_Scroll *>(gui->child(n))) scroll=candidate;

	assert(scroll && scroll->y()==gui->y()+90);
	for (int n=0; n<gui->children(); ++n)
	{
		Fl_Widget *control=gui->child(n);
		if (control!=scroll) assert(control->y()+control->h()<=scroll->y());

	}

	unsigned packs=0;
	for (int n=0; n<scroll->children(); ++n)
		if (Fl_Pack *pack=dynamic_cast<Fl_Pack *>(scroll->child(n)))
		{
			assert(pack->parent()==scroll);
			assert(pack->y()>=scroll->y());
			++packs;
		}

	assert(packs==2);
#if FL_API_VERSION >= 10300
	Fl_Image_Surface surface(window.w(),window.h());
	surface.set_current();
	surface.draw(&window);
	Fl_RGB_Image *capture=surface.image();
	assert(capture);
	const char *pixels=capture->data()[0];
	FILE *image=fopen("jack-panel.ppm","wb");
	assert(image);
	fprintf(image,"P6\n%d %d\n255\n",window.w(),window.h());
	fwrite(pixels,3,window.w()*window.h(),image);
	fclose(image);
	delete capture;
	Fl_Display_Device::display_device()->set_current();
#endif
	window.hide();
	delete gui;
	delete plugin;
	puts("JACK scroll panel leaves header controls accessible PASS");
	return 0;
}
