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
#include "PanelControlRegistry.h"

namespace Spiral
{
	namespace Panels
	{
		PanelControlRegistry &PanelControlRegistry::Get()
		{
			static PanelControlRegistry *registry = new PanelControlRegistry();
			return *registry;
		}

		void PanelControlRegistry::Register(const std::string &type, Factory factory)
		{
			if (factory && m_Factories.find(type) == m_Factories.end())
				m_Factories[type] = factory;
		}

		bool PanelControlRegistry::Has(const std::string &type) const
		{
			return m_Factories.find(type) != m_Factories.end();
		}

		PanelControl *PanelControlRegistry::Create(const std::string &type, Panel &panel,
			const Spumoni::JSON &node) const
		{
			std::map<std::string, Factory>::const_iterator i = m_Factories.find(type);
			return i == m_Factories.end() ? NULL : i->second(panel, node);
		}
	}
}
