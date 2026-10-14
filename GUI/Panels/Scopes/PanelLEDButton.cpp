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
#include "PanelLEDButton.h"
#include "Panel.h"
#include "PanelControlRegistry.h"
#include "PanelProperty.h"
#include "JSON.h"
#include "Fl_LED_Button.H"

namespace SpiralPanels
{
	static PanelControlRegistry::Entry s_Entry("LEDButton", &PanelLEDButton::Create);

	PanelControl *PanelLEDButton::Create(Panel &panel, const Spumoni::JSON &node)
	{
		return new PanelLEDButton(panel, node);
	}

	PanelLEDButton::PanelLEDButton(Panel &panel, const Spumoni::JSON &node):
		PanelControl(panel, node),
		m_HasChoice(node.Get("choice") != NULL),
		m_Choice(NumberOf(node.Get("choice"), 0))
	{
		m_Button = new Fl_LED_Button(m_X, m_Y, m_W, m_H);
		if (node.Get("down") && node.Get("down")->AsBool())
			m_Button->value(1);
		const std::string role = TextOf(node.Get("color_role"));
		if (role == "status.on" || role == "record")
			m_Button->color(FL_RED);
		m_Button->copy_label(TextOf(node.Get("text")).c_str());
		m_Button->labelsize((int)NumberOf(node.Get("size"), 10));
		if (m_HasChoice)
			m_Button->type(FL_RADIO_BUTTON);
		else
			m_Button->type(FL_TOGGLE_BUTTON);
		m_Button->align(FL_ALIGN_INSIDE | FL_ALIGN_LEFT);
		m_Button->callback(Changed, this);
		Adopt(m_Button);
	}

	void PanelLEDButton::Load()
	{
		if (!Property())
			return;
		const double value = Physical();
		m_Button->value(m_HasChoice ? value == m_Choice : value != 0);
	}

	void PanelLEDButton::Changed(Fl_Widget *, void *data)
	{
		PanelLEDButton *self = (PanelLEDButton *)data;
		if (self->IsToggle() && !self->m_Button->value())
		{
			self->Fired(0, std::string());
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
	void PanelLEDButton::DeclareEvents(std::vector<Event> &out) const
	{
		static const Event pressed = {"pressed", {"value", "text", NULL}};
		static const Event on = {"on", {"value", "text", NULL}};
		static const Event off = {"off", {"value", "text", NULL}};
		PanelControl::DeclareEvents(out);
		out.push_back(pressed);
		out.push_back(on);
		out.push_back(off);
	}
}
