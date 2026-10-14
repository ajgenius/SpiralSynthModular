// SPDX-License-Identifier: GPL-2.0-or-later
/*  SpiralSynthModular
 *  Copyright (C) 2026 ajgenius
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
// Written by GROK (Grok Bot).
#include "Fl_VirtualScrollArea.H"
#include <FL/Fl.H>
#include <FL/Fl_Box.H>

Fl_VirtualScrollArea::Fl_VirtualScrollArea(int x, int y, int w, int h, const char *label):
	Fl_Scroll(x, y, w, h, label),
	m_Horizontal(true),
	m_Vertical(true),
	m_Always(false),
	m_Extent(NULL)
{
	box(FL_NO_BOX);
	Show(true, true);
}

// Fl_Scroll moves what is on screen by copying it, which takes along
// whatever is drawn over the area, and on macOS copies the window as last
// shown rather than as now being drawn. These areas are small: they are
// drawn again whole instead.
void Fl_VirtualScrollArea::draw()
{
	if (damage() & FL_DAMAGE_SCROLL)
		clear_damage(damage() | FL_DAMAGE_ALL);
	Fl_Scroll::draw();
}

void Fl_VirtualScrollArea::Show(bool horizontal, bool vertical, bool always)
{
	m_Horizontal = horizontal;
	m_Vertical = vertical;
	m_Always = always;
	unsigned char t = 0;
	if (horizontal) t |= always ? Fl_Scroll::HORIZONTAL_ALWAYS : Fl_Scroll::HORIZONTAL;
	if (vertical) t |= always ? Fl_Scroll::VERTICAL_ALWAYS : Fl_Scroll::VERTICAL;
	type(t);
}

void Fl_VirtualScrollArea::SetContentSize(int width, int height)
{
	// An invisible box at the content's far corner holds the extent
	// Fl_Scroll measures from its children.
	if (!m_Extent)
	{
		Fl_Group *current = Fl_Group::current();
		begin();
		m_Extent = new Fl_Box(x(), y(), 0, 0);
		end();
		Fl_Group::current(current);
		m_Extent->box(FL_NO_BOX);
	}
	const int left = x() - xposition(), top = y() - yposition();
	m_Extent->resize(left, top, width > 0 ? width : 0, height > 0 ? height : 0);
	SetView(xposition(), yposition());
}

int Fl_VirtualScrollArea::ContentWidth() const
{
	int most = 0;
	for (int i = 0; i < children(); ++i)
	{
		const Fl_Widget *c = child(i);
		if (c == &scrollbar || c == &hscrollbar)
			continue;
		const int right = c->x() + c->w() - (x() - xposition());
		if (right > most) most = right;
	}
	return most;
}

int Fl_VirtualScrollArea::ContentHeight() const
{
	int most = 0;
	for (int i = 0; i < children(); ++i)
	{
		const Fl_Widget *c = child(i);
		if (c == &scrollbar || c == &hscrollbar)
			continue;
		const int bottom = c->y() + c->h() - (y() - yposition());
		if (bottom > most) most = bottom;
	}
	return most;
}

int Fl_VirtualScrollArea::ViewportWidth() const
{
	const bool bar = m_Vertical && (m_Always || ContentHeight() > h());
	return w() - (bar ? Fl::scrollbar_size() : 0);
}

int Fl_VirtualScrollArea::ViewportHeight() const
{
	const bool bar = m_Horizontal && (m_Always || ContentWidth() > w());
	return h() - (bar ? Fl::scrollbar_size() : 0);
}

void Fl_VirtualScrollArea::SetView(int vx, int vy)
{
	const int maxX = ContentWidth() - ViewportWidth(), maxY = ContentHeight() - ViewportHeight();
	if (vx > maxX) vx = maxX;
	if (vy > maxY) vy = maxY;
	if (vx < 0) vx = 0;
	if (vy < 0) vy = 0;
	scroll_to(vx, vy);
}
