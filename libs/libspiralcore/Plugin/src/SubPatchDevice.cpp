// SPDX-License-Identifier: GPL-2.0-or-later
#include "SubPatchDevice.h"

using namespace Spiral::Core;

static SpiralPlugin *CreateInstance() { return new SubPatchDevice; }

// No "id" is written for a subpatch, so this ID is the host's alone.
const DeviceClass &SubPatchDevice::Class()
{
	static DeviceClass device;
	device.ID = 290;
	device.Name = "SubPatch";
	device.Category = "SubPatch";
	device.Create = CreateInstance;
	return device;
}

namespace
{
	const char *const Modes[] = {"Virtual", "Inline", "Isolated"};
	const char *const Links[] = {"Buffer", "Jack"};

	int IndexOf(const char *const names[], int count, const std::string &name)
	{
		for (int i = 0; i < count; ++i)
			if (name == names[i])
				return i;
		return 0;
	}
}

SubPatchDevice::SubPatchDevice(): m_Step(NULL), m_Host(NULL)
{
	const NamedPropertyValue modes[] = {
		{"Virtual", DefaultUnsigned(0)},
		{"Inline", DefaultUnsigned(1)},
		{"Isolated", DefaultUnsigned(2)}};
	const NamedPropertyValue links[] = {
		{"Buffer", DefaultUnsigned(0)},
		{"Jack", DefaultUnsigned(1)}};

	// The private tree's properties, flags and ranges.
	m_Mode = SetProperty(Property::WriteOnly | Property::ForceUpdate, 0, NamedPropertySet(modes, 3));
	m_Link = SetProperty(Property::WriteOnly | Property::ForceUpdate, 0, NamedPropertySet(links, 2));
	m_BufferFrames = UnsignedProperty(DefaultLinearFlags | Property::ForceUpdate, 0, 0, 65536, 1, 64);
	m_LinkName = StringProperty(Property::WriteOnly | Property::ForceUpdate, "");
	m_Voices = UnsignedProperty(DefaultLinearFlags | Property::ForceUpdate, 1, 0, 256, 1, 1);
	m_PolyOutput = ToggleProperty(Property::WriteOnly | Property::ForceUpdate, false);

	RegisterSharedProperty(m_Mode, "Mode", "How the inner patch runs");
	RegisterSharedProperty(m_Link, "Link", "Isolated: how it talks to this patch");
	RegisterSharedProperty(m_BufferFrames, "Buffer Frames", "Buffer pipeline latency, rounded up to whole blocks; zero selects two blocks");
	RegisterSharedProperty(m_LinkName, "Link Name", "Isolated, Jack link: the client name");
	RegisterSharedProperty(m_Voices, "Voices", "Instrument voice pool; zero retains inherited legacy voices; virtual groups inherit");
	RegisterSharedProperty(m_PolyOutput, "Poly Output", "Keep audio output voice lanes instead of summing them");

	SetString(DeviceName, "SubPatch");
	m_PluginInfo.Name = "SubPatch";
	m_PluginInfo.NumInputs = 0;
	m_PluginInfo.NumOutputs = 0;
}

SubPatchConfig SubPatchDevice::Settings()
{
	SubPatchConfig config;
	const int mode = (int)AsUnsigned(m_Mode), link = (int)AsUnsigned(m_Link);
	const int frames = (int)AsUnsigned(m_BufferFrames), voices = (int)AsUnsigned(m_Voices);
	const bool poly = AsBoolean(m_PolyOutput);
	config.Mode = Modes[mode >= 0 && mode < 3 ? mode : 0];
	config.Link = Links[link >= 0 && link < 2 ? link : 0];
	config.BufferFrames = frames;
	config.Voices = voices;
	config.PolyOutput = poly;
	config.LinkName = m_LinkName.Value;
	return config;
}

void SubPatchDevice::SetConfig(const SubPatchConfig &config)
{
	m_Config = config;
	m_Config.Mode = SubPatchConfig::ModeSpelling(config.Mode);
	m_Config.Link = SubPatchConfig::LinkSpelling(config.Link);
	const int mode = IndexOf(Modes, 3, SubPatchConfig::ModeSpelling(config.Mode));
	const int link = IndexOf(Links, 2, SubPatchConfig::LinkSpelling(config.Link));
	const int frames = (int)config.BufferFrames, voices = (int)config.Voices;
	const bool poly = config.PolyOutput;
	AsUnsigned(m_Mode) = mode;
	AsUnsigned(m_Link) = link;
	AsUnsigned(m_BufferFrames) = frames;
	AsUnsigned(m_Voices) = voices;
	AsBoolean(m_PolyOutput) = poly;
	m_LinkName.Value = config.LinkName;
}

void SubPatchDevice::Execute()
{
	RunMembers();
}

void SubPatchDevice::Reset()
{
	RunMembers();
}

void SubPatchDevice::RunMembers()
{
	if (!m_Step)
		return;
	const std::list<int> &order = m_Graph.GetSortedList();
	for (std::list<int>::const_reverse_iterator i = order.rbegin(); i != order.rend(); ++i)
		m_Step(*i, m_Host);
}

bool SubPatchDevice::TakeSettings()
{
	SubPatchConfig wanted = Settings();
	const bool putBack = wanted.Mode == "Isolated" && m_Config.Mode != "Isolated";
	if (putBack)
		wanted.Mode = m_Config.Mode;
	const bool changed = wanted.Mode != m_Config.Mode || wanted.Voices != m_Config.Voices
		|| wanted.Link != m_Config.Link || wanted.BufferFrames != m_Config.BufferFrames
		|| wanted.PolyOutput != m_Config.PolyOutput || wanted.LinkName != m_Config.LinkName;
	if (putBack)
		SetConfig(wanted);
	else
		m_Config = wanted;
	return changed;
}

void SubPatchDevice::PortsChanged()
{
	m_PluginInfo.NumInputs = (int)Inputs.size();
	m_PluginInfo.NumOutputs = (int)Outputs.size();
	m_PluginInfo.PortTips.clear();
	m_PluginInfo.PortTypes.clear();
	for (int input = 1; input >= 0; --input)
	{
		const std::vector<BoundaryPort> &ports = input ? Inputs : Outputs;
		for (size_t k = 0; k < ports.size(); ++k)
		{
			m_PluginInfo.PortTips.push_back(ports[k].Name);
			m_PluginInfo.PortTypes.push_back(0);
		}
	}
}
