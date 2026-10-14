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
#include "PanelScope.h"
#include "ScopeView.h"
#include "Panel.h"
#include "PanelControlRegistry.h"
#include "DeviceSource.h"
#include "SampleFeed.h"
#include "JSON.h"
#include <cstdlib>

namespace SpiralPanels
{
	static PanelControlRegistry::Entry s_Entry("Scope", &PanelScope::Create);

	PanelControl *PanelScope::Create(Panel &panel, const Spumoni::JSON &node)
	{
		return new PanelScope(panel, node);
	}

	PanelScope::PanelScope(Panel &panel, const Spumoni::JSON &node):
		PanelControl(panel, node),
		m_Feed(NULL)
	{
		const Spumoni::JSON *samples = node.Get("samples");
		if (samples)
		{
			bool output = false;
			int port = (int)NumberOf(samples->Get("shadow"), 0);
			const std::string named = TextOf(samples->Get("port"));
			if (named.compare(0, 3, "in:") == 0)
				port = std::atoi(named.c_str() + 3);
			else if (named.compare(0, 4, "out:") == 0)
			{
				output = true;
				port = std::atoi(named.c_str() + 4);
			}
			m_Feed = m_Panel.Source()->RequestFeed(output, port);
		}
		m_View = new ScopeView(m_X, m_Y, m_W, m_H);
		m_View->SetTimeBase(NumberOf(node.Get("time_base"), 1));
		m_View->SetAttenuation(NumberOf(node.Get("attenuation"), 0));
		Adopt(m_View);
	}

	PanelScope::~PanelScope()
	{
		m_Panel.Source()->ReleaseFeed(m_Feed);
	}

	void PanelScope::Refresh()
	{
		if (m_Feed && m_Feed->Take(m_Samples))
			m_View->SetSamples(m_Samples);
	}

	void PanelScope::SetAttribute(const std::string &attribute, double value)
	{
		if (attribute == "attenuation")
			m_View->SetAttenuation(value);
		else if (attribute == "time_base")
			m_View->SetTimeBase(value);
	}

	bool PanelScope::GetAttribute(const std::string &attribute, double &value) const
	{
		if (attribute == "attenuation")
			value = m_View->Attenuation();
		else if (attribute == "time_base")
			value = m_View->TimeBase();
		else
			return false;
		return true;
	}
}
