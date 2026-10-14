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
#include "PanelButton.h"
#include "PanelListChoosingButton.h"
#include "Panel.h"
#include "PanelControlRegistry.h"
#include "PanelProperty.h"
#include "JSON.h"
#include <FL/Fl_Button.H>
#include <FL/Fl_Repeat_Button.H>
#include <utility>

namespace SpiralPanels
{
	static PanelControlRegistry::Entry s_Entry("Button", &PanelButton::Create);

	PanelControl *PanelButton::Create(Panel &panel, const Spumoni::JSON &node)
	{
		if (node.Get("list"))
			return PanelListChoosingButton::Create(panel, node);
		return new PanelButton(panel, node);
	}

	PanelButton::PanelButton(Panel &panel, const Spumoni::JSON &node):
		PanelControl(panel, node),
		m_TextBound(false),
		m_Text(TextOf(node.Get("text"))),
		m_HasChoice(node.Get("choice") != NULL),
		m_Choice(NumberOf(node.Get("choice"), 0))
	{
		const Spumoni::JSON *text = node.Get("text");
		m_TextBound = RuleOf(panel, Spiral::Data::ValueRule::Looks(text) ? text : node.Get("text_when"), "text",
				     TextOf(node.Get("name")), m_TextRule);
		if (m_TextBound)
			m_Text = m_TextRule.Evaluate(Panel::Scope(panel)).text;
		const Spumoni::JSON *repeat = node.Get("repeat");
		const bool repeats = repeat && (repeat->GetType() == Spumoni::JSON::Object ||
						(repeat->GetType() == Spumoni::JSON::Boolean && repeat->AsBool()));
		if (repeats)
			m_Button = new Fl_Repeat_Button(m_X, m_Y, m_W, m_H);
		else
			m_Button = new Fl_Button(m_X, m_Y, m_W, m_H);
		m_Button->copy_label(m_Text.c_str());
		m_Button->labelsize((int)NumberOf(node.Get("size"), 10));
		if (TextOf(node.Get("label")) == "symbol")
			m_Button->labeltype(FL_SYMBOL_LABEL);
		if (m_HasChoice)
			m_Button->type(FL_RADIO_BUTTON);
		else if (Property() || IsToggle() || (node.Get("toggle") && node.Get("toggle")->AsBool()))
			m_Button->type(FL_TOGGLE_BUTTON);
		if (node.Get("pressed") && node.Get("pressed")->AsBool())
			m_Button->value(1);
		if (TextOf(node.Get("box")) == "plain")
			m_Button->box(FL_UP_BOX);
		else if (TextOf(node.Get("box")) == "thin")
			m_Button->box(FL_THIN_UP_BOX);
		else
		{
			m_Button->box(FL_PLASTIC_UP_BOX);
			m_Button->color((Fl_Color)GUIColour());
			m_Button->selection_color((Fl_Color)GUIColour());
		}
		if (TextOf(node.Get("color_role")) == "record")
		{
			if (m_Button->box() == FL_PLASTIC_UP_BOX)
			{
				m_Button->color(FL_RED);
				m_Button->selection_color(FL_RED);
			}
			else
				m_Button->labelcolor(FL_RED);
		}
		m_Button->callback(Changed, this);
		Adopt(m_Button);
	}

	void PanelButton::Load()
	{
		if (!Property())
			return;
		const double value = Physical();
		m_Button->value(m_HasChoice ? value == m_Choice : value != 0);
	}

	void PanelButton::Tick()
	{
		PanelControl::Tick();
		if (m_TextBound)
			m_Button->copy_label(m_TextRule.Evaluate(Panel::Scope(m_Panel)).text.c_str());
	}

	void PanelButton::Changed(Fl_Widget *, void *data)
	{
		PanelButton *self = (PanelButton *)data;
		if (self->IsToggle() && !self->m_Button->value())
		{
			self->Fired(0, self->m_Text);
			return;
		}
		if (self->HasHandlers() && !self->Property())
		{
			self->Fired(self->m_Button->value() ? 1 : 0, self->m_Text);
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
	void PanelButton::DeclareEvents(std::vector<Event> &out) const
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
