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

namespace SpiralPanels
{
	static const int Inset = 7;
	static const int Grab = 6;
	static const double MinGap = 0.002;

	EnvelopeCurveView::EnvelopeCurveView(int x, int y, int w, int h, unsigned maxCount):
		Fl_Widget(x, y, w, h),
		m_MaxCount(maxCount),
		m_Mode(Spiral::SmoothCurve),
		m_Playhead(0),
		m_Length(1),
		m_Drag(-1),
		m_DragSide(0),
		m_Moved(-1)
	{
		color(fl_rgb_color(14, 51, 14));
	}

	void EnvelopeCurveView::SetPoints(const std::vector<Spiral::CurvePoint> &points,
		const std::vector<Spiral::CurvePoint> &tangents)
	{
		m_Points = points;
		m_Tangents = tangents;
		m_Tangents.resize(m_Points.size());
		redraw();
	}

	void EnvelopeCurveView::SetCount(unsigned count)
	{
		while (m_Points.size() < count && SubdivideLongest() >= 0)
			;
		while (m_Points.size() > count && RemoveLeastSignificant() >= 0)
			;
		m_Moved = -1;
		redraw();
	}

	int EnvelopeCurveView::SubdivideLongest()
	{
		if (m_Points.size() >= m_MaxCount || m_Points.size() < 2)
			return -1;
		const double pw = w() - 2 * Inset, ph = h() - 2 * Inset;
		size_t widest = 0;
		double best = -1;
		for (size_t i = 0; i + 1 < m_Points.size(); ++i)
		{
			const double span = m_Points[i + 1].x - m_Points[i].x;
			if (span <= MinGap)
				continue;
			const double dx = span * pw, dy = (m_Points[i + 1].y - m_Points[i].y) * ph;
			const double length = std::sqrt(dx * dx + dy * dy);
			if (length > best)
			{
				best = length;
				widest = i;
			}
		}
		if (best < 0)
			return -1;

		const double span = m_Points[widest + 1].x - m_Points[widest].x;
		const double mid = m_Points[widest].x + span / 2;
		double level = (m_Points[widest].y + m_Points[widest + 1].y) / 2;
		Spiral::CurvePoint tangent;
		if (m_Mode != Spiral::StraightCurve)
		{
			std::vector<Spiral::CurvePoint> curve;
			Spiral::BuildBreakpointCurve(m_Points, (Spiral::CurveMode)m_Mode, 16, &curve, &m_Tangents);
			double closest = -1;
			size_t at = 0;
			for (size_t i = 0; i < curve.size(); ++i)
			{
				const double d = std::fabs(curve[i].x - mid);
				if (closest < 0 || d < closest)
				{
					closest = d;
					level = curve[i].y;
					at = i;
				}
			}
			if (m_Mode == Spiral::BezierCurve && at > 0 && at + 1 < curve.size())
			{
				const double dx = curve[at + 1].x - curve[at - 1].x, dy = curve[at + 1].y - curve[at - 1].y;
				if (dx > 0)
					tangent = Spiral::CurvePoint(span / 6, dy / dx * span / 6);
			}
		}
		m_Points.insert(m_Points.begin() + (widest + 1), Spiral::CurvePoint(mid, Spiral::CurveClamp(level)));
		m_Tangents.insert(m_Tangents.begin() + (widest + 1), tangent);
		return (int)widest + 1;
	}

