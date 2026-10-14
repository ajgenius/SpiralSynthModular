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
#include "PanelLED.h"
#include "Panel.h"
#include "PanelControlRegistry.h"
#include "PanelProperty.h"
#include "JSON.h"
#include "Fl_LED_Button.H"

namespace SpiralPanels
{
	static PanelControlRegistry::Entry s_Entry("LED", &PanelLED::Create);

	PanelControl *PanelLED::Create(Panel &panel, const Spumoni::JSON &node)
	{
		return new PanelLED(panel, node);
	}

	PanelLED::PanelLED(Panel &panel, const Spumoni::JSON &node):
		PanelControl(panel, node),
		m_HasChoice(node.Get("choice") != NULL),
		m_Choice(NumberOf(node.Get("choice"), 0)),
		m_Latch(node.Get("latch") && node.Get("latch")->AsBool())
	{
		m_Button = new Fl_LED_Button(m_X, m_Y, m_W, m_H);
		m_Button->copy_label(TextOf(node.Get("text")).c_str());
		m_Button->labelsize(10);
		m_Button->type(FL_TOGGLE_BUTTON);
		const std::string role = TextOf(node.Get("color_role"));
		if (role == "matrix.status")
			m_Button->selection_color(FL_WHITE);
		else if (role == "meter.peak")
			m_Button->selection_color(FL_YELLOW);
		m_Button->clear_visible_focus();
		m_Button->callback(Changed, this);
		Adopt(m_Button);
	}

	void PanelLED::Load()
	{
		if (!Property())
			return;
		const double value = Physical();
		if (m_Latch)
		{
			if (value != 0)
				m_Button->value(1);
		}
		else
			m_Button->value(m_HasChoice ? value == m_Choice : value != 0);
	}

	void PanelLED::SetAttribute(const std::string &attribute, double)
	{
		if (attribute == "clear")
			m_Button->value(0);
	}

	void PanelLED::Changed(Fl_Widget *, void *data)
	{
		PanelLED *self = (PanelLED *)data;
		if (self->m_Latch)
		{
			self->m_Button->value(0);
			return;
		}
		if (self->m_HasChoice)
		{
			if (self->m_Button->value())
				self->Commit(self->m_Choice);
		}
		else
			self->Commit(self->m_Button->value() ? 1 : 0);
	}
}
