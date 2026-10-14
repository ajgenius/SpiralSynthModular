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
#include "PanelSpinner.h"
#include "Panel.h"
#include "PanelControlRegistry.h"
#include "PanelProperty.h"
#include "JSON.h"
#include <FL/Fl.H>
#include <FL/Fl_Counter.H>
#include <cmath>

namespace SpiralPanels
{
	static PanelControlRegistry::Entry s_Entry("Spinner", &PanelSpinner::Create);

	class Counter: public Fl_Counter
	{
	public:
		Counter(int x, int y, int w, int h): Fl_Counter(x, y, w, h) {}

		virtual int handle(int event)
		{
			if (event == FL_PUSH)
				Fl::focus(this);
			return Fl_Counter::handle(event);
		}
	};

	PanelControl *PanelSpinner::Create(Panel &panel, const Spumoni::JSON &node)
	{
		return new PanelSpinner(panel, node);
	}

	PanelSpinner::PanelSpinner(Panel &panel, const Spumoni::JSON &node):
		PanelControl(panel, node)
	{
		const Spumoni::JSON *simple = node.Get("simple");
		m_Counter = new Counter(m_X, m_Y, m_W, m_H);
		m_Counter->type(!simple || simple->AsBool() ? FL_SIMPLE_COUNTER : FL_NORMAL_COUNTER);
		if (TextOf(node.Get("box")) == "plain")
			m_Counter->box(FL_UP_BOX);
		else
		{
			m_Counter->box(FL_PLASTIC_UP_BOX);
			m_Counter->color((Fl_Color)GUIColour());
			m_Counter->selection_color((Fl_Color)GUIColour());
		}
		m_Counter->textsize((int)NumberOf(node.Get("size"), 10));
		ShowRange();
		const double step = m_Step > 0 ? (node.Get("step") ? m_Step : Span(m_Step)) : 1;
		const double page = Property() && Property()->Page() > 0 ? Span(Property()->Page()) : step * 10;
		m_Counter->step(step, NumberOf(node.Get("page"), page));
		m_Counter->callback(Changed, this);
		Adopt(m_Counter);
	}

	double PanelSpinner::Span(double step) const
	{
		const double span = std::fabs(Mapped(m_Min + step) - Mapped(m_Min));
		if (!(span > 0))
			return span;
		const double digits = std::pow(10.0, 6 - std::ceil(std::log10(span)));
		return std::floor(span * digits + 0.5) / digits;
	}

	void PanelSpinner::ShowRange()
	{
		const double low = Mapped(m_MinProperty ? m_Panel.Read(*m_MinProperty) : m_Min);
		const double high = Mapped(m_MaxProperty ? m_Panel.Read(*m_MaxProperty) : m_Max);
		if (low != m_Counter->minimum() || high != m_Counter->maximum())
			m_Counter->range(low, high);
	}

	void PanelSpinner::Load()
	{
		ShowRange();
		if (Property())
			m_Counter->value(Value());
	}

	void PanelSpinner::Refresh()
	{
		if (m_MinProperty || m_MaxProperty || m_ScalePower)
			ShowRange();
	}

	void PanelSpinner::Changed(Fl_Widget *, void *data)
	{
		PanelSpinner *self = (PanelSpinner *)data;
		self->Commit(self->m_Counter->value());
	}
}
