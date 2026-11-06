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
#ifndef SPIRAL_PANELS_PANEL_PROPERTY_H
#define SPIRAL_PANELS_PANEL_PROPERTY_H

#include "DataSource.h"
#include <string>
#include <map>
#include <vector>

namespace Spumoni
{
	class JSON;
}

namespace Spiral
{
	namespace Panels
	{
		class PanelProperty
		{
		public:
			typedef Spiral::Data::ControlValue::Type Kind;
			static const Kind Number = Spiral::Data::ControlValue::Number;
			static const Kind Integer = Spiral::Data::ControlValue::Integer;
			static const Kind Boolean = Spiral::Data::ControlValue::Boolean;
			static const Kind String = Spiral::Data::ControlValue::String;

			static PanelProperty *FromJSON(const Spumoni::JSON &node);

			std::string Key;
			Kind Type;
			bool ReadOnly;
			std::string Label;

			bool IsCommand() const { return m_IsCommand; }
			const std::string &Name() const { return Key; }
			Kind GetKind() const { return Type; }
			bool IsReadOnly() const { return ReadOnly; }
			bool HasRange() const { return m_Max > m_Min; }
			double Min() const { return m_Min; }
			double Max() const { return m_Max; }
			double Step() const { return m_Step; }
			double Page() const { return m_Page; }
			double Default() const { return m_Default; }
			int StorageSize() const { return m_StorageSize; }
			bool IsUnsigned() const { return m_Unsigned; }
			bool Accepts(double value) const;

		protected:
			friend class DeviceSource;
			PanelProperty(const std::string &name, Kind kind);
			double m_Min, m_Max, m_Step, m_Page, m_Default;
			int m_StorageSize;
			bool m_Unsigned;
			bool m_IsSet;
			bool m_IsCommand;
			std::vector<double> m_Values;
		};
	}
}

#endif
