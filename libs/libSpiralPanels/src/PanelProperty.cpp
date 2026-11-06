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
#include "PanelProperty.h"
#include "JSON.h"
#include <cstdlib>
#include <cstring>
#include <algorithm>

namespace Spiral
{
	namespace Panels
	{
		static double NumberOf(const Spumoni::JSON *node, double fallback)
		{
			if (!node)
				return fallback;
			if (node->GetType() == Spumoni::JSON::Number)
				return std::atof(node->Text().c_str());
			if (node->GetType() == Spumoni::JSON::Boolean)
				return node->AsBool() ? 1.0 : 0.0;
			return fallback;
		}

		PanelProperty::PanelProperty(const std::string &name, Kind kind):
			Key(name),
			Type(kind),
			ReadOnly(false),
			m_Min(0),
			m_Max(0),
			m_Step(0),
			m_Page(0),
			m_Default(0),
			m_StorageSize(0),
			m_Unsigned(false),
			m_IsSet(false),
			m_IsCommand(name == "COMMAND" && kind == String)
		{
		}

		bool PanelProperty::Accepts(double value) const
		{
			return !m_IsSet || std::find(m_Values.begin(), m_Values.end(), value) != m_Values.end();
		}

		PanelProperty *PanelProperty::FromJSON(const Spumoni::JSON &node)
		{
			const Spumoni::JSON *name = node.Get("property");
			const Spumoni::JSON *type = node.Get("type");
			if (!name || name->GetType() != Spumoni::JSON::String || !type)
				return NULL;

			Kind kind;
			if (type->Text() == "number")
				kind = Number;
			else if (type->Text() == "integer")
				kind = Integer;
			else if (type->Text() == "boolean")
				kind = Boolean;
			else if (type->Text() == "string")
				kind = String;
			else
				return NULL;

			PanelProperty *property = new PanelProperty(name->Text(), kind);
			const Spumoni::JSON *readOnly = node.Get("read_only");
			property->ReadOnly = readOnly && readOnly->GetType() == Spumoni::JSON::Boolean && readOnly->AsBool();
			property->m_Min = NumberOf(node.Get("min"), 0);
			property->m_Max = NumberOf(node.Get("max"), 0);
			property->m_Step = NumberOf(node.Get("step"), 0);
			property->m_Page = NumberOf(node.Get("page"), 0);
			property->m_Default = NumberOf(node.Get("default"), 0);

			return property;
		}
	}
}

namespace Spiral
{
	namespace Panels
	{
		const PanelProperty::Kind PanelProperty::Number;
		const PanelProperty::Kind PanelProperty::Integer;
		const PanelProperty::Kind PanelProperty::Boolean;
		const PanelProperty::Kind PanelProperty::String;
	}
}
