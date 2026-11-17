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
#include "DeviceSource.h"
#include "SpiralPlugin.h"
#include "Device.h"
#include "Property.h"
#include "SampleFeed.h"
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <vector>

namespace Spiral
{
	namespace Panels
	{
		using Spiral::Data::ControlValue;

		static std::string Upper(const std::string &text)
		{
			std::string out(text);
			for (size_t i = 0; i < out.size(); ++i)
				out[i] = (char)std::toupper((unsigned char)out[i]);
			return out;
		}

		DeviceSource::DeviceSource(SpiralPlugin *plugin):
			Spiral::Data::PropertySource(dynamic_cast<Spiral::Core::Device *>(plugin), plugin ? plugin->GetChannelHandler() : NULL),
			m_Plugin(plugin)
		{
		}

		void DeviceSource::Collect()
		{
			m_Elements.clear();
			PropertySource::Collect();
		}

		static bool Fits(PanelProperty::Kind kind, int size)
		{
			switch (kind)
			{
				case PanelProperty::Number:
					return size == sizeof(float) || size == sizeof(double);
				case PanelProperty::Integer:
					return size == sizeof(char) || size == sizeof(short) || size == sizeof(int) || size == sizeof(long);
				case PanelProperty::Boolean:
					return size == sizeof(bool) || size == sizeof(int);
				case PanelProperty::String:
					return size > 1;
			}
			return false;
		}

		bool DeviceSource::AddElement(const PanelProperty &property)
		{
			ChannelHandler *ch = m_Plugin ? m_Plugin->GetChannelHandler() : NULL;
			if (!ch)
				return false;
			const std::string key = property.Name();
			ChannelHandler::Type type;
			Element e;
			e.Field = Spiral::Data::DataField(key, property.GetKind(), property.IsReadOnly(), property.Label);
			e.Known = false;
			e.Channel = ChannelNamed(key);
			int size = e.Channel.empty() ? -1 : ch->ChannelSize(e.Channel, &type);
			if (size > 0 && Fits(property.GetKind(), size))
			{
				e.Offset = 0;
				e.Size = size;
			}
			else
			{
				size_t digits = key.size();
				while (digits > 0 && std::isdigit((unsigned char)key[digits - 1]))
					--digits;
				if (digits == 0 || digits == key.size() || property.GetKind() == PanelProperty::String)
					return false;
				e.Channel = ChannelNamed(key.substr(0, digits));
				const int index = std::atoi(key.c_str() + digits);
				const int element = property.GetKind() == PanelProperty::Boolean ? (int)sizeof(bool) : 4;
				size = e.Channel.empty() ? -1 : ch->ChannelSize(e.Channel, &type);
				if (size <= 0 || size % element || (index + 1) * element > size)
					return false;
				e.Offset = index * element;
				e.Size = element;
			}
			if (type != ChannelHandler::INPUT)
				e.Field.ReadOnly = true;
			m_Elements.push_back(e);
			return true;
		}

		bool DeviceSource::ReadElement(const Element &element, ControlValue &out) const
		{
			ChannelHandler *ch = m_Plugin ? m_Plugin->GetChannelHandler() : NULL;
			if (!ch)
				return false;
			if (element.Field.Type == ControlValue::String)
			{
				std::vector<char> text(element.Size + 1, 0);
				if (!ch->Peek(element.Channel, element.Offset, &text[0], element.Size))
					return false;
				out = ControlValue(std::string(&text[0]));
				return true;
			}
			unsigned char bytes[8] = {0};
			if (element.Size > (int)sizeof(bytes) || !ch->Peek(element.Channel, element.Offset, bytes, element.Size))
				return false;
			if (element.Field.Type == ControlValue::Boolean)
				out = ControlValue(bytes[0] != 0);
			else if (element.Field.Type == ControlValue::Number)
			{
				if (element.Size == (int)sizeof(double))
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
			}
			else
			{
				double v = 0;
				if (element.Size == (int)sizeof(char)) v = (signed char)bytes[0];
				else if (element.Size == (int)sizeof(short)) { short s; std::memcpy(&s, bytes, sizeof(s)); v = s; }
				else if (element.Size == (int)sizeof(int)) { int i; std::memcpy(&i, bytes, sizeof(i)); v = i; }
				else { long l; std::memcpy(&l, bytes, sizeof(l)); v = (double)l; }
				out = ControlValue(v);
				out.type = ControlValue::Integer;
			}
			return true;
		}

