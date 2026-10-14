/*  Copyleft (C) 2002 David Griffiths <dave@pawfal.org>
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

#ifndef SSM_CORE_BREAKPOINT_CURVE_H
#define SSM_CORE_BREAKPOINT_CURVE_H

#include <cstddef>
#include <vector>

namespace Spiral
{

	struct CurvePoint
	{
		double x, y;

		CurvePoint(): x(0), y(0) {}
		CurvePoint(double a, double b): x(a), y(b) {}
	};

	enum CurveMode
	{
		StraightCurve = 0,
		SmoothCurve = 1,
		BezierCurve = 2
	};

	inline bool CurveTangentAutomatic(const CurvePoint &tangent)
	{
		return tangent.x == 0 && tangent.y == 0;
	}

	inline CurvePoint CurveAutoTangent(const std::vector<CurvePoint> &points, size_t index)
	{
		if (points.size() < 2 || index >= points.size())
			return CurvePoint();

		const CurvePoint &before = points[index > 0 ? index - 1 : index];
		const CurvePoint &after = points[index + 1 < points.size() ? index + 1 : index];

		return CurvePoint((after.x - before.x) / 6, (after.y - before.y) / 6);
	}

	inline CurvePoint CurveClampTangent(const std::vector<CurvePoint> &points, size_t index, CurvePoint t)
	{
		if (index >= points.size())
			return CurvePoint();

		const CurvePoint &p = points[index];
		const bool arrives = index > 0;
		const bool leaves = index + 1 < points.size();

		if (t.x < 0)
			t.x = 0;

		if (arrives && t.x > p.x - points[index - 1].x)
			t.x = p.x - points[index - 1].x;

		if (leaves && t.x > points[index + 1].x - p.x)
			t.x = points[index + 1].x - p.x;

		if (leaves && t.y > 1 - p.y)
			t.y = 1 - p.y;

		if (leaves && t.y < -p.y)
			t.y = -p.y;

		if (arrives && t.y > p.y)
			t.y = p.y;

		if (arrives && t.y < p.y - 1)
			t.y = p.y - 1;

		return t;
	}

	inline CurvePoint CurveTangentAt(const std::vector<CurvePoint> &points, const std::vector<CurvePoint> *tangents,
		size_t index)
	{
		CurvePoint t;

		if (tangents && index < tangents->size() && !CurveTangentAutomatic((*tangents)[index]))
			t = (*tangents)[index];
		else
			t = CurveAutoTangent(points, index);

		return CurveClampTangent(points, index, t);
	}

	inline double CurveClamp(double v)
	{
		return v < 0 ? 0 : (v > 1 ? 1 : v);
	}

	inline CurvePoint CurveBezier(const CurvePoint &a, const CurvePoint &b, const CurvePoint &c, const CurvePoint &d,
		double t)
	{
		double tc = t * t * t;
		double ts = t * t;

		return CurvePoint((-a.x + 3 * b.x - 3 * c.x + d.x) * tc + (3 * a.x - 6 * b.x + 3 * c.x) * ts
				+ (-3 * a.x + 3 * b.x) * t + a.x,
			(-a.y + 3 * b.y - 3 * c.y + d.y) * tc + (3 * a.y - 6 * b.y + 3 * c.y) * ts + (-3 * a.y + 3 * b.y) * t
				+ a.y);
	}

	inline double CurveCatmullRom(double y0, double y1, double y2, double y3, double t)
	{
		double t2 = t * t;
		double t3 = t2 * t;

		return 0.5 * ((2 * y1) + (-y0 + y2) * t + (2 * y0 - 5 * y1 + 4 * y2 - y3) * t2 + (-y0 + 3 * y1 - 3 * y2 + y3) * t3);
	}

	inline void BuildBreakpointCurve(const std::vector<CurvePoint> &points, CurveMode mode, int sections,
		std::vector<CurvePoint> *out, const std::vector<CurvePoint> *tangents = NULL)
	{
		if (!out)
			return;

		out->clear();

		if (points.empty())
			return;

		if (mode == StraightCurve || points.size() < 2)
		{
			for (size_t i = 0; i < points.size(); ++i)
				out->push_back(CurvePoint(points[i].x, CurveClamp(points[i].y)));

			return;
		}

		if (sections < 1)
			sections = 1;

		out->push_back(CurvePoint(points[0].x, CurveClamp(points[0].y)));

		if (mode == SmoothCurve)
		{
			for (size_t i = 0; i + 1 < points.size(); ++i)
			{
				const CurvePoint &p1 = points[i];
				const CurvePoint &p2 = points[i + 1];
				const CurvePoint &p0 = i > 0 ? points[i - 1] : p1;
				const CurvePoint &p3 = i + 2 < points.size() ? points[i + 2] : p2;

				for (int s = 1; s <= sections; ++s)
				{
					double t = s / (double)sections;

					out->push_back(CurvePoint(p1.x + (p2.x - p1.x) * t,
						CurveClamp(CurveCatmullRom(p0.y, p1.y, p2.y, p3.y, t))));
				}
			}

			return;
		}

		CurvePoint leave = CurveTangentAt(points, tangents, 0);

		for (size_t i = 0; i + 1 < points.size(); ++i)
		{
			const CurvePoint &a = points[i];
			const CurvePoint &d = points[i + 1];
			const CurvePoint arrive = CurveTangentAt(points, tangents, i + 1);
			const CurvePoint b(a.x + leave.x, a.y + leave.y);
			const CurvePoint c(d.x - arrive.x, d.y - arrive.y);

			for (int s = 1; s <= sections; ++s)
			{
				CurvePoint p = CurveBezier(a, b, c, d, s / (double)sections);

				out->push_back(CurvePoint(p.x, CurveClamp(p.y)));
			}

			leave = arrive;
		}
	}

	inline void RasteriseBreakpointCurve(const std::vector<CurvePoint> &curve, float *out, size_t count)
	{
		if (!out || count == 0)
			return;

		if (curve.empty())
		{
			for (size_t n = 0; n < count; ++n)
				out[n] = 0;

			return;
		}

		if (curve.size() == 1)
		{
			for (size_t n = 0; n < count; ++n)
				out[n] = (float)curve[0].y;

			return;
		}

		size_t seg = 0;

		for (size_t n = 0; n < count; ++n)
		{
			double gt = n / (double)count;

			while (seg + 2 < curve.size() && gt >= curve[seg + 1].x)
				++seg;

			const CurvePoint &a = curve[seg];
			const CurvePoint &b = curve[seg + 1];

			if (gt >= b.x)
			{
				out[n] = (float)b.y;
				continue;
			}

			double span = b.x - a.x;
			double t = span > 0 ? (gt - a.x) / span : 0;

			if (t < 0)
				t = 0;

			if (t > 1)
				t = 1;

			out[n] = (float)(a.y + (b.y - a.y) * t);
		}
	}

}

#endif
