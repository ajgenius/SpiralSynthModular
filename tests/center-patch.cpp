// SPDX-License-Identifier: GPL-2.0-or-later
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

	Place(canvas, scroll->x() + PageW + 400, scroll->y() + PageH + 300);
	Place(canvas, scroll->x() + PageW + 700, scroll->y() + PageH + 500);

	canvas->CenterPatch();

	{
		const Patch patch(canvas);
		const int GapLeft = patch.Left - scroll->x();
		const int GapRight = (scroll->x() + PageW) - patch.Right;
		const int GapTop = patch.Top - scroll->y();
		const int GapBottom = (scroll->y() + PageH) - patch.Bottom;

		assert(patch.Right - patch.Left < PageW && patch.Bottom - patch.Top < PageH);
		assert(std::abs(GapLeft - GapRight) <= 1);
		assert(std::abs(GapTop - GapBottom) <= 1);
	}

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
		assert(patch.Left < scroll->x() && patch.Top < scroll->y());
	}

	while (canvas->children())
		canvas->remove(canvas->child(0));

	Place(canvas, -918, -2479);
	Place(canvas, 1710, 438);

	canvas->CenterPatch();

	{
		const Patch patch(canvas);
		const int PatchMiddleX = (patch.Left + patch.Right) / 2;
		const int PatchMiddleY = (patch.Top + patch.Bottom) / 2;
		const int ViewMiddleX = scroll->x() + PageW / 2;
		const int ViewMiddleY = scroll->y() + PageH / 2;

		assert(canvas->x() <= scroll->x() && canvas->y() <= scroll->y());
		assert(canvas->x() + canvas->w() >= scroll->x() + PageW);
		assert(canvas->y() + canvas->h() >= scroll->y() + PageH);
		assert(std::abs(PatchMiddleX - ViewMiddleX) <= 1);
		assert(std::abs(PatchMiddleY - ViewMiddleY) <= 1);
	}

	while (canvas->children())
		canvas->remove(canvas->child(0));

	Place(canvas, canvas->x() - 400, canvas->y() - 400);
	Place(canvas, canvas->x() - 200, canvas->y() - 200);

	canvas->CenterPatch();
	assert(canvas->x() == scroll->x() && canvas->y() == scroll->y());

	while (canvas->children())
		canvas->remove(canvas->child(0));

	Place(canvas, canvas->x() + canvas->w() + 200, canvas->y() + canvas->h() + 200);
	Place(canvas, canvas->x() + canvas->w() + 400, canvas->y() + canvas->h() + 400);

	canvas->CenterPatch();
	assert(canvas->x() + canvas->w() == scroll->x() + PageW);
	assert(canvas->y() + canvas->h() == scroll->y() + PageH);

	while (canvas->children())
		canvas->remove(canvas->child(0));

	canvas->ResetView();
	assert(canvas->x() == scroll->x() - canvas->w() / 2);
	assert(canvas->y() == scroll->y() - canvas->h() / 2);
	Place(canvas, 400, 300);
	const int FirstOffsetX = canvas->child(0)->x() - canvas->x();
	const int FirstOffsetY = canvas->child(0)->y() - canvas->y();

	scroll->scroll_to(scroll->xposition() + 4000, scroll->yposition() + 3000);
	canvas->remove(canvas->child(0));
	canvas->ResetView();
	Place(canvas, 400, 300);
	assert(canvas->child(0)->x() - canvas->x() == FirstOffsetX);
	assert(canvas->child(0)->y() - canvas->y() == FirstOffsetY);

	std::puts("A patch is centered on its middle, and the view stays on the canvas");
	return 0;
}
