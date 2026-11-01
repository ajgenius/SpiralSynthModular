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
#include "PropertySource.h"
#include "ChannelHandler.h"
#include "Device.h"
#include "Property.h"

#include <cmath>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <map>

namespace Spiral
{
	namespace Data
	{
		namespace
		{
			bool Finite(double value)
			{
				return value == value && std::fabs(value) <= std::numeric_limits<double>::max();
			}

			template <class T> bool StoreInteger(double &value, unsigned char *bytes)
			{
				const double limit = std::ldexp(1.0, std::numeric_limits<T>::digits);
				const double minimum = std::numeric_limits<T>::is_signed ? -limit : 0;
				if (value < minimum || value >= limit)
					return false;
				const T native = static_cast<T>(value);
				std::memcpy(bytes, &native, sizeof(native));
				value = native;
				return true;
			}

			double NumberOf(const ControlValue &value)
			{
				if (value.type == ControlValue::Boolean)
					return value.boolean ? 1 : 0;
				if (value.type == ControlValue::String)
					return std::strtod(value.text.c_str(), NULL);
				return value.number;
			}

			bool Same(const ControlValue &a, const ControlValue &b)
			{
				if (a.type != b.type)
					return false;
				if (a.type == ControlValue::String)
					return a.text == b.text;
				if (a.type == ControlValue::Boolean)
					return a.boolean == b.boolean;
				return a.number == b.number;
			}
		}

		PropertySource::PropertySource(Core::Device *device, Core::ChannelHandler *channels):
		    m_Device(device),
		    m_Channels(channels)
		{
		}

		PropertySource::~PropertySource()
		{
			Release();
		}

		void PropertySource::Release()
		{
			for (size_t i = 0; i < m_Slots.size(); ++i)
				delete m_Slots[i].Property;
			m_Slots.clear();
		}

		void PropertySource::Collect()
		{
			Release();
			if (!m_Device || !m_Channels)
				return;
			std::map<std::string, Core::Property *> registered;
			m_Device->DuplicateProperties(registered);
			for (std::map<std::string, Core::Property *>::iterator i = registered.begin(); i != registered.end(); ++i)
			{
				Core::Property *property = i->second;
				Slot slot;
				slot.Property = property;
				slot.Channel = property->GetLabel();
				slot.Size = property->IsInternal() ? -1 : m_Channels->ChannelSize(slot.Channel);
				slot.Known = false;
				const std::string name = property->ClassName();
				ControlValue::Type type = ControlValue::Number;
				bool usable = slot.Size > 0;
				if (usable && name == "FloatProperty")
					slot.Kind = Float;
				else if (usable && name == "SignedProperty")
					slot.Kind = Signed;
				else if (usable && name == "UnsignedProperty")
					slot.Kind = Unsigned;
				else if (usable && name == "ToggleProperty")
					slot.Kind = Toggle;
				else if (usable && name == "SetProperty")
				{
					const Core::NumericPropertyValue &value = static_cast<Core::NumericProperty *>(property)->Value;
					slot.Kind = value.Type == Core::NumericPropertyValue::Float ? Float
					    : value.Type == Core::NumericPropertyValue::Boolean ? Toggle
					    : value.Type == Core::NumericPropertyValue::Unsigned ? Unsigned : Signed;
				}
				else if (usable && name == "StringProperty" && slot.Size > 1)
					slot.Kind = Text;
				else
					usable = false;
				if (usable)
				{
					type = slot.Kind == Float ? ControlValue::Number
					    : slot.Kind == Toggle ? ControlValue::Boolean
					    : slot.Kind == Text ? ControlValue::String : ControlValue::Integer;
					usable = slot.Kind == Text || slot.Size == (int)sizeof(float) || slot.Size == (int)sizeof(double)
					    || slot.Size == (int)sizeof(bool);
				}
				if (!usable)
				{
					delete property;
					continue;
				}
				slot.Field = DataField(i->first, type, property->IsReadOnly(), property->GetLabel());
				m_Slots.push_back(slot);
			}
		}

		bool PropertySource::Read(const Slot &slot, ControlValue &out) const
		{
			if (slot.Kind == Text)
			{
				std::vector<char> text(slot.Size + 1, 0);
				if (!m_Channels->Peek(slot.Channel, 0, &text[0], slot.Size))
					return false;
				out = ControlValue(std::string(&text[0]));
				return true;
			}
			unsigned char bytes[8] = {0};
			if (slot.Size > (int)sizeof(bytes) || !m_Channels->Peek(slot.Channel, 0, bytes, slot.Size))
				return false;
			switch (slot.Kind)
			{
				case Float:
					if (slot.Size == (int)sizeof(double))
					{
						double v;
						std::memcpy(&v, bytes, sizeof(v));
						out = ControlValue(v);
					}
					else
					{
						float v;
						std::memcpy(&v, bytes, sizeof(v));
						out = ControlValue((double)v);
					}
					return true;
				case Signed:
				{
					int v;
					std::memcpy(&v, bytes, sizeof(v));
					out = ControlValue((double)v);
					out.type = ControlValue::Integer;
					return true;
				}
				case Unsigned:
				{
					unsigned int v;
					std::memcpy(&v, bytes, sizeof(v));
					out = ControlValue((double)v);
					out.type = ControlValue::Integer;
					return true;
				}
				case Toggle:
					out = ControlValue(bytes[0] != 0);
					return true;
				case Text:
					break;
			}
			return false;
		}

