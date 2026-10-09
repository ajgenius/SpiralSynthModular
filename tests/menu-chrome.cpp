// SPDX-License-Identifier: GPL-2.0-or-later
// On macOS the menu is in the system menu bar at the top of the screen
// whatever the option says. The option is whether the strip inside the window
// shows the same menu as well, and the strip is chrome like the toolbar: when
// it is hidden the toolbar and canvas move up into the row it occupied.
// Everywhere else the strip is the only menu there is, and it may still be
// hidden: the toolbar carries the Options button that brings it back.
//
// The chrome also follows the window's width, which it did not when both bars
// were built at the startup width and the canvas took every extra pixel.
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

	// The chrome spans the window, and keeps spanning it when the window
	// grows. The fixed toolbar widgets keep their size, so the plugin tabs
	// take the whole of what the window gained. The drawer is chrome at
	// the right edge, a rail until it is opened, and the canvas scroll
	// takes what it leaves.
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

	// Opening the drawer takes its width from the canvas, and the two
	// still meet at the edge.
	drawer->SetCollapsed(false);
	synth.LayoutChrome();
	assert(drawer->w() == drawer->ExpandedWidth());
	assert(canvas->w() == Wide - drawer->w());
	assert(canvas->x() + canvas->w() == drawer->x());
	drawer->SetCollapsed(true);
	synth.LayoutChrome();
	assert(canvas->w() == Wide - drawer->RailWidth());

#ifndef __APPLE__
	// About is on the Help menu here, where there is no application menu
	// to carry it. It is modal, so close it from a timeout: the box has
	// to come up and be the modal window, then go away on its own.
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
	// The system menu bar is an Fl_Menu_Bar as well, so say which one this is.
	assert(!dynamic_cast<Fl_Sys_Menu_Bar *>(strip));

	const int MenuHeight = strip->h();
	const int RaisedY = canvas->y();

	// Hidden here by default, with the canvas in the row it would occupy.
	assert(!strip->visible());

	// Both bars carry the menu, whichever of them is on screen.
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