		bool DeviceSource::WriteElement(const Element &element, const ControlValue &value)
		{
			ChannelHandler *ch = m_Plugin ? m_Plugin->GetChannelHandler() : NULL;
			if (!ch || element.Field.ReadOnly)
				return false;
			if (element.Field.Type == ControlValue::String)
			{
				std::vector<char> bytes(element.Size, 0);
				std::strncpy(&bytes[0], value.text.c_str(), bytes.size() - 1);
				return ch->SetDataAt(element.Channel, element.Offset, &bytes[0], (int)bytes.size());
			}
			const double number = value.type == ControlValue::Boolean ? (value.boolean ? 1 : 0) : value.number;
			unsigned char bytes[8] = {0};
			if (element.Field.Type == ControlValue::Number)
			{
				if (element.Size == (int)sizeof(double)) std::memcpy(bytes, &number, sizeof(number));
				else { const float f = (float)number; std::memcpy(bytes, &f, sizeof(f)); }
			}
			else if (element.Field.Type == ControlValue::Boolean && element.Size == (int)sizeof(bool))
				bytes[0] = number != 0;
			else
			{
				const long rounded = (long)(number < 0 ? number - 0.5 : number + 0.5);
				if (element.Size == (int)sizeof(char)) bytes[0] = (unsigned char)(signed char)rounded;
				else if (element.Size == (int)sizeof(short)) { const short s = (short)rounded; std::memcpy(bytes, &s, sizeof(s)); }
				else if (element.Size == (int)sizeof(int)) { const int i = (int)rounded; std::memcpy(bytes, &i, sizeof(i)); }
				else std::memcpy(bytes, &rounded, sizeof(rounded));
			}
			return ch->SetDataAt(element.Channel, element.Offset, bytes, element.Size);
		}

		void DeviceSource::Refresh()
		{
			PropertySource::Refresh();
			const int base = PropertySource::FieldCount();
			for (size_t i = 0; i < m_Elements.size(); ++i)
			{
				ControlValue value;
				if (!ReadElement(m_Elements[i], value))
					continue;
				const bool same = m_Elements[i].Known && value.type == m_Elements[i].Value.type
				    && value.number == m_Elements[i].Value.number && value.boolean == m_Elements[i].Value.boolean
				    && value.text == m_Elements[i].Value.text;
				if (same)
					continue;
				m_Elements[i].Value = value;
				m_Elements[i].Known = true;
				Changed(base + (int)i);
			}
		}

		int DeviceSource::FieldCount() const
		{
			return PropertySource::FieldCount() + (int)m_Elements.size();
		}

		const Spiral::Data::DataField &DeviceSource::Field(int index) const
		{
			const int base = PropertySource::FieldCount();
			return index < base ? PropertySource::Field(index) : m_Elements[index - base].Field;
		}

		bool DeviceSource::Get(int field, ControlValue &out) const
		{
			const int base = PropertySource::FieldCount();
			if (field < base)
				return PropertySource::Get(field, out);
			if (field - base >= (int)m_Elements.size())
				return false;
			const Element &element = m_Elements[field - base];
			if (element.Known)
			{
				out = element.Value;
				return true;
			}
			return ReadElement(element, out);
		}

		bool DeviceSource::Set(int field, const ControlValue &value)
		{
			const int base = PropertySource::FieldCount();
			if (field < base)
				return PropertySource::Set(field, value);
			if (field - base >= (int)m_Elements.size() || !WriteElement(m_Elements[field - base], value))
				return false;
			Element &element = m_Elements[field - base];
			ControlValue stored;
			if (ReadElement(element, stored))
			{
				element.Value = stored;
				element.Known = true;
			}
			Changed(field);
			return true;
		}

		void DeviceSource::Describe(std::map<std::string, PanelProperty *> &properties) const
		{
			for (int i = 0; i < FieldCount(); ++i)
			{
				Spiral::Core::Property *registered = PropertySource::Describe(i);
				PanelProperty *field = registered ? FromRegistered(*registered) : NULL;
				if (field && !properties.count(field->Name()))
					properties[field->Name()] = field;
				else
					delete field;
			}
		}

		bool DeviceSource::Write(const std::string &key, double value)
		{
			return Set(FindField(key), ControlValue(value));
		}

		bool DeviceSource::WriteText(const std::string &key, const std::string &text)
		{
			return Set(FindField(key), ControlValue(text));
		}

