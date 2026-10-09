// SPDX-License-Identifier: GPL-2.0-or-later
// Centering moves the view, never the devices, and it looks at the middle of
// the patch whatever its size. A patch smaller than the view ends up with
// equal margins; one larger than the view puts its middle in the middle of
// the view, which is the case that matters because real patches are bigger
// than the window.
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
		// Not Above and Below: Xlib defines both as macros, and FLTK's
		// headers bring X11 in on Linux.
		const int GapLeft = patch.Left - scroll->x();
		const int GapRight = (scroll->x() + PageW) - patch.Right;
		const int GapTop = patch.Top - scroll->y();
		const int GapBottom = (scroll->y() + PageH) - patch.Bottom;

		assert(patch.Right - patch.Left < PageW && patch.Bottom - patch.Top < PageH);
		assert(std::abs(GapLeft - GapRight) <= 1);
		assert(std::abs(GapTop - GapBottom) <= 1);
	}

	// Grow it well past the view, the shape of a real song: song50-3 spans
	// about 2600x2900 against a 700x644 window. Anchoring the corner would
	// leave the view on the topmost devices with the rest below it.
	{
		const Patch patch(canvas);
		Place(canvas, patch.Left + 4 * PageW, patch.Top + 4 * PageH);
	}

	canvas->CenterPatch();

	{
		const Patch patch(canvas);
		const int PatchMiddleX = (patch.Left + patch.Right) / 2;
		const int PatchMiddleY = (patch.Top + patch.Bottom) / 2;
		const int ViewMiddleX = scroll->x() + PageW / 2;
		const int ViewMiddleY = scroll->y() + PageH / 2;

		assert(patch.Right - patch.Left > PageW && patch.Bottom - patch.Top > PageH);
		assert(std::abs(PatchMiddleX - ViewMiddleX) <= 1);
		assert(std::abs(PatchMiddleY - ViewMiddleY) <= 1);
		// The devices that used to fill the view are now off it, above and
		// to the left, which is the whole point of the change.
		assert(patch.Left < scroll->x() && patch.Top < scroll->y());
	}

	std::puts("A patch is centered on its middle whether or not it fits in the view");
	return 0;
}
