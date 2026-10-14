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
#include "PanelVirtualScrollArea.h"
#include "Panel.h"
#include "PanelControlRegistry.h"
#include "Fl_VirtualScrollArea.H"
#include "JSON.h"

namespace SpiralPanels
{
	static PanelControlRegistry::Entry s_Entry("VirtualScrollArea", &PanelVirtualScrollArea::Create);

	PanelControl *PanelVirtualScrollArea::Create(Panel &panel, const Spumoni::JSON &node)
	{
		return new PanelVirtualScrollArea(panel, node);
	}

	static bool Flag(const Spumoni::JSON *node, bool fallback)
	{
		if (!node)
			return fallback;
		if (node->GetType() == Spumoni::JSON::Boolean)
			return node->AsBool();
		return PanelControl::NumberOf(node, fallback ? 1 : 0) != 0;
	}

	PanelVirtualScrollArea::PanelVirtualScrollArea(Panel &panel, const Spumoni::JSON &node):
		PanelControl(panel, node)
	{
		m_Area = new Fl_VirtualScrollArea(m_X, m_Y, m_W, m_H);
		m_Area->Show(Flag(node.Get("horizontal"), true), Flag(node.Get("vertical"), false),
			Flag(node.Get("always"), false));
		if (TextOf(node.Get("box")) == "plastic")
			m_Area->box(FL_PLASTIC_UP_BOX);
		const Spumoni::JSON *children = node.Get("children");
		if (children)
			m_Panel.BuildChildren(*children, m_X, m_Y);
		const Spumoni::JSON *size = node.Get("content_size");
		const Spumoni::JSON *view = node.Get("view");
		m_Area->end();
		m_Area->SetContentSize(size && size->Size() == 2 ? (int)NumberOf(size->At(0), 0) : 0,
			size && size->Size() == 2 ? (int)NumberOf(size->At(1), 0) : 0);
		if (view && view->Size() == 2)
			m_Area->SetView((int)NumberOf(view->At(0), 0), (int)NumberOf(view->At(1), 0));
		Adopt(m_Area);
	}
}
