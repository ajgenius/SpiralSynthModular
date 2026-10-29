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

#ifndef SPIRAL_PATCH_DESCRIPTION
#define SPIRAL_PATCH_DESCRIPTION

#include <map>
#include <string>
#include <vector>

namespace Spiral
{
namespace Core
{
	class PropertyDescription
	{
	public:
		PropertyDescription() {}
		PropertyDescription(const std::string &name, const std::string &type,
		                    const std::string &className) :
			Name(name), Type(type), PropertyClassName(className) {}

		std::string Name;
		std::string Type;
		std::string PropertyClassName;

		std::map<std::string, std::string> Values;

		std::vector<PropertyDescription> Children;

		std::string Value() const
		{
			std::map<std::string, std::string>::const_iterator i = Values.find("value");
			return i == Values.end() ? std::string() : i->second;
		}

		bool HasValue() const { return Values.find("value") != Values.end(); }

		void SetValue(const std::string &text) { Values["value"] = text; }
		void SetValue(const std::string &key, const std::string &text) { Values[key] = text; }

		PropertyDescription &AddChild()
		{
			Children.push_back(PropertyDescription());
			return Children.back();
		}
	};

	class DeviceDescription
	{
	public:
		std::map<std::string, std::string> Values;

		std::vector<PropertyDescription> Properties;

		std::string Value(const std::string &key) const
		{
			std::map<std::string, std::string>::const_iterator i = Values.find(key);
			return i == Values.end() ? std::string() : i->second;
		}
		bool HasValue(const std::string &key) const { return Values.find(key) != Values.end(); }

		void SetValue(const std::string &key, const std::string &text) { Values[key] = text; }

		PropertyDescription &AddProperty(const std::string &name, const std::string &type,
		                                 const std::string &className)
		{
			Properties.push_back(PropertyDescription(name, type, className));
			return Properties.back();
		}

		const PropertyDescription *FindProperty(const std::string &name) const
		{
			for (size_t i = 0; i < Properties.size(); i++)
				if (Properties[i].Name == name) return &Properties[i];

			return 0;
		}
		PropertyDescription *FindProperty(const std::string &name)
		{
			for (size_t i = 0; i < Properties.size(); i++)
				if (Properties[i].Name == name) return &Properties[i];

			return 0;
		}


		void ClearProperties() { Properties.clear(); }
	};

	class WireDescription
	{
	public:
		std::string FromDevice;
		std::string FromPort;
		std::string ToDevice;
		std::string ToPort;
	};

	class ObjectDescription
	{
	public:
		std::map<std::string, std::string> Values;

		std::vector<DeviceDescription> Devices;

		std::string Value(const std::string &key) const
		{
			std::map<std::string, std::string>::const_iterator i = Values.find(key);
			return i == Values.end() ? std::string() : i->second;
		}

		void SetValue(const std::string &key, const std::string &text) { Values[key] = text; }

		DeviceDescription &AddDevice()
		{
			Devices.push_back(DeviceDescription());
			return Devices.back();
		}

		bool IsComment() const { return Devices.empty(); }
	};

	class PatchDescription
	{
	public:
		std::vector<ObjectDescription> Objects;
		std::vector<WireDescription> Wires;

		std::map<std::string, std::string> Values;

		const DeviceDescription *FindDevice(const std::string &patchId) const
		{
			for (size_t o = 0; o < Objects.size(); o++)
				for (size_t d = 0; d < Objects[o].Devices.size(); d++)
					if (Objects[o].Devices[d].Value("patch_id") == patchId)
						return &Objects[o].Devices[d];

			return 0;
		}

		ObjectDescription &AddObject()
		{
			Objects.push_back(ObjectDescription());
			return Objects.back();
		}

		WireDescription &AddWire()
		{
			Wires.push_back(WireDescription());
			return Wires.back();
		}

		void Clear()
		{
			Objects.clear();
			Wires.clear();
			Values.clear();
		}

		void Swap(PatchDescription &other)
		{
			Objects.swap(other.Objects);
			Wires.swap(other.Wires);
			Values.swap(other.Values);
		}
	};
}
}

#endif
