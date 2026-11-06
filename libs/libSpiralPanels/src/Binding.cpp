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
#include "Binding.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>

namespace Spiral
{
	namespace Data
	{
		Bindable::Bindable()
		{
		}

		Bindable::~Bindable()
		{
			while (!observers.empty())
			{
				BindableObserver *observer = observers.back();
				observers.pop_back();
				observer->BindableDestroyed(*this);
			}
		}

		void Bindable::Observe(BindableObserver *observer)
		{
			if (observer && std::find(observers.begin(), observers.end(), observer) == observers.end())
				observers.push_back(observer);
		}

		void Bindable::Unobserve(BindableObserver *observer)
		{
			observers.erase(std::remove(observers.begin(), observers.end(), observer), observers.end());
		}

		void Bindable::Changed(const Attribute &attribute)
		{
			const std::vector<BindableObserver *> snapshot = observers;
			for (size_t i = 0; i < snapshot.size(); ++i)
				if (std::find(observers.begin(), observers.end(), snapshot[i]) != observers.end())
					snapshot[i]->BindableChanged(*this, attribute);
		}

		bool ValueConverter::ToControl(const ControlValue &field, ControlValue::Type to, ControlValue &out) const
		{
			return Binding::Convert(field, to, out);
		}

		bool ValueConverter::ToField(const ControlValue &control, ControlValue::Type to, ControlValue &out) const
		{
			return Binding::Convert(control, to, out);
		}

		Binding::Binding(Bindable *target, const Attribute *endpoint, DataSource *source, int field, bool writes, ValueConverter *converter):
		    Target(target),
		    Endpoint(endpoint),
		    Source(source),
		    Field(field),
		    Writes(writes),
		    Wrote(false),
		    Converter(converter),
		    refreshing(false)
		{
			if (Target)
				Target->Observe(this);
			if (Source)
				Source->Observe(this);
		}

		Binding::~Binding()
		{
			if (Target)
				Target->Unobserve(this);
			if (Source)
				Source->Unobserve(this);
			delete Converter;
		}

		std::string Binding::Format(const ControlValue &value)
		{
			char buffer[64];
			switch (value.type)
			{
				case ControlValue::String:
					return value.text;
				case ControlValue::Boolean:
					return value.boolean ? "1" : "0";
				case ControlValue::Integer:
					std::snprintf(buffer, sizeof(buffer), "%.0f", value.number);
					return buffer;
				case ControlValue::Number:
				default:
					std::snprintf(buffer, sizeof(buffer), "%.15g", value.number);
					return buffer;
			}
		}

		bool Binding::Convert(const ControlValue &from, ControlValue::Type to, ControlValue &out)
		{
			if (from.type == to)
			{
				out = from;
				return true;
			}
			switch (to)
			{
				case ControlValue::String:
					out = ControlValue(Format(from));
					return true;
				case ControlValue::Boolean:
					if (from.type == ControlValue::String)
						out = ControlValue(from.text == "1" || from.text == "true" || from.text == "yes");
					else
						out = ControlValue(from.number != 0);
					return true;
				case ControlValue::Number:
				case ControlValue::Integer:
				{
					double number = 0;
					if (from.type == ControlValue::Boolean)
						number = from.boolean ? 1 : 0;
					else if (from.type == ControlValue::String)
					{
						char *end = NULL;
						number = std::strtod(from.text.c_str(), &end);
						if (end == from.text.c_str())
							return false;
					}
					else
						number = from.number;
					if (to == ControlValue::Integer)
						number = std::floor(number + 0.5);
					out = ControlValue(number);
					out.type = to;
					return true;
				}
			}
			return false;
		}

		void Binding::Refresh()
		{
			if (!Target || !Source || !Endpoint)
				return;
			ControlValue value, converted;
			if (!Source->Get(Field, value))
			{
				converted = Endpoint->Type == ControlValue::String ? ControlValue(std::string()) : ControlValue(0.0);
				converted.type = Endpoint->Type;
			}
			else if (!(Converter ? Converter->ToControl(value, Endpoint->Type, converted) : Convert(value, Endpoint->Type, converted)))
				return;
			refreshing = true;
			Target->SetAttribute(*Endpoint, converted);
			refreshing = false;
		}

		void Binding::BindableChanged(Bindable &bindable, const Attribute &attribute)
		{
			if (&bindable != Target || &attribute != Endpoint || !Writes || refreshing || !Source)
				return;
			ControlValue value, converted;
			const ControlValue::Type to = Source->Field(Field).Type;
			Wrote = Target->GetAttribute(*Endpoint, value) &&
			    (Converter ? Converter->ToField(value, to, converted) : Convert(value, to, converted)) &&
			    Source->Set(Field, converted);
			if (!Wrote)
				Refresh();
		}

		void Binding::BindableDestroyed(Bindable &)
		{
			Target = NULL;
		}

		void Binding::DataChanged(DataSource &, int field)
		{
			if (field < 0 || field == Field)
				Refresh();
		}

		void Binding::DataSourceDestroyed(DataSource &)
		{
			Source = NULL;
		}

		Binder::Binder()
		{
		}

		Binder::~Binder()
		{
			Clear();
		}

		Binding *Binder::Bind(Bindable *target, const char *attribute, DataSource *source, const std::string &field, ValueConverter *converter)
		{
			const Attribute *endpoint = !target ? NULL : attribute ? target->FindAttribute(attribute) : target->ValueAttribute();
			const int index = source ? source->FindField(field) : -1;
			if (!endpoint || index < 0)
			{
				delete converter;
				return NULL;
			}
			const bool writes = endpoint->Notifies && !source->Field(index).ReadOnly;
			Binding *binding = new Binding(target, endpoint, source, index, writes, converter);
			bindings.push_back(binding);
			binding->Refresh();
			return binding;
		}

		Binding *Binder::BindValue(Bindable *target, DataSource *source, const std::string &field, ValueConverter *converter)
		{
			return Bind(target, NULL, source, field, converter);
		}

		void Binder::Unbind(Bindable *target)
		{
			for (size_t i = bindings.size(); i-- > 0;)
				if (bindings[i]->Target == target)
				{
					delete bindings[i];
					bindings.erase(bindings.begin() + i);
				}
		}

		void Binder::Clear()
		{
			for (size_t i = 0; i < bindings.size(); ++i)
				delete bindings[i];
			bindings.clear();
		}

		void Binder::Refresh()
		{
			for (size_t i = 0; i < bindings.size(); ++i)
				bindings[i]->Refresh();
		}
	}
}
