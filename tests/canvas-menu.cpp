// SPDX-License-Identifier: GPL-2.0-or-later
#include "Fl_Canvas.h"
#include <cassert>

int main()
{
	Fl_Group::current(NULL);
	Fl_Canvas *canvas = new Fl_Canvas(0, 0, 100, 100, NULL);
	canvas->end();
	Fl_Group *other = new Fl_Group(0, 0, 100, 100);
	canvas->AddPluginName("Test", 1);
	other->end();
	assert(canvas->m_Menu && !canvas->m_Menu->parent());
	delete other;
	delete canvas;
	return 0;
}
