// SPDX-License-Identifier: GPL-2.0-or-later
#include "Input.h"
#include "ControlSupport.h"
#include <cmath>
#include <cstdio>

namespace Spiral
{
	namespace Panels
	{
		static PanelControlRegistry::Entry s_EnvelopeCurve("EnvelopeCurve", &EnvelopeCurveControl::Create);

		static const int HoldTicks = 10;
		static const double MinGap = 0.002;

		PanelControl *EnvelopeCurveControl::Create(Panel &panel, const Spumoni::JSON &node)
		{
			return Viewed(new EnvelopeCurveControl(panel, node));
		}

		std::string EnvelopeCurveControl::IndexedKey(const std::string &pattern, unsigned index)
		{
			const std::string::size_type at = pattern.find("{index}");

			if (at == std::string::npos)
				return pattern;

			char number[16];
			std::snprintf(number, sizeof(number), "%u", index);

			return pattern.substr(0, at) + number + pattern.substr(at + 7);
		}

		EnvelopeCurveControl::EnvelopeCurveControl(Panel &panel, const Spumoni::JSON &node):
			PanelControl(panel, node),
			m_Curve(NULL),
			m_MaxCount(16),
			m_Hold(0),
			m_ModeShown(SmoothCurve),
			m_PlayheadShown(0),
			m_LengthShown(1),
			m_Drag(-1),
			m_DragSide(0)
		{
			if (node.Get("mode"))
				m_Mode = TextOf(node.Get("mode")->Get("property"));

			if (node.Get("playhead"))
				m_Playhead = TextOf(node.Get("playhead")->Get("property"));

			if (node.Get("length") && node.Get("length")->Get("property"))
				m_Length = TextOf(node.Get("length")->Get("property"));

			const Spumoni::JSON *points = node.Get("points");

			if (points)
			{
				m_Count = TextOf(points->Get("count_property"));
				m_XPattern = TextOf(points->Get("x_property"));
				m_YPattern = TextOf(points->Get("y_property"));
				m_TangentXPattern = TextOf(points->Get("tangent_x_property"));
				m_TangentYPattern = TextOf(points->Get("tangent_y_property"));
				m_MaxCount = (unsigned)NumberOf(points->Get("max_count"), 16);
			}

			m_Curve = MakeView<CurveView>("EnvelopeCurve", *this, node);

			if (m_Curve)
				Adopt(m_Curve);
		}

		double EnvelopeCurveControl::ReadNamed(const std::string &name, double fallback) const
		{
			const PanelProperty *property = name.empty() ? NULL : m_Panel.FindProperty(name);

			return property ? m_Panel.Read(*property) : fallback;
		}

		void EnvelopeCurveControl::WriteNamed(const std::string &name, double value)
		{
			const PanelProperty *property = name.empty() ? NULL : m_Panel.FindProperty(name);

			if (property)
				m_Panel.Write(*property, value);
		}

		static bool Same(const std::vector<CurvePoint> &a, const std::vector<CurvePoint> &b)
		{
			if (a.size() != b.size())
				return false;

			for (size_t i = 0; i < a.size(); ++i)
			{
				if (a[i].x != b[i].x || a[i].y != b[i].y)
					return false;
			}

			return true;
		}

		void EnvelopeCurveControl::Load()
		{
			unsigned count = (unsigned)ReadNamed(m_Count, 0);

			if (count > m_MaxCount)
				count = m_MaxCount;

			std::vector<CurvePoint> points, tangents;

			for (unsigned i = 0; i < count; ++i)
			{
				points.push_back(CurvePoint(ReadNamed(IndexedKey(m_XPattern, i), 0), ReadNamed(IndexedKey(m_YPattern, i), 0)));
				tangents.push_back(CurvePoint(ReadNamed(IndexedKey(m_TangentXPattern, i), 0),
						ReadNamed(IndexedKey(m_TangentYPattern, i), 0)));
			}

			const int mode = (int)ReadNamed(m_Mode, SmoothCurve);
			bool changed = !Same(points, m_Points) || !Same(tangents, m_Tangents) || mode != m_ModeShown;
			m_Points = points;
			m_Tangents = tangents;
			m_ModeShown = mode;

			if (changed)
				m_Curve->Redraw();

			ShowPlayhead();
		}

		void EnvelopeCurveControl::ShowPlayhead()
		{
			const double playhead = ReadNamed(m_Playhead, 0);
			const double length = ReadNamed(m_Length, 1);

			if (playhead == m_PlayheadShown && length == m_LengthShown)
				return;

			m_PlayheadShown = playhead;
			m_LengthShown = length;
			m_Curve->Redraw();
		}

		void EnvelopeCurveControl::Refresh()
		{
			if (!Dragging() && m_Hold <= 0)
			{
				Load();

				return;
			}

			if (m_Hold > 0)
				--m_Hold;

			ShowPlayhead();
		}

		void EnvelopeCurveControl::WriteRow(unsigned index)
		{
			WriteNamed(IndexedKey(m_XPattern, index), m_Points[index].x);
			WriteNamed(IndexedKey(m_YPattern, index), m_Points[index].y);
			WriteNamed(IndexedKey(m_TangentXPattern, index), m_Tangents[index].x);
			WriteNamed(IndexedKey(m_TangentYPattern, index), m_Tangents[index].y);
		}

		void EnvelopeCurveControl::WriteAll()
		{
			for (unsigned i = 0; i < m_Points.size(); ++i)
				WriteRow(i);

			WriteNamed(m_Count, (double)m_Points.size());
		}

		void EnvelopeCurveControl::Edited(int moved)
		{
			m_Hold = HoldTicks;
			m_Curve->Redraw();

			if (moved >= 0)
				WriteRow((unsigned)moved);
			else
				WriteAll();
		}