		bool DeviceSource::ReadNow(const PanelProperty &property, double &value) const
		{
			ControlValue out;
			const int field = FindField(property.Name());
			const int base = PropertySource::FieldCount();
			if (field >= base ? !(field - base < (int)m_Elements.size() && ReadElement(m_Elements[field - base], out))
			                  : !PropertySource::ReadNow(field, out))
				return false;
			value = out.type == ControlValue::Boolean ? (out.boolean ? 1 : 0) : out.number;
			return true;
		}

		std::string DeviceSource::ChannelNamed(const std::string &name) const
		{
			const int field = FindField(Upper(name));
			ChannelHandler *ch = m_Plugin ? m_Plugin->GetChannelHandler() : NULL;
			if (!ch)
				return std::string();
			std::vector<std::string> names;
			ch->ChannelNames(names);
			const std::string wanted = field >= 0 ? Upper(Field(field).Label) : Upper(name);
			for (size_t i = 0; i < names.size(); ++i)
				if (Upper(names[i]) == wanted)
					return names[i];
			return std::string();
		}

		bool DeviceSource::Command(const std::string &name, const std::vector<std::string> &args)
		{
			ChannelHandler *ch = m_Plugin ? m_Plugin->GetChannelHandler() : NULL;
			if (!ch)
				return false;
			std::map<std::string, Declared>::const_iterator command = m_Commands.find(name);
			if (command == m_Commands.end())
			{
				Declared declared;
				std::string letters;
				if (!ch->CommandArguments(name, declared.Args, &letters))
					return false;
				for (size_t a = 0; a < declared.Args.size(); ++a)
				{
					const char letter = a < letters.size() ? letters[a] : '?';
					declared.Typed.push_back(letter == 'i' || letter == 'f' || letter == 's');
					declared.Kinds.push_back(letter == 'f' ? PanelProperty::Number : letter == 's' ? PanelProperty::String : PanelProperty::Integer);
				}
				m_Commands[name] = declared;
				command = m_Commands.find(name);
			}
			std::string types;
			std::vector<char> data;
			for (size_t i = 0; i < args.size() && i < command->second.Args.size(); ++i)
			{
				const int width = ch->ChannelSize(command->second.Args[i]);
				PanelProperty::Kind kind = command->second.Typed[i] ? command->second.Kinds[i]
					: width > (int)sizeof(double) ? PanelProperty::String
					: args[i].find_first_not_of("-0123456789") == std::string::npos ? PanelProperty::Integer : PanelProperty::Number;
				if (kind == PanelProperty::String)
				{
					types += 's';
					data.insert(data.end(), args[i].c_str(), args[i].c_str() + args[i].size() + 1);
				}
				else if (kind == PanelProperty::Number)
				{
					types += 'f';
					const float value = (float)std::atof(args[i].c_str());
					data.insert(data.end(), (const char *)&value, (const char *)&value + sizeof(value));
				}
				else
				{
					types += 'i';
					const int value = std::atoi(args[i].c_str());
					data.insert(data.end(), (const char *)&value, (const char *)&value + sizeof(value));
				}
			}
			return ch->SendCommand(CommandRingBuffer::Command(name.c_str(), types.c_str(), data.empty() ? "" : &data[0], (unsigned int)data.size()));
		}

		bool DeviceSource::ReadBytes(const std::string &name, std::vector<unsigned char> &bytes) const
		{
			ChannelHandler *ch = m_Plugin ? m_Plugin->GetChannelHandler() : NULL;
			const std::string channel = name.empty() ? std::string() : ChannelNamed(name);
			if (!ch || channel.empty())
				return false;
			const int size = ch->ChannelSize(channel);
			if (size <= 0)
				return false;
			bytes.resize(size);
			return ch->Peek(channel, 0, &bytes[0], size);
		}

		// Only the part of the channel in use is read; its cells are real
		// numbers where the device registered it so, whole ones otherwise.
		bool DeviceSource::ReadTable(const std::string &name, std::vector<double> &cells) const
		{
			ChannelHandler *ch = m_Plugin ? m_Plugin->GetChannelHandler() : NULL;
			const std::string channel = name.empty() ? std::string() : ChannelNamed(name);
			if (!ch || channel.empty())
				return false;
			const bool real = ch->ChannelKind(channel) == 'f';
			const int width = real ? (int)sizeof(float) : (int)sizeof(int);
			const int used = ch->ChannelUsed(channel);
			if (used < 0)
				return false;
			std::vector<unsigned char> bytes(used - used % width);
			if (!bytes.empty() && !ch->Peek(channel, 0, &bytes[0], (int)bytes.size()))
				return false;
			cells.resize(bytes.size() / width);
			for (size_t i = 0; i < cells.size(); ++i)
			{
				if (real) { float v; std::memcpy(&v, &bytes[i * width], sizeof(v)); cells[i] = v; }
				else { int v; std::memcpy(&v, &bytes[i * width], sizeof(v)); cells[i] = v; }
			}
			return true;
		}

