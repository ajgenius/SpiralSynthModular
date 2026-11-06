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

#include "Input.h"
#include "FltkTheme.h"
#include <FL/Fl_Widget.H>
#include <vector>

namespace Spiral
{
	namespace Panels
	{
		class EnvelopeCurveView: public Fl_Widget
		{
		public:
			EnvelopeCurveView(int x, int y, int w, int h, EnvelopeCurveControl &control, const FltkLook &look);

			int PlotWidth() const { return w() - 2 * Inset; }
			int PlotHeight() const { return h() - 2 * Inset; }

		protected:
			static const int Inset = 7;

			virtual void draw();
			virtual int handle(int event);

			int Nearest(int mx, int my) const;
			int NearestHandle(int mx, int my, int &side) const;
			double ToX(int mx) const;
			double ToY(int my) const;
			int FromX(double x) const;
			int FromY(double y) const;

			EnvelopeCurveControl &m_Control;
			FltkLook m_Look;
			std::vector<CurvePoint> m_Curve;
		};
	}
}

#endif
