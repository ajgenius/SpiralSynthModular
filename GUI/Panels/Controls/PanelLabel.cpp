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
#include "PanelLabel.h"
#include "PanelControlRegistry.h"
#include "../Panel.h"
#include "JSON.h"
#include <FL/Fl_Box.H>

namespace SpiralPanels
{
	static PanelControlRegistry::Entry s_Entry("Label", &PanelLabel::Create);

	PanelControl *PanelLabel::Create(Panel &panel, const Spumoni::JSON &node)
	{
		return new PanelLabel(panel, node);
	}

	PanelLabel::PanelLabel(Panel &panel, const Spumoni::JSON &node):
		PanelControl(panel, node)
	{
		m_Box = new Fl_Box(m_X, m_Y, m_W, m_H);
		m_Box->copy_label(TextOf(node.Get("text")).c_str());
		m_Box->labelsize((int)NumberOf(node.Get("size"), 10));
		const std::string align = TextOf(node.Get("align"));
		m_Box->align(FL_ALIGN_INSIDE | (align == "left" ? FL_ALIGN_LEFT : align == "right" ? FL_ALIGN_RIGHT : FL_ALIGN_CENTER));
		const Spumoni::JSON *texts = node.Get("texts");
		if (texts && texts->GetType() == Spumoni::JSON::Array)
		{
			for (size_t i = 0; i < texts->Size(); ++i)
				m_Texts.push_back(TextOf(texts->At(i)));
			m_Box->box(FL_PLASTIC_UP_BOX);
			m_Box->color(FL_BACKGROUND2_COLOR);
		}
		Adopt(m_Box);
	}

	void PanelLabel::Load()
	{
		if (!Property() || m_Texts.empty())
			return;
		int index = (int)Physical();
		if (index < 0)
			index = 0;
		if (index >= (int)m_Texts.size())
			index = (int)m_Texts.size() - 1;
		m_Box->copy_label(m_Texts[index].c_str());
	}
}