		CurvePoint EnvelopeCurveControl::Handle(size_t index, int side) const
		{
			const CurvePoint t = CurveTangentAt(m_Points, &m_Tangents, index);

			return CurvePoint(m_Points[index].x + side * t.x, m_Points[index].y + side * t.y);
		}

		void EnvelopeCurveControl::AddPoint(double x, double y)
		{
			if (m_Points.size() >= m_MaxCount)
				return;

			size_t at = 0;

			while (at < m_Points.size() && m_Points[at].x <= x)
				++at;

			if ((at > 0 && x - m_Points[at - 1].x < MinGap) || (at < m_Points.size() && m_Points[at].x - x < MinGap))
				return;

			m_Points.insert(m_Points.begin() + at, CurvePoint(x, y));
			m_Tangents.insert(m_Tangents.begin() + at, CurvePoint());
			Edited(-1);
		}

		void EnvelopeCurveControl::RemovePoint(size_t index)
		{
			if (m_Points.size() <= 2 || index >= m_Points.size())
				return;

			m_Points.erase(m_Points.begin() + index);
			m_Tangents.erase(m_Tangents.begin() + index);
			Edited(-1);
		}

		void EnvelopeCurveControl::ResetHandle(size_t index)
		{
			if (index >= m_Tangents.size())
				return;

			m_Tangents[index] = CurvePoint();
			Edited((int)index);
		}

		void EnvelopeCurveControl::BeginDrag(int index, int side)
		{
			m_Drag = index >= 0 && index < (int)m_Points.size() ? index : -1;
			m_DragSide = m_Drag >= 0 ? side : 0;
		}

		void EnvelopeCurveControl::DragTo(double x, double y)
		{
			if (m_Drag < 0)
				return;

			if (m_DragSide)
			{
				const CurvePoint &p = m_Points[m_Drag];
				CurvePoint clamped = CurveClampTangent(m_Points, m_Drag, CurvePoint(m_DragSide * (x - p.x), m_DragSide * (y - p.y)));

				if (clamped.x == 0 && clamped.y == 0)
					clamped.x = 1e-6;

				m_Tangents[m_Drag] = clamped;
				Edited(m_Drag);

				return;
			}

			double earliest = m_Drag > 0 ? m_Points[m_Drag - 1].x + MinGap : 0;
			double latest = m_Drag + 1 < (int)m_Points.size() ? m_Points[m_Drag + 1].x - MinGap : 1;

			if (earliest > latest)
				earliest = latest = m_Points[m_Drag].x;

			m_Points[m_Drag] = CurvePoint(x < earliest ? earliest : x > latest ? latest : x, y);
			Edited(m_Drag);
		}

		void EnvelopeCurveControl::EndDrag()
		{
			m_Drag = -1;
			m_DragSide = 0;
		}

		void EnvelopeCurveControl::SetCount(unsigned count)
		{
			while (m_Points.size() < count && SubdivideLongest() >= 0)
				;

			while (m_Points.size() > count && RemoveLeastSignificant() >= 0)
				;
		}

		int EnvelopeCurveControl::SubdivideLongest()
		{
			if (m_Points.size() >= m_MaxCount || m_Points.size() < 2)
				return -1;

			double pw, ph;
			m_Curve->PlotSize(pw, ph);
			size_t widest = 0;
			double best = -1;

			for (size_t i = 0; i + 1 < m_Points.size(); ++i)
			{
				const double span = m_Points[i + 1].x - m_Points[i].x;

				if (span <= MinGap)
					continue;

				const double dx = span * pw;
				const double dy = (m_Points[i + 1].y - m_Points[i].y) * ph;
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
			CurvePoint tangent;

			if (m_ModeShown != StraightCurve)
			{
				std::vector<CurvePoint> curve;
				BuildBreakpointCurve(m_Points, (CurveMode)m_ModeShown, 16, &curve, &m_Tangents);
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

				if (m_ModeShown == BezierCurve && at > 0 && at + 1 < curve.size())
				{
					const double dx = curve[at + 1].x - curve[at - 1].x;
					const double dy = curve[at + 1].y - curve[at - 1].y;

					if (dx > 0)
						tangent = CurvePoint(span / 6, dy / dx * span / 6);
				}
			}

			m_Points.insert(m_Points.begin() + (widest + 1), CurvePoint(mid, CurveClamp(level)));
			m_Tangents.insert(m_Tangents.begin() + (widest + 1), tangent);

			return (int)widest + 1;
		}

		int EnvelopeCurveControl::RemoveLeastSignificant()
		{
			if (m_Points.size() < 3)
				return -1;

			double xs, ys;
			m_Curve->PlotSize(xs, ys);
			size_t flattest = 0;
			double best = -1;

			for (size_t i = 1; i + 1 < m_Points.size(); ++i)
			{
				const double ax = m_Points[i - 1].x * xs;
				const double ay = m_Points[i - 1].y * ys;
				const double bx = m_Points[i + 1].x * xs;
				const double by = m_Points[i + 1].y * ys;
				const double px = m_Points[i].x * xs;
				const double py = m_Points[i].y * ys;
				const double dx = bx - ax;
				const double dy = by - ay;
				const double length = std::sqrt(dx * dx + dy * dy);
				const double distance = length > 0 ? std::fabs(dx * (ay - py) - (ax - px) * dy) / length
						: std::sqrt((px - ax) * (px - ax) + (py - ay) * (py - ay));

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

		void EnvelopeCurveControl::SetAttribute(const std::string &attribute, double value)
		{
			if (attribute != "count" || value < 0)
				return;

			SetCount((unsigned)value);
			Edited(-1);
		}
	}
}
