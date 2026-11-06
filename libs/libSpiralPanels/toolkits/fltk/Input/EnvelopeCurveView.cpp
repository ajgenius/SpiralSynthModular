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
#include "EnvelopeCurveView.h"
#include <FL/Fl.H>
#include <FL/fl_draw.H>
#include <cmath>
#include <cstdlib>

namespace Spiral
{
	namespace Panels
	{
		static const int Grab = 6;

		EnvelopeCurveView::EnvelopeCurveView(int x, int y, int w, int h, EnvelopeCurveControl &control,
				const FltkLook &look):
			Fl_Widget(x, y, w, h),
			m_Control(control),
			m_Look(look)
		{
			look.Apply(this);
		}

		double EnvelopeCurveView::ToX(int mx) const
		{
			const double v = (double)(mx - x() - Inset) / (w() - 2 * Inset);
			return v < 0 ? 0 : v > 1 ? 1 : v;
		}

		double EnvelopeCurveView::ToY(int my) const
		{
			const double v = 1.0 - (double)(my - y() - Inset) / (h() - 2 * Inset);
			return v < 0 ? 0 : v > 1 ? 1 : v;
		}

		int EnvelopeCurveView::FromX(double v) const
		{
			return x() + Inset + (int)(v * (w() - 2 * Inset));
		}

		int EnvelopeCurveView::FromY(double v) const
		{
			return y() + Inset + (int)((1.0 - v) * (h() - 2 * Inset));
		}

		int EnvelopeCurveView::Nearest(int mx, int my) const
		{
			for (size_t i = 0; i < m_Control.Points().size(); ++i)
			{
				if (std::abs(FromX(m_Control.Points()[i].x) - mx) <= Grab && std::abs(FromY(m_Control.Points()[i].y) - my) <= Grab)
					return (int)i;
			}
			return -1;
		}

		int EnvelopeCurveView::NearestHandle(int mx, int my, int &side) const
		{
			if (m_Control.Mode() != BezierCurve)
				return -1;
			for (size_t i = 0; i < m_Control.Points().size(); ++i)
			{
				for (int s = -1; s <= 1; s += 2)
				{
					if ((s < 0 && i == 0) || (s > 0 && i + 1 == m_Control.Points().size()))
						continue;
					const CurvePoint p = m_Control.Handle(i, s);
					if (std::abs(FromX(p.x) - mx) <= Grab && std::abs(FromY(p.y) - my) <= Grab)
					{
						side = s;
						return (int)i;
					}
				}
			}
			return -1;
		}

		void EnvelopeCurveView::draw()
		{
			const Fl_Color grid = m_Look.Colour("grid"), axis = m_Look.Colour("axis");
			const Fl_Color ink = m_Look.Colour("trace"), handle = m_Look.Colour("handle");
			const int left = FromX(0), right = FromX(1), top = FromY(1), bottom = FromY(0);
			draw_box(box(), color());
			fl_color(color());
			fl_rectf(x() + 2, y() + 2, w() - 4, h() - 4);

			fl_push_clip(x() + 2, y() + 2, w() - 4, h() - 4);
			for (int i = 0; i <= 10; ++i)
			{
				fl_color(i == 5 ? axis : grid);
				const int ly = top + i * (bottom - top) / 10;
				fl_line(left, ly, right, ly);
			}
			fl_color(grid);
			for (int s = 1; s <= (int)m_Control.Length(); ++s)
			{
				const int lx = left + (int)((right - left) * (s / (m_Control.Length() > 0 ? m_Control.Length() : 1)));
				if (lx >= left && lx <= right)
					fl_line(lx, top, lx, bottom);
			}

			BuildBreakpointCurve(m_Control.Points(), (CurveMode)m_Control.Mode(), 10, &m_Curve, &m_Control.Tangents());
			if (m_Control.Mode() == BezierCurve)
			{
				fl_color(axis);
				for (size_t i = 0; i < m_Control.Points().size(); ++i)
				{
					const int px = FromX(m_Control.Points()[i].x), py = FromY(m_Control.Points()[i].y);
					for (int s = -1; s <= 1; s += 2)
					{
						if ((s < 0 && i == 0) || (s > 0 && i + 1 == m_Control.Points().size()))
							continue;
						const CurvePoint h = m_Control.Handle(i, s);
						fl_line(px, py, FromX(h.x), FromY(h.y));
						fl_pie(FromX(h.x) - 2, FromY(h.y) - 2, 5, 5, 0, 360);
					}
				}
			}
			fl_color(ink);
			fl_line_style(FL_SOLID, 1);
			for (size_t i = 1; i < m_Curve.size(); ++i)
				fl_line(FromX(m_Curve[i - 1].x), FromY(m_Curve[i - 1].y), FromX(m_Curve[i].x), FromY(m_Curve[i].y));

			for (size_t i = 0; i < m_Control.Points().size(); ++i)
			{
				const int px = FromX(m_Control.Points()[i].x), py = FromY(m_Control.Points()[i].y);
				fl_color(handle);
				fl_pie(px - 4, py - 4, 9, 9, 0, 360);
				fl_color(color());
				fl_arc(px - 4, py - 4, 9, 9, 0, 360);
			}

			if (m_Control.Playhead() > 0)
			{
				fl_color(m_Look.Colour("playhead"));
				fl_line(FromX(m_Control.Playhead()), top, FromX(m_Control.Playhead()), bottom);
			}
			fl_pop_clip();
		}

		int EnvelopeCurveView::handle(int event)
		{
			switch (event)
			{
				case FL_PUSH:
				{
					const int hit = Nearest(Fl::event_x(), Fl::event_y());

					if (Fl::event_state(FL_ALT))
					{
						m_Control.AddPoint(ToX(Fl::event_x()), ToY(Fl::event_y()));

						return 1;
					}

					if (hit >= 0 && (Fl::event_state(FL_CTRL) || Fl::event_button() == FL_RIGHT_MOUSE))
					{
						m_Control.RemovePoint((size_t)hit);

						return 1;
					}

					int side = 0;
					const int handle = hit >= 0 ? -1 : NearestHandle(Fl::event_x(), Fl::event_y(), side);

					if (handle >= 0 && (Fl::event_clicks() || Fl::event_state(FL_CTRL) || Fl::event_button() == FL_RIGHT_MOUSE))
					{
						m_Control.ResetHandle((size_t)handle);

						return 1;
					}

					m_Control.BeginDrag(handle >= 0 ? handle : hit, handle >= 0 ? side : 0);

					return 1;
				}
				case FL_DRAG:
					if (m_Control.Dragging())
						m_Control.DragTo(ToX(Fl::event_x()), ToY(Fl::event_y()));

					return 1;
				case FL_RELEASE:
					m_Control.EndDrag();

					return 1;
			}

			return Fl_Widget::handle(event);
		}
	}
}