		bool DeviceSource::ReadFloats(const std::string &name, std::vector<float> &values) const
		{
			ChannelHandler *ch = m_Plugin ? m_Plugin->GetChannelHandler() : NULL;
			const std::string channel = name.empty() ? std::string() : ChannelNamed(name);
			if (!ch || channel.empty())
				return false;
			const int size = ch->ChannelSize(channel);
			if (size <= 0 || size % (int)sizeof(float))
				return false;
			values.resize(size / sizeof(float));
			return ch->Peek(channel, 0, &values[0], size);
		}

		bool DeviceSource::FetchFloats(const std::string &command, const std::string &sizeChannel, const std::string &bufferChannel, std::vector<float> &values)
		{
			ChannelHandler *ch = m_Plugin ? m_Plugin->GetChannelHandler() : NULL;
			if (!ch)
				return false;
			const std::string size = ChannelNamed(sizeChannel), buffer = ChannelNamed(bufferChannel);
			if (size.empty() || buffer.empty())
				return false;
			if (!Command(command, std::vector<std::string>()))
				return false;
			do
				ch->Wait();
			while (ch->CommandsQueued());
			ch->RequestChannelAndWait(size);
			const long count = ch->GetLong(size);
			values.resize(count > 0 ? count : 0);
			if (count > 0)
				ch->BulkTransfer(buffer, &values[0], (int)(count * sizeof(float)));
			return true;
		}

		void DeviceSource::Wait()
		{
			if (m_Plugin && m_Plugin->GetChannelHandler())
				m_Plugin->GetChannelHandler()->Wait();
		}

		namespace
		{
			class SampleFeedOf: public Feed
			{
			public:
				explicit SampleFeedOf(Spiral::Core::SampleFeed *feed): m_Feed(feed) {}
				bool Take(std::vector<float> &out) { return m_Feed->Take(out); }
				Spiral::Core::SampleFeed *m_Feed;
			};
		}

		Feed *DeviceSource::RequestFeed(bool output, int port)
		{
			Spiral::Core::SampleFeed *feed = m_Plugin ? m_Plugin->RequestFeed(output, port) : NULL;

			return feed ? new SampleFeedOf(feed) : NULL;
		}

		void DeviceSource::ReleaseFeed(Feed *feed)
		{
			SampleFeedOf *of = static_cast<SampleFeedOf *>(feed);

			if (of && m_Plugin)
				m_Plugin->ReleaseFeed(of->m_Feed);

			delete of;
		}

		unsigned DeviceSource::Colour() const
		{
			const HostInfo *host = m_Plugin ? m_Plugin->GetHostInfo() : NULL;

			return host ? host->GUI_COLOUR : 179;
		}

		PanelProperty *DeviceSource::FromRegistered(Spiral::Core::Property &property)
		{
			using namespace Spiral::Core;

			if (property.IsInternal())
				return NULL;

			const std::string name = property.ClassName();
			PanelProperty::Kind kind;
			int storage;

			if (name == "FloatProperty")
			{
				kind = PanelProperty::Number;
				storage = sizeof(FloatType);
			}
			else if (name == "SignedProperty" || name == "UnsignedProperty" || name == "SetProperty")
			{
				kind = PanelProperty::Integer;
				storage = sizeof(int);
			}
			else if (name == "ToggleProperty")
			{
				kind = PanelProperty::Boolean;
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
				field->Type = numeric.Value.Type == NumericPropertyValue::Float ? PanelProperty::Number
				    : numeric.Value.Type == NumericPropertyValue::Boolean ? PanelProperty::Boolean : PanelProperty::Integer;

				PropertySet values = static_cast<SetProperty &>(property).Set();

				for (unsigned i = 0; i < values.size(); ++i)
				{
					const NumericPropertyValue value = values[i];
					field->m_Values.push_back(value.Type == NumericPropertyValue::Float ? double(value.AsFloat)
					    : value.Type == NumericPropertyValue::Boolean ? (value.AsBoolean ? 1.0 : 0.0)
					    : value.Type == NumericPropertyValue::Unsigned ? double(value.AsUnsigned) : double(value.AsSigned));
				}
			}

			if (field->Type == PanelProperty::Number)
				field->m_Default = numeric.Value.AsFloat;
			else if (field->Type == PanelProperty::Boolean)
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
	}
}
