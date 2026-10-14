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
#include "PanelLCD.h"
#include "Panel.h"
#include "PanelControlRegistry.h"
#include "PanelProperty.h"
#include "JSON.h"
#include <FL/Fl.H>
#include <FL/Fl_Output.H>
#include <cstdio>
#include <cstring>

namespace SpiralPanels
{
	static PanelControlRegistry::Entry s_Entry("LCD", &PanelLCD::Create);

	PanelControl *PanelLCD::Create(Panel &panel, const Spumoni::JSON &node)
	{
		return new PanelLCD(panel, node);
	}

	PanelLCD::PanelLCD(Panel &panel, const Spumoni::JSON &node):
		PanelControl(panel, node),
		m_Format(TextOf(node.Get("format"))),
		m_Empty(TextOf(node.Get("placeholder")))
	{
		m_Output = new Fl_Output(m_X, m_Y, m_W, m_H);
		m_Output->box(TextOf(node.Get("box")) == "up" ? FL_PLASTIC_UP_BOX : FL_PLASTIC_DOWN_BOX);
		m_Output->textsize((int)NumberOf(node.Get("size"), 14));
		if (m_Format.empty())
			m_Format = "%1.5f";
		Adopt(m_Output);
	}

	void PanelLCD::Load()
	{
		if (!Property())
			return;
		if (Property()->GetKind() == PanelProperty::String)
		{
			const std::string text = Text();
			m_Output->value(text.empty() ? m_Empty.c_str() : text.c_str());
			m_Output->position(text.empty() ? 0 : (int)text.size() - 1);
		}
		else
			SetAttribute("value", Value());
	}

	void PanelLCD::SetAttribute(const std::string &attribute, double value)
	{
		if (attribute != "value")
			return;
		char text[64];
		std::snprintf(text, sizeof(text), m_Format.c_str(), value);
		if (std::strcmp(text, m_Output->value()))
			m_Output->value(text);
	}
}
