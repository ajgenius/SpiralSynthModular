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
#include "Property.h"
#include <cstdlib>
#include <cstring>
#include <algorithm>

namespace SpiralPanels
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

	PanelProperty *PanelProperty::FromRegistered(Spiral::Core::Property &property)
	{
		using namespace Spiral::Core;

		if (property.IsInternal())
			return NULL;

		const std::string name = property.ClassName();
		Kind kind;
		int storage;

		if (name == "FloatProperty")
		{
			kind = Number;
			storage = sizeof(FloatType);
		}
		else if (name == "SignedProperty" || name == "UnsignedProperty" || name == "SetProperty")
		{
			kind = Integer;
			storage = sizeof(int);
		}
		else if (name == "ToggleProperty")
		{
			kind = Boolean;
			storage = sizeof(bool);
		}
		else
			return NULL;

		PanelProperty *field = new PanelProperty(property.GetKey(), kind);
		field->Label = property.GetLabel();
		field->ReadOnly = property.IsReadOnly();
		field->m_StorageSize = storage;
		field->m_Unsigned = name == "UnsignedProperty";

		NumericProperty &numeric = static_cast<NumericProperty &>(property);

		if (name == "SetProperty")
		{
			field->m_IsSet = true;
			field->m_Unsigned = numeric.Value.Type == NumericPropertyValue::Unsigned;
			field->Type = numeric.Value.Type == NumericPropertyValue::Float ? Number
			    : numeric.Value.Type == NumericPropertyValue::Boolean ? Boolean : Integer;

			PropertySet values = static_cast<SetProperty &>(property).Set();

			for (unsigned i = 0; i < values.size(); ++i)
			{
				const NumericPropertyValue value = values[i];
				field->m_Values.push_back(value.Type == NumericPropertyValue::Float ? double(value.AsFloat)
				    : value.Type == NumericPropertyValue::Boolean ? (value.AsBoolean ? 1.0 : 0.0)
				    : value.Type == NumericPropertyValue::Unsigned ? double(value.AsUnsigned) : double(value.AsSigned));
			}
		}

		if (field->Type == Number)
			field->m_Default = numeric.Value.AsFloat;
		else if (field->Type == Boolean)
			field->m_Default = numeric.Value.AsBoolean ? 1 : 0;
		else if (field->m_Unsigned)
			field->m_Default = numeric.Value.AsUnsigned;
		else
			field->m_Default = numeric.Value.AsSigned;

		if (name == "FloatProperty")
		{
			LinearProperty &linear = static_cast<LinearProperty &>(property);
			field->m_Min = linear.Min.AsFloat;
			field->m_Max = linear.Max.AsFloat;
			field->m_Step = linear.SmallStep.AsFloat;
			field->m_Page = linear.Step.AsFloat;
		}
		else if (name == "UnsignedProperty")
		{
			LinearProperty &linear = static_cast<LinearProperty &>(property);
			field->m_Min = linear.Min.AsUnsigned;
			field->m_Max = linear.Max.AsUnsigned;
			field->m_Step = linear.SmallStep.AsUnsigned;
			field->m_Page = linear.Step.AsUnsigned;
		}
		else if (name == "SignedProperty")
		{
			LinearProperty &linear = static_cast<LinearProperty &>(property);
			field->m_Min = linear.Min.AsSigned;
			field->m_Max = linear.Max.AsSigned;
			field->m_Step = linear.SmallStep.AsSigned;
			field->m_Page = linear.Step.AsSigned;
		}

		return field;
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

namespace SpiralPanels
{
	const PanelProperty::Kind PanelProperty::Number;
	const PanelProperty::Kind PanelProperty::Integer;
	const PanelProperty::Kind PanelProperty::Boolean;
	const PanelProperty::Kind PanelProperty::String;
}
