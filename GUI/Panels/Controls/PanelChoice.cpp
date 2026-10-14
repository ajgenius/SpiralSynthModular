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
#include "PanelChoice.h"
#include "Panel.h"
#include "PanelControlRegistry.h"
#include "PanelProperty.h"
#include "JSON.h"
#include <FL/Fl_Choice.H>

namespace SpiralPanels
{
	static PanelControlRegistry::Entry s_Entry("Choice", &PanelChoice::Create);

	PanelControl *PanelChoice::Create(Panel &panel, const Spumoni::JSON &node)
	{
		return new PanelChoice(panel, node);
	}

	PanelChoice::PanelChoice(Panel &panel, const Spumoni::JSON &node):
		PanelControl(panel, node)
	{
		m_Choice = new Fl_Choice(m_X, m_Y, m_W, m_H);
		m_Choice->textsize((int)NumberOf(node.Get("size"), 10));
		m_Choice->labelsize((int)NumberOf(node.Get("size"), 10));
		m_Choice->box(FL_PLASTIC_UP_BOX);
		const Spumoni::JSON *items = node.Get("items");
		bool valued = false;
		if (items && items->GetType() == Spumoni::JSON::Array)
		{
			for (size_t i = 0; i < items->Size(); ++i)
			{
				const Spumoni::JSON *item = items->At(i);
				int index;
				const bool thisValued = item->GetType() == Spumoni::JSON::Object && item->Get("value") != NULL;
				if (thisValued)
				{
					index = m_Choice->add(TextOf(item->Get("text")).c_str());
					valued = true;
				}
				else
				{
					index = m_Choice->add("x");
					m_Choice->replace(index, TextOf(item).c_str());
				}
				m_Indices.push_back(index);
				m_Values.resize(index + 1, 0);
				m_Values[index] = thisValued ? NumberOf(item->Get("value"), 0) : (double)i;
			}
		}
		if (!valued)
			m_Values.clear();
		m_Choice->callback(Changed, this);
		Adopt(m_Choice);
	}

	void PanelChoice::Load()
	{
		if (!Property())
			return;
		const double value = Physical();
		if (m_Values.empty())
			m_Choice->value((int)value);
		else
			for (size_t i = 0; i < m_Indices.size(); ++i)
				if (m_Values[m_Indices[i]] == value)
				{
					m_Choice->value(m_Indices[i]);
					break;
				}
	}

	void PanelChoice::Changed(Fl_Widget *, void *data)
	{
		PanelChoice *self = (PanelChoice *)data;
		const int index = self->m_Choice->value();
		self->Commit(self->m_Values.empty() || index < 0 || index >= (int)self->m_Values.size() ? index : self->m_Values[index]);
	}
}