		bool PropertySource::Store(Slot &slot, ControlValue &value)
		{
			if (slot.Kind == Text)
			{
				std::vector<char> bytes(slot.Size, 0);
				std::strncpy(&bytes[0], value.text.c_str(), bytes.size() - 1);
				if (!m_Channels->SetDataAt(slot.Channel, 0, &bytes[0], (int)bytes.size()))
					return false;
				value = ControlValue(std::string(&bytes[0]));
				return true;
			}
			double number = NumberOf(value);
			if (!Finite(number))
				return false;
			Core::Property &property = *slot.Property;
			const std::string name = property.ClassName();
			if (name == "SetProperty")
			{
				Core::PropertySet set = static_cast<Core::SetProperty &>(property).Set();
				bool member = false;
				for (Core::UnsignedType i = 0; i < set.size() && !member; ++i)
				{
					const Core::NumericPropertyValue v = set[i];
					const double candidate = v.Type == Core::NumericPropertyValue::Float ? double(v.AsFloat)
					    : v.Type == Core::NumericPropertyValue::Boolean ? (v.AsBoolean ? 1.0 : 0.0)
					    : v.Type == Core::NumericPropertyValue::Unsigned ? double(v.AsUnsigned) : double(v.AsSigned);
					member = candidate == number;
				}
				if (!member)
					return false;
			}
			if (slot.Kind == Float && slot.Size != (int)sizeof(double) && std::fabs(number) > std::numeric_limits<float>::max())
				return false;
			if (slot.Kind == Signed || slot.Kind == Unsigned)
				number = number < 0 ? std::ceil(number - 0.5) : std::floor(number + 0.5);
			unsigned char bytes[8] = {0};
			switch (slot.Kind)
			{
				case Float:
					if (slot.Size == (int)sizeof(double))
						std::memcpy(bytes, &number, sizeof(number));
					else
					{
						const float native = (float)number;
						std::memcpy(bytes, &native, sizeof(native));
						number = native;
					}
					break;
				case Toggle:
					number = number != 0 ? 1 : 0;
					if (slot.Size == (int)sizeof(bool))
					{
						const bool native = number != 0;
						std::memcpy(bytes, &native, sizeof(native));
					}
					else if (!StoreInteger<unsigned int>(number, bytes))
						return false;
					break;
				case Signed:
					if (!StoreInteger<int>(number, bytes))
						return false;
					break;
				case Unsigned:
					if (!StoreInteger<unsigned int>(number, bytes))
						return false;
					break;
				case Text:
					return false;
			}
			if (!m_Channels->SetDataAt(slot.Channel, 0, bytes, slot.Size))
				return false;
			if (slot.Kind == Toggle)
				value = ControlValue(number != 0);
			else
			{
				value = ControlValue(number);
				value.type = slot.Kind == Float ? ControlValue::Number : ControlValue::Integer;
			}
			return true;
		}

		void PropertySource::Refresh()
		{
			for (size_t i = 0; i < m_Slots.size(); ++i)
			{
				ControlValue value;
				if (!Read(m_Slots[i], value))
					continue;
				if (m_Slots[i].Known && Same(value, m_Slots[i].Value))
					continue;
				m_Slots[i].Value = value;
				m_Slots[i].Known = true;
				Changed((int)i);
			}
		}

		Core::Property *PropertySource::Describe(int field) const
		{
			return field >= 0 && field < (int)m_Slots.size() ? m_Slots[field].Property : NULL;
		}

		bool PropertySource::ReadNow(int field, ControlValue &out) const
		{
			return field >= 0 && field < (int)m_Slots.size() && Read(m_Slots[field], out);
		}

		int PropertySource::FieldCount() const
		{
			return (int)m_Slots.size();
		}

		const DataField &PropertySource::Field(int index) const
		{
			return m_Slots[index].Field;
		}

		bool PropertySource::Get(int field, ControlValue &out) const
		{
			if (field < 0 || field >= (int)m_Slots.size())
				return false;
			const Slot &slot = m_Slots[field];
			if (slot.Known)
			{
				out = slot.Value;
				return true;
			}
			return Read(slot, out);
		}

		bool PropertySource::Set(int field, const ControlValue &value)
		{
			if (field < 0 || field >= (int)m_Slots.size() || m_Slots[field].Field.ReadOnly)
				return false;
			Slot &slot = m_Slots[field];
			ControlValue stored = value;
			if (!Store(slot, stored))
				return false;
			slot.Value = stored;
			slot.Known = true;
			Changed(field);
			return true;
		}

		long PropertySource::Count() const
		{
			return 1;
		}

		long PropertySource::Position() const
		{
			return 0;
		}

		bool PropertySource::Seek(long row)
		{
			return row == 0;
		}
	}
}
