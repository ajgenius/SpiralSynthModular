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

	class PatchDescription;

	class PortBind
	{
	public:
		bool OuterIsInput;
		unsigned OuterIndex;
		std::string InnerPath;

		PortBind(): OuterIsInput(true), OuterIndex(0) {}
		PortBind(bool input, unsigned index, const std::string &path):
			OuterIsInput(input), OuterIndex(index), InnerPath(path) {}
	};

	class PortName
	{
	public:
		std::string Name;
		std::string Type;

		PortName() {}
		PortName(const std::string &name, const std::string &type = std::string()): Name(name), Type(type) {}
	};

	class SubPatchConfig
	{
	public:
		std::string Mode;
		long Voices;
		bool PolyOutput;
		std::string Link;
		long BufferFrames;
		std::string LinkName;

		SubPatchConfig(): Mode("Virtual"), Voices(1), PolyOutput(false), Link("Buffer"), BufferFrames(0) {}

		// The spellings the file uses, as the private tree reads and writes
		// them: Virtual, Inline or Isolated; Buffer or Jack. Anything else
		// reads as Virtual, and as Buffer.
		static std::string ModeSpelling(const std::string &text)
		{
			if (text == "Inline" || text == "inline")
				return "Inline";
			if (text == "Isolated" || text == "isolated")
				return "Isolated";
			return "Virtual";
		}
		static std::string LinkSpelling(const std::string &text)
		{
			return text == "Jack" || text == "jack" ? "Jack" : "Buffer";
		}
	};

	class DeviceDescription
	{
	public:
		std::map<std::string, std::string> Values;

		std::vector<PropertyDescription> Properties;

		SubPatchConfig Config;
		std::vector<PortBind> Binds;
		std::vector<PortName> Inputs, Outputs;

		DeviceDescription(): m_Inner(0) {}
		DeviceDescription(const DeviceDescription &other);
		DeviceDescription &operator=(const DeviceDescription &other);
		~DeviceDescription();

		bool IsSubPatch() const { return m_Inner != 0; }
		PatchDescription &MakeSubPatch();
		PatchDescription *Inner() { return m_Inner; }
		const PatchDescription *Inner() const { return m_Inner; }

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

	private:
		PatchDescription *m_Inner;
	};

	class WireDescription
	{
	public:
		std::string FromDevice;
		std::string FromPort;
		std::string ToDevice;
		std::string ToPort;
	};

	class StateValue
	{
	public:
		enum Kind { Number, Boolean, String };
		Kind Type;
		std::string Text;

		StateValue(): Type(String) {}
		StateValue(Kind type, const std::string &text): Type(type), Text(text) {}
	};

	class ObjectDescription
	{
	public:
		std::map<std::string, std::string> Values;
		std::map<std::string, StateValue> State;

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

	// A control the patch's MIDI input moves: a controller number on a
	// channel from 1 to 16, or on any channel when Channel is 0.
	class MidiBindingDescription
	{
	public:
		std::string Control;
		long Channel;
		long Controller;

		MidiBindingDescription(): Channel(0), Controller(0) {}
		MidiBindingDescription(const std::string &control, long channel, long controller):
			Control(control), Channel(channel), Controller(controller) {}
	};

	// A device the patch's MIDI map lists, by patch id, and its bound
	// controls. A device is listed from when it is added, bound or not.
	class MidiDeviceDescription
	{
	public:
		std::string Device;
		std::vector<MidiBindingDescription> Bindings;

		MidiDeviceDescription() {}
		explicit MidiDeviceDescription(const std::string &device): Device(device) {}
	};

	class PatchDescription
	{
	public:
		std::vector<ObjectDescription> Objects;
		std::vector<WireDescription> Wires;

		std::map<std::string, std::string> Values;

		std::vector<MidiDeviceDescription> Midi;

		// The patch's own voice pool, as the private tree saves it; zero
		// inherits the system default.
		unsigned Voices;

		PatchDescription(): Voices(0) {}

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
			Midi.clear();
			Voices = 0;
		}

		void Swap(PatchDescription &other)
		{
			Objects.swap(other.Objects);
			Wires.swap(other.Wires);
			Values.swap(other.Values);
			Midi.swap(other.Midi);
			const unsigned voices = Voices;
			Voices = other.Voices;
			other.Voices = voices;
		}
	};

	inline DeviceDescription::DeviceDescription(const DeviceDescription &other):
		Values(other.Values), Properties(other.Properties), Config(other.Config), Binds(other.Binds),
		Inputs(other.Inputs), Outputs(other.Outputs),
		m_Inner(other.m_Inner ? new PatchDescription(*other.m_Inner) : 0)
	{
	}

	inline DeviceDescription &DeviceDescription::operator=(const DeviceDescription &other)
	{
		if (this == &other)
			return *this;
		PatchDescription *inner = other.m_Inner ? new PatchDescription(*other.m_Inner) : 0;
		delete m_Inner;
		m_Inner = inner;
		Values = other.Values;
		Properties = other.Properties;
		Config = other.Config;
		Binds = other.Binds;
		Inputs = other.Inputs;
		Outputs = other.Outputs;
		return *this;
	}

	inline DeviceDescription::~DeviceDescription()
	{
		delete m_Inner;
	}

	inline PatchDescription &DeviceDescription::MakeSubPatch()
	{
		if (!m_Inner)
			m_Inner = new PatchDescription();
		return *m_Inner;
	}
}
}

#endif
