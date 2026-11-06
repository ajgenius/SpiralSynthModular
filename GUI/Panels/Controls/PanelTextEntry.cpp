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
#include "PanelTextEntry.h"
#include "Panel.h"
#include "PanelControlRegistry.h"
#include "PanelProperty.h"
#include "JSON.h"
#include <FL/Fl_Input.H>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <limits>

namespace SpiralPanels
{
	static PanelControlRegistry::Entry s_Entry("TextEntry", &PanelTextEntry::Create);

	PanelControl *PanelTextEntry::Create(Panel &panel, const Spumoni::JSON &node)
	{
		return new PanelTextEntry(panel, node);
	}

	PanelTextEntry::PanelTextEntry(Panel &panel, const Spumoni::JSON &node):
		PanelControl(panel, node),
		m_Format(TextOf(node.Get("format")))
	{
		if (m_Format.empty())
			m_Format = "%g";
		const Spumoni::JSON *below = node.Get("below"), *above = node.Get("above");
		m_Below = below ? panel.FindProperty(TextOf(below->Get("property"))) : NULL;
		m_Above = above ? panel.FindProperty(TextOf(above->Get("property"))) : NULL;
		m_Input = new Fl_Input(m_X, m_Y, m_W, m_H);
		m_Input->textsize(10);
		if (TextOf(node.Get("box")) == "plastic")
			m_Input->box(FL_PLASTIC_DOWN_BOX);
		m_Input->when(FL_WHEN_ENTER_KEY | FL_WHEN_RELEASE);
		m_Input->callback(Changed, this);
		Adopt(m_Input);
	}

	void PanelTextEntry::Load()
	{
		if (!Property())
			return;
		if (Property()->GetKind() == PanelProperty::String)
		{
			m_Input->value(Text().c_str());
			return;
		}
		char text[64];
		std::snprintf(text, sizeof(text), m_Format.c_str(), Physical());
		m_Input->value(text);
	}

	void PanelTextEntry::Changed(Fl_Widget *, void *data)
	{
		PanelTextEntry *self = (PanelTextEntry *)data;
		if (!self->Property())
			return;
		if (self->Property()->GetKind() == PanelProperty::String)
		{
			self->CommitText(self->m_Input->value());
			return;
		}
		const char *text = self->m_Input->value();
		char *end = NULL;
		const double value = std::strtod(text, &end);
		while (*end && std::isspace((unsigned char)*end))
			++end;
		if (end == text || *end || !(std::fabs(value) <= std::numeric_limits<float>::max()))
			self->Load();
		else
			self->Commit(self->Ordered(value));
	}

	double PanelTextEntry::Ordered(double value)
	{
		const PanelProperty *partner = m_Below ? m_Below : m_Above;
		if (!partner)
			return value;

		const double limit = m_Panel.Read(*partner);
		if (m_Below ? value <= limit : value >= limit)
			return value;

		m_Panel.Write(*partner, value);
		return limit;
	}
}
