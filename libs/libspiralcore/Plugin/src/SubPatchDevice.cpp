// SPDX-License-Identifier: GPL-2.0-or-later
#include "SubPatchDevice.h"

using namespace Spiral::Core;

static SpiralPlugin *CreateInstance() { return new SubPatchDevice; }

const DeviceClass &SubPatchDevice::Class()
{
	static DeviceClass device;
	device.ID = 290;
	device.Name = "SubPatch";
	device.Category = "SubPatch";
	device.Create = CreateInstance;
	device.PanelGenerated = true;
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
	int mode = 0, link = 0, frames = 0, voices = 1;
	bool poly = false;
	m_AudioCH->Peek("Mode", 0, &mode, sizeof(mode));
	m_AudioCH->Peek("Link", 0, &link, sizeof(link));
	m_AudioCH->Peek("Buffer Frames", 0, &frames, sizeof(frames));
	m_AudioCH->Peek("Voices", 0, &voices, sizeof(voices));
	m_AudioCH->Peek("Poly Output", 0, &poly, sizeof(poly));
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
	m_AudioCH->SetDataAt("Mode", 0, &mode, sizeof(mode));
	m_AudioCH->SetDataAt("Link", 0, &link, sizeof(link));
	m_AudioCH->SetDataAt("Buffer Frames", 0, &frames, sizeof(frames));
	m_AudioCH->SetDataAt("Voices", 0, &voices, sizeof(voices));
	m_AudioCH->SetDataAt("Poly Output", 0, &poly, sizeof(poly));
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
			m_PluginInfo.PortTypes.push_back(ports[k].IsEvent() ? Sample::EVENT : Sample::AUDIO);
		}
	}
}

bool SubPatchDevice::GeneratePanel(const std::string &, std::string &Document)
{
	Document =
		"{\"panel\":{\"schema_version\":1,\"width\":400,\"height\":134,"
		"\"help\":\"Virtual shares the parent graph. Inline owns a graph on the parent audio thread. "
		"Isolated owns an audio thread and communicates through internal buffers or JACK; it is not here yet. "
		"Voices are copies of the subpatch's devices, one per note; zero inherits the parent's voices.\","
		"\"children\":["
		"{\"type\":\"Label\",\"bounds\":[4,4,134,22],\"text\":\"Mode\"},"
		"{\"type\":\"Choice\",\"name\":\"MODE\",\"bounds\":[140,4,256,22],\"property\":\"MODE\",\"items\":["
		"{\"value\":0,\"text\":\"Virtual\"},{\"value\":1,\"text\":\"Inline\"},{\"value\":2,\"text\":\"Isolated\"}]},"
		"{\"type\":\"Label\",\"bounds\":[4,30,134,22],\"text\":\"Transport\"},"
		"{\"type\":\"Choice\",\"name\":\"LINK\",\"bounds\":[140,30,256,22],\"property\":\"LINK\",\"enabled\":false,\"items\":["
		"{\"value\":0,\"text\":\"Internal buffers\"},{\"value\":1,\"text\":\"JACK\"}]},"
		"{\"type\":\"Label\",\"bounds\":[4,56,134,22],\"text\":\"Buffer frames\"},"
		"{\"type\":\"Spinner\",\"name\":\"BUFFER FRAMES\",\"bounds\":[140,56,256,22],\"property\":\"BUFFER FRAMES\",\"enabled\":false},"
		"{\"type\":\"Label\",\"bounds\":[4,82,134,22],\"text\":\"Voices (0: parent)\"},"
		"{\"type\":\"Spinner\",\"name\":\"VOICES\",\"bounds\":[140,82,256,22],\"property\":\"VOICES\","
		"\"enabled\":{\"property\":\"MODE\",\"not_equals\":0}},"
		"{\"type\":\"CheckBox\",\"name\":\"POLY OUTPUT\",\"bounds\":[140,108,256,22],\"property\":\"POLY OUTPUT\","
		"\"text\":\"Poly output\",\"enabled\":false}]}}";
	return true;
}
