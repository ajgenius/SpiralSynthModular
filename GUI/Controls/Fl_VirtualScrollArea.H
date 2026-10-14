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
// Written by GROK (Grok Bot), after the private tree's VirtualScrollArea:
// one clipped viewport over content of its own (virtual) size, with the
// scrollbars wanted. Its bounds stay the host's; the content may be wider
// or taller. A reusable container: a piano, a long table, a port list.
#ifndef FL_VIRTUAL_SCROLL_AREA_H
#define FL_VIRTUAL_SCROLL_AREA_H

#include <FL/Fl_Scroll.H>

class Fl_Box;

class Fl_VirtualScrollArea : public Fl_Scroll
{
public:
	Fl_VirtualScrollArea(int x, int y, int w, int h, const char *label = 0);

	// Which bars may show; each shows only when the content needs it,
	// or always ("always").
	void Show(bool horizontal, bool vertical, bool always = false);
	bool ShowHorizontal() const { return m_Horizontal; }
	bool ShowVertical() const { return m_Vertical; }

	// The virtual content's size; 0 keeps the children's own extent.
	// Call once the children are added.
	void SetContentSize(int width, int height);
	int ContentWidth() const;
	int ContentHeight() const;

	// The content point at the viewport's top left, kept in range.
	void SetView(int x, int y);
	int ViewX() const { return xposition(); }
	int ViewY() const { return yposition(); }

	// The viewport left for content under the bars that show.
	int ViewportWidth() const;
	int ViewportHeight() const;

protected:
	void draw();

private:
	bool m_Horizontal, m_Vertical, m_Always;
	Fl_Box *m_Extent;
};

#endif
