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
#ifndef SPIRAL_PANELS_PANEL_CONTROL_REGISTRY_H
#define SPIRAL_PANELS_PANEL_CONTROL_REGISTRY_H

#include <map>
#include <string>

namespace Spumoni
{
	class JSON;
}

namespace Spiral
{
	namespace Panels
	{
		class Panel;
		class PanelControl;

		class PanelControlRegistry
		{
		public:
			typedef PanelControl *(*Factory)(Panel &, const Spumoni::JSON &);

			class Entry
			{
			public:
				Entry(const char *type, Factory factory)
				{
					PanelControlRegistry::Get().Register(type, factory);
				}
			};

			static PanelControlRegistry &Get();

			void Register(const std::string &type, Factory factory);
			bool Has(const std::string &type) const;
			PanelControl *Create(const std::string &type, Panel &panel, const Spumoni::JSON &node) const;

		protected:
			PanelControlRegistry() {}

			std::map<std::string, Factory> m_Factories;
		};
	}
}

#endif
