// SPDX-License-Identifier: GPL-2.0-or-later
#include "SpiralSynthModular.h"
#include "SpiralInfo.h"
#include <FL/Fl_Pack.H>
#include <FL/Fl_Tabs.H>
#include <FL/Fl_Scroll.H>
#include <cassert>
#include <cstdio>
#include <cstring>

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

int main()
{
	SpiralInfo::AUDIOCLIENT = "dummy";

	SynthModular synth;
	Fl_Widget *window = synth.CreateWindow();
	Fl_Menu_Bar *strip = Find<Fl_Menu_Bar>(window);
	Fl_Scroll *canvas = Find<Fl_Scroll>(window);
	Fl_Pack *toolbar = Find<Fl_Pack>(window);
	Fl_Tabs *tabs = Find<Fl_Tabs>(window);
	Fl_BoundaryDrawer *drawer = Find<Fl_BoundaryDrawer>(window);

	assert(strip && canvas && toolbar && tabs && drawer);

	const int TabsBefore = tabs->w();
	const int Before = window->w();
	const int Wide = Before + 240;
	window->size(Wide, window->h());

	std::printf("window %d: strip %d toolbar %d canvas %d drawer %d tabs %d (was %d)\n",
	            Wide, strip->w(), toolbar->w(), canvas->w(), drawer->w(), tabs->w(), TabsBefore);

	assert(strip->w() == Wide);
	assert(toolbar->w() == Wide);
	assert(drawer->Collapsed());
	assert(drawer->w() == drawer->RailWidth());
	assert(drawer->x() + drawer->w() == Wide);
	assert(canvas->w() == Wide - drawer->w());
	assert(canvas->y() == drawer->y());
	assert(tabs->w() == TabsBefore + (Wide - Before));

	drawer->SetCollapsed(false);
	synth.LayoutChrome();
	assert(drawer->w() == drawer->ExpandedWidth());
	assert(canvas->w() == Wide - drawer->w());
	assert(canvas->x() + canvas->w() == drawer->x());
	drawer->SetCollapsed(true);
	synth.LayoutChrome();
	assert(canvas->w() == Wide - drawer->RailWidth());

#ifndef __APPLE__
	const Fl_Menu_Item *about = strip->find_item("Help/About");
	assert(about);

	struct Closer
	{
		static void Run(void *)
		{
			Fl_Window *box = Fl::modal();
			assert(box && box->shown());
			assert(std::strcmp(box->label(), "About SpiralSynthModular") == 0);
			box->hide();
		}
	};
	Fl::add_timeout(0.1, Closer::Run);
	about->do_callback(strip, about->user_data());
	assert(!Fl::modal());

	std::puts("No system menu bar on this platform; the strip is the only menu, and the chrome follows the width");
	return 0;
#else
	assert(!dynamic_cast<Fl_Sys_Menu_Bar *>(strip));

	const int MenuHeight = strip->h();
	const int RaisedY = canvas->y();

	assert(!strip->visible());

	assert(strip->find_item("Edit/Options"));
	assert(fl_sys_menu_bar && fl_sys_menu_bar->find_item("Edit/Options"));

	SpiralInfo::SHOWMENUBAR = true;
	synth.ApplyViewOptions();
	assert(strip->visible());
	assert(canvas->y() == RaisedY + MenuHeight);
	assert(fl_sys_menu_bar->find_item("Edit/Options"));

	SpiralInfo::SHOWMENUBAR = false;
	synth.ApplyViewOptions();
	assert(!strip->visible());
	assert(canvas->y() == RaisedY);
	assert(strip->find_item("Edit/Options"));
	assert(fl_sys_menu_bar->find_item("Edit/Options"));

	std::puts("The Mac menu bar keeps the menu; the strip is the option, and the canvas takes its row");
	return 0;
#endif
}
