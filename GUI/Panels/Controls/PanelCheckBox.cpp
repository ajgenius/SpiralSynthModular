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
#include "PanelCheckBox.h"
#include "Panel.h"
#include "PanelControlRegistry.h"
#include "PanelProperty.h"
#include "JSON.h"
#include <FL/Fl_Check_Button.H>

namespace SpiralPanels
{
	static PanelControlRegistry::Entry s_Entry("CheckBox", &PanelCheckBox::Create);

	PanelControl *PanelCheckBox::Create(Panel &panel, const Spumoni::JSON &node)
	{
		return new PanelCheckBox(panel, node);
	}

	PanelCheckBox::PanelCheckBox(Panel &panel, const Spumoni::JSON &node):
		PanelControl(panel, node),
		m_HasChoice(node.Get("choice") != NULL),
		m_Choice(NumberOf(node.Get("choice"), 0))
	{
		m_Button = new Fl_Check_Button(m_X, m_Y, m_W, m_H);
		m_Button->copy_label(TextOf(node.Get("text")).c_str());
		m_Button->labelsize((int)NumberOf(node.Get("size"), 10));

		m_Button->callback(Changed, this);
		Adopt(m_Button);
	}

	void PanelCheckBox::Load()
	{
		if (!Property())
			return;
		const double value = Physical();
		m_Button->value(m_HasChoice ? value == m_Choice : value != 0);
	}

	void PanelCheckBox::Changed(Fl_Widget *, void *data)
	{
		PanelCheckBox *self = (PanelCheckBox *)data;
		if (self->m_HasChoice)
		{
			if (self->m_Button->value())
				self->Commit(self->m_Choice);
		}
		else
			self->Commit(self->m_Button->value() ? 1 : 0);
	}
}
