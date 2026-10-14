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
#include "PanelNotebook.h"
#include "Panel.h"
#include "PanelControlRegistry.h"
#include "JSON.h"
#include <FL/Fl_Tabs.H>
#include <FL/Fl_Group.H>

namespace SpiralPanels
{
	static PanelControlRegistry::Entry s_Entry("Notebook", &PanelNotebook::Create);

	PanelControl *PanelNotebook::Create(Panel &panel, const Spumoni::JSON &node)
	{
		return new PanelNotebook(panel, node);
	}

	PanelNotebook::PanelNotebook(Panel &panel, const Spumoni::JSON &node):
		PanelControl(panel, node)
	{
		const int tabs = (int)NumberOf(node.Get("tab_height"), 20);
		const int padding = (int)NumberOf(node.Get("page_padding"), 4);
		const int edge = padding > 0 ? 2 : 0;
		const int pageX = m_X + padding, pageY = m_Y + tabs + edge;
		const int pageW = m_W - 2 * padding, pageH = m_H - tabs - edge - padding;

		m_Tabs = new Fl_Tabs(m_X, m_Y, m_W, m_H);
		m_Tabs->labelsize(10);
		m_Tabs->box(FL_PLASTIC_DOWN_BOX);
		m_Tabs->color((Fl_Color)GUIColour());
		const Spumoni::JSON *pages = node.Get("pages");
		if (pages && pages->GetType() == Spumoni::JSON::Array)
		{
			for (size_t i = 0; i < pages->Size(); ++i)
			{
				const Spumoni::JSON &page = *pages->At(i);
				Fl_Group *group = new Fl_Group(pageX, pageY, pageW > 0 ? pageW : 0, pageH > 0 ? pageH : 0);
				group->copy_label(TextOf(page.Get("title")).c_str());
				group->labelsize(10);
				const Spumoni::JSON *children = page.Get("children");
				if (children)
					m_Panel.BuildChildren(*children, pageX, pageY);
				group->end();
				if (i > 0)
					group->hide();
			}
		}
		m_Tabs->end();
		Adopt(m_Tabs);
	}
}
