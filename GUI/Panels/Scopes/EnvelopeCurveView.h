// SPDX-License-Identifier: GPL-2.0-or-later
/*  SpiralPlugin
 *  Copyleft (C) 2000 David Griffiths <dave@pawfal.org>
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
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/ 
#ifndef SPIRAL_PANELS_ENVELOPE_CURVE_VIEW_H
#define SPIRAL_PANELS_ENVELOPE_CURVE_VIEW_H

#include "BreakpointCurve.h"
#include <FL/Fl_Widget.H>
#include <vector>

namespace SpiralPanels
{
	class EnvelopeCurveView: public Fl_Widget
	{
	public:
		EnvelopeCurveView(int x, int y, int w, int h, unsigned maxCount);

		void SetPoints(const std::vector<Spiral::CurvePoint> &points, const std::vector<Spiral::CurvePoint> &tangents);
		void SetCount(unsigned count);
		const std::vector<Spiral::CurvePoint> &Points() const { return m_Points; }
		const std::vector<Spiral::CurvePoint> &Tangents() const { return m_Tangents; }
		void SetMode(int mode);
		void SetPlayhead(double playhead);
		void SetLength(double seconds);
		bool Dragging() const { return m_Drag >= 0; }
		int Moved() const { return m_Moved; }

	protected:
		virtual void draw();
		virtual int handle(int event);

		int Nearest(int mx, int my) const;
		int NearestHandle(int mx, int my, int &side) const;
		Spiral::CurvePoint Handle(size_t index, int side) const;
		int SubdivideLongest();
		int RemoveLeastSignificant();
		double ToX(int mx) const;
		double ToY(int my) const;
		int FromX(double x) const;
		int FromY(double y) const;

		std::vector<Spiral::CurvePoint> m_Points, m_Tangents, m_Curve;
		unsigned m_MaxCount;
		int m_Mode;
		double m_Playhead;
		double m_Length;
		int m_Drag;
		int m_DragSide;
		int m_Moved;
	};
}

#endif
