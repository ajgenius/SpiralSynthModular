// SPDX-License-Identifier: GPL-2.0-or-later
// Centering moves the view, never the devices. A patch smaller than the view
// ends up with equal margins; one larger than the view keeps its top left
// corner on screen behind a 32px margin.
//
// The geometry does not care what the children are, so plain boxes stand in
// for devices and the test needs no plugins.
#include "SpiralSynthModular.h"
#include "SpiralInfo.h"
#include <FL/Fl_Box.H>
#include <cassert>
#include <cstdio>
#include <cstdlib>

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

static void Place(Fl_Canvas *canvas, int x, int y)
{
	Fl_Group::current(0);
	canvas->add(new Fl_Box(x, y, 120, 80, ""));
}

struct Patch
{
	int Left, Top, Right, Bottom;

	Patch(Fl_Canvas *canvas)
	{
		Left = canvas->child(0)->x();
		Top = canvas->child(0)->y();
		Right = Left + canvas->child(0)->w();
		Bottom = Top + canvas->child(0)->h();

		for (int n = 1; n < canvas->children(); ++n)
		{
			Fl_Widget *o = canvas->child(n);

			if (o->x() < Left) Left = o->x();
			if (o->y() < Top) Top = o->y();
			if (o->x() + o->w() > Right) Right = o->x() + o->w();
			if (o->y() + o->h() > Bottom) Bottom = o->y() + o->h();
		}
	}
};

int main()
{
	SpiralInfo::AUDIOCLIENT = "dummy";

	SynthModular synth;
	Fl_Widget *window = synth.CreateWindow();
	Fl_Canvas *canvas = Find<Fl_Canvas>(window);
	Fl_Scroll *scroll = Find<Fl_Scroll>(window);

	assert(canvas && scroll);

	const int PageW = scroll->w() - scroll->scrollbar.w();
	const int PageH = scroll->h() - scroll->hscrollbar.h();

	// A patch saved a page and a half away from where the view is now.
	Place(canvas, scroll->x() + PageW + 400, scroll->y() + PageH + 300);
	Place(canvas, scroll->x() + PageW + 700, scroll->y() + PageH + 500);

	canvas->CenterPatch();

	{
		const Patch patch(canvas);
		const int Before = patch.Left - scroll->x();
		const int After = (scroll->x() + PageW) - patch.Right;
		const int Above = patch.Top - scroll->y();
		const int Below = (scroll->y() + PageH) - patch.Bottom;

		assert(patch.Right - patch.Left < PageW && patch.Bottom - patch.Top < PageH);
		assert(std::abs(Before - After) <= 1);
		assert(std::abs(Above - Below) <= 1);
	}

	// Grow it past the view: the top left corner is what has to stay visible.
	{
		const Patch patch(canvas);
		Place(canvas, patch.Left + PageW + 200, patch.Top + PageH + 200);
	}

	canvas->CenterPatch();

	{
		const Patch patch(canvas);

		assert(patch.Right - patch.Left > PageW && patch.Bottom - patch.Top > PageH);
		assert(patch.Left - scroll->x() == 32);
		assert(patch.Top - scroll->y() == 32);
	}

	std::puts("A small patch is centered in the view; a large one keeps its top left corner");
	return 0;
}