	int EnvelopeCurveView::RemoveLeastSignificant()
	{
		if (m_Points.size() < 3)
			return -1;
		const double xs = w() - 2 * Inset, ys = h() - 2 * Inset;
		size_t flattest = 0;
		double best = -1;
		for (size_t i = 1; i + 1 < m_Points.size(); ++i)
		{
			const double ax = m_Points[i - 1].x * xs, ay = m_Points[i - 1].y * ys;
			const double bx = m_Points[i + 1].x * xs, by = m_Points[i + 1].y * ys;
			const double px = m_Points[i].x * xs, py = m_Points[i].y * ys;
			const double dx = bx - ax, dy = by - ay;
			const double length = std::sqrt(dx * dx + dy * dy);
			const double distance = length > 0 ? std::fabs(dx * (ay - py) - (ax - px) * dy) / length :
				std::sqrt((px - ax) * (px - ax) + (py - ay) * (py - ay));
			if (best < 0 || distance < best)
			{
				best = distance;
				flattest = i;
			}
		}
		m_Points.erase(m_Points.begin() + flattest);
		m_Tangents.erase(m_Tangents.begin() + flattest);
		return (int)flattest;
	}

	void EnvelopeCurveView::SetMode(int mode)
	{
		if (mode != m_Mode)
		{
			m_Mode = mode;
			redraw();
		}
	}

	void EnvelopeCurveView::SetPlayhead(double playhead)
	{
		if (playhead != m_Playhead)
		{
			m_Playhead = playhead;
			redraw();
		}
	}

	void EnvelopeCurveView::SetLength(double seconds)
	{
		if (seconds != m_Length)
		{
			m_Length = seconds;
			redraw();
		}
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
		for (size_t i = 0; i < m_Points.size(); ++i)
		{
			if (std::abs(FromX(m_Points[i].x) - mx) <= Grab && std::abs(FromY(m_Points[i].y) - my) <= Grab)
				return (int)i;
		}
		return -1;
	}

	Spiral::CurvePoint EnvelopeCurveView::Handle(size_t index, int side) const
	{
		const Spiral::CurvePoint t = Spiral::CurveTangentAt(m_Points, &m_Tangents, index);
		return Spiral::CurvePoint(m_Points[index].x + side * t.x, m_Points[index].y + side * t.y);
	}

