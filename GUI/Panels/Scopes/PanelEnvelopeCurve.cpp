// SPDX-License-Identifier: GPL-2.0-or-later
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
#include "PanelEnvelopeCurve.h"
#include "EnvelopeCurveView.h"
#include "Panel.h"
#include "PanelControlRegistry.h"
#include "PanelProperty.h"
#include "JSON.h"
#include <cstdio>

namespace SpiralPanels
{
	static PanelControlRegistry::Entry s_Entry("EnvelopeCurve", &PanelEnvelopeCurve::Create);

	static const int HoldTicks = 10;

	PanelControl *PanelEnvelopeCurve::Create(Panel &panel, const Spumoni::JSON &node)
	{
		return new PanelEnvelopeCurve(panel, node);
	}

	std::string PanelEnvelopeCurve::IndexedKey(const std::string &pattern, unsigned index)
	{
		const std::string::size_type at = pattern.find("{index}");
		if (at == std::string::npos)
			return pattern;
		char number[16];
		std::snprintf(number, sizeof(number), "%u", index);
		return pattern.substr(0, at) + number + pattern.substr(at + 7);
	}

	PanelEnvelopeCurve::PanelEnvelopeCurve(Panel &panel, const Spumoni::JSON &node):
		PanelControl(panel, node),
		m_MaxCount(16),
		m_Hold(0)
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
		m_View = new EnvelopeCurveView(m_X, m_Y, m_W, m_H, m_MaxCount);
		m_View->callback(Changed, this);
		Adopt(m_View);
	}

	double PanelEnvelopeCurve::ReadNamed(const std::string &name, double fallback) const
	{
		const PanelProperty *property = name.empty() ? NULL : m_Panel.FindProperty(name);
		return property ? m_Panel.Read(*property) : fallback;
	}

	void PanelEnvelopeCurve::WriteNamed(const std::string &name, double value)
	{
		const PanelProperty *property = name.empty() ? NULL : m_Panel.FindProperty(name);
		if (property)
			m_Panel.Write(*property, value);
	}

	void PanelEnvelopeCurve::Load()
	{
		unsigned count = (unsigned)ReadNamed(m_Count, 0);
		if (count > m_MaxCount)
			count = m_MaxCount;

		std::vector<Spiral::CurvePoint> points, tangents;
		for (unsigned i = 0; i < count; ++i)
		{
			points.push_back(Spiral::CurvePoint(ReadNamed(IndexedKey(m_XPattern, i), 0), ReadNamed(IndexedKey(m_YPattern, i), 0)));
			tangents.push_back(Spiral::CurvePoint(ReadNamed(IndexedKey(m_TangentXPattern, i), 0),
				ReadNamed(IndexedKey(m_TangentYPattern, i), 0)));
		}
		m_View->SetPoints(points, tangents);
		m_View->SetMode((int)ReadNamed(m_Mode, Spiral::SmoothCurve));
		m_View->SetPlayhead(ReadNamed(m_Playhead, 0));
		m_View->SetLength(ReadNamed(m_Length, 1));
	}

	void PanelEnvelopeCurve::Refresh()
	{
		if (m_View->Dragging() || m_Hold > 0)
		{
			if (m_Hold > 0)
				--m_Hold;
			m_View->SetPlayhead(ReadNamed(m_Playhead, 0));
		m_View->SetLength(ReadNamed(m_Length, 1));
			return;
		}
		Load();
	}

	void PanelEnvelopeCurve::WriteRow(unsigned index)
	{
		const Spiral::CurvePoint &point = m_View->Points()[index];
		const Spiral::CurvePoint &tangent = m_View->Tangents()[index];
		WriteNamed(IndexedKey(m_XPattern, index), point.x);
		WriteNamed(IndexedKey(m_YPattern, index), point.y);
		WriteNamed(IndexedKey(m_TangentXPattern, index), tangent.x);
		WriteNamed(IndexedKey(m_TangentYPattern, index), tangent.y);
	}

	void PanelEnvelopeCurve::WriteAll()
	{
		for (unsigned i = 0; i < m_View->Points().size(); ++i)
			WriteRow(i);
		WriteNamed(m_Count, (double)m_View->Points().size());
	}

	void PanelEnvelopeCurve::Changed(Fl_Widget *, void *data)
	{
		PanelEnvelopeCurve *self = (PanelEnvelopeCurve *)data;
		self->m_Hold = HoldTicks;
		const int moved = self->m_View->Moved();
		if (moved >= 0)
		{
			self->WriteRow((unsigned)moved);
			return;
		}
		self->WriteAll();
	}

	void PanelEnvelopeCurve::SetAttribute(const std::string &attribute, double value)
	{
		if (attribute != "count" || value < 0)
			return;
		m_View->SetCount((unsigned)value);
		m_Hold = HoldTicks;
		WriteAll();
	}
}
