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
#include "PanelKnob.h"
#include "Panel.h"
#include "PanelControlRegistry.h"
#include "PanelProperty.h"
#include "Fl_Knob.H"
#include "JSON.h"
#include <algorithm>

namespace SpiralPanels
{
	static PanelControlRegistry::Entry s_Entry("Knob", &PanelKnob::Create);

	PanelControl *PanelKnob::Create(Panel &panel, const Spumoni::JSON &node)
	{
		return new PanelKnob(panel, node);
	}

	PanelKnob::PanelKnob(Panel &panel, const Spumoni::JSON &node):
		PanelControl(panel, node)
	{
		m_Knob = new Fl_Knob(m_X, m_Y, m_W, m_H);
		m_Knob->color((Fl_Color)GUIColour());
		m_Knob->type(TextOf(node.Get("style")) == "dot" ? Fl_Knob::DOTLIN : Fl_Knob::LINELIN);
		m_Knob->scaleticks((int)NumberOf(node.Get("scale_ticks"), 10));
		m_Knob->range(std::min(Mapped(m_Min), Mapped(m_Max)), std::max(Mapped(m_Min), Mapped(m_Max)));
		if (m_Step > 0)
			m_Knob->step(m_Step);
		m_Knob->callback(Changed, this);
		Adopt(m_Knob);
	}

	void PanelKnob::Load()
	{
		if (m_MinProperty || m_MaxProperty)
		{
			const double low = m_MinProperty ? Mapped(m_Panel.Read(*m_MinProperty)) : m_Knob->minimum();
			const double high = m_MaxProperty ? Mapped(m_Panel.Read(*m_MaxProperty)) : m_Knob->maximum();
			if (low < high)
				m_Knob->range(low, high);
		}
		double linked;
		if (Property())
		{
			m_Knob->value(Value());
			Link(m_Knob->value());
		}
		else if (LinkedValue(linked))
			m_Knob->value(linked);
	}

	void PanelKnob::Changed(Fl_Widget *, void *data)
	{
		PanelKnob *self = (PanelKnob *)data;
		self->Commit(self->m_Knob->value());
	}
}