	int EnvelopeCurveView::NearestHandle(int mx, int my, int &side) const
	{
		if (m_Mode != Spiral::BezierCurve)
			return -1;
		for (size_t i = 0; i < m_Points.size(); ++i)
		{
			for (int s = -1; s <= 1; s += 2)
			{
				if ((s < 0 && i == 0) || (s > 0 && i + 1 == m_Points.size()))
					continue;
				const Spiral::CurvePoint p = Handle(i, s);
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
		const Fl_Color grid = fl_rgb_color(92, 102, 20), axis = fl_rgb_color(204, 204, 20);
		const Fl_Color ink = fl_rgb_color(102, 255, 77), handle = fl_rgb_color(255, 140, 38);
		const int left = FromX(0), right = FromX(1), top = FromY(1), bottom = FromY(0);
		draw_box(FL_PLASTIC_DOWN_BOX, color());
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
		for (int s = 1; s <= (int)m_Length; ++s)
		{
			const int lx = left + (int)((right - left) * (s / (m_Length > 0 ? m_Length : 1)));
			if (lx >= left && lx <= right)
				fl_line(lx, top, lx, bottom);
		}

		Spiral::BuildBreakpointCurve(m_Points, (Spiral::CurveMode)m_Mode, 10, &m_Curve, &m_Tangents);
		if (m_Mode == Spiral::BezierCurve)
		{
			fl_color(axis);
			for (size_t i = 0; i < m_Points.size(); ++i)
			{
				const int px = FromX(m_Points[i].x), py = FromY(m_Points[i].y);
				for (int s = -1; s <= 1; s += 2)
				{
					if ((s < 0 && i == 0) || (s > 0 && i + 1 == m_Points.size()))
						continue;
					const Spiral::CurvePoint h = Handle(i, s);
					fl_line(px, py, FromX(h.x), FromY(h.y));
					fl_pie(FromX(h.x) - 2, FromY(h.y) - 2, 5, 5, 0, 360);
				}
			}
		}
		fl_color(ink);
		fl_line_style(FL_SOLID, 1);
		for (size_t i = 1; i < m_Curve.size(); ++i)
			fl_line(FromX(m_Curve[i - 1].x), FromY(m_Curve[i - 1].y), FromX(m_Curve[i].x), FromY(m_Curve[i].y));

		for (size_t i = 0; i < m_Points.size(); ++i)
		{
			const int px = FromX(m_Points[i].x), py = FromY(m_Points[i].y);
			fl_color(handle);
			fl_pie(px - 4, py - 4, 9, 9, 0, 360);
			fl_color(color());
			fl_arc(px - 4, py - 4, 9, 9, 0, 360);
		}

		if (m_Playhead > 0)
		{
			fl_color(fl_rgb_color(255, 209, 15));
			fl_line(FromX(m_Playhead), top, FromX(m_Playhead), bottom);
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
					if (m_Points.size() >= m_MaxCount)
						return 1;
					const Spiral::CurvePoint added(ToX(Fl::event_x()), ToY(Fl::event_y()));
					size_t at = 0;
					while (at < m_Points.size() && m_Points[at].x <= added.x)
						++at;
					if ((at > 0 && added.x - m_Points[at - 1].x < MinGap) ||
						(at < m_Points.size() && m_Points[at].x - added.x < MinGap))
						return 1;
					m_Points.insert(m_Points.begin() + at, added);
					m_Tangents.insert(m_Tangents.begin() + at, Spiral::CurvePoint());
					m_Moved = -1;
					redraw();
					do_callback();
					return 1;
				}
				if (hit >= 0 && (Fl::event_state(FL_CTRL) || Fl::event_button() == FL_RIGHT_MOUSE))
				{
					if (m_Points.size() <= 2)
						return 1;
					m_Points.erase(m_Points.begin() + hit);
					m_Tangents.erase(m_Tangents.begin() + hit);
					m_Moved = -1;
					redraw();
					do_callback();
					return 1;
				}
				int side = 0;
				const int handle = hit >= 0 ? -1 : NearestHandle(Fl::event_x(), Fl::event_y(), side);
				if (handle >= 0 && (Fl::event_clicks() || Fl::event_state(FL_CTRL) || Fl::event_button() == FL_RIGHT_MOUSE))
				{
					m_Tangents[handle] = Spiral::CurvePoint();
					m_Moved = handle;
					redraw();
					do_callback();
					return 1;
				}
				m_Drag = handle >= 0 ? handle : hit;
				m_DragSide = handle >= 0 ? side : 0;
				return 1;
			}
			case FL_DRAG:
			{
				if (m_Drag < 0)
					return 1;
				if (m_DragSide)
				{
					const Spiral::CurvePoint &p = m_Points[m_Drag];
					const Spiral::CurvePoint t(m_DragSide * (ToX(Fl::event_x()) - p.x),
						m_DragSide * (ToY(Fl::event_y()) - p.y));
					Spiral::CurvePoint clamped = Spiral::CurveClampTangent(m_Points, m_Drag, t);
					if (clamped.x == 0 && clamped.y == 0)
						clamped.x = 1e-6;
					m_Tangents[m_Drag] = clamped;
					m_Moved = m_Drag;
					redraw();
					do_callback();
					return 1;
				}
				double px = ToX(Fl::event_x());
				double earliest = m_Drag > 0 ? m_Points[m_Drag - 1].x + MinGap : 0;
				double latest = m_Drag + 1 < (int)m_Points.size() ? m_Points[m_Drag + 1].x - MinGap : 1;
				if (earliest > latest)
					earliest = latest = m_Points[m_Drag].x;
				if (px < earliest)
					px = earliest;
				if (px > latest)
					px = latest;
				m_Points[m_Drag] = Spiral::CurvePoint(px, ToY(Fl::event_y()));
				m_Moved = m_Drag;
				redraw();
				do_callback();
				return 1;
			}
			case FL_RELEASE:
				m_Drag = -1;
				m_DragSide = 0;
				return 1;
		}
		return Fl_Widget::handle(event);
	}
}
