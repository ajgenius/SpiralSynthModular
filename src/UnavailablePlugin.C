#include "UnavailablePlugin.h"

#include <iostream>

using namespace std;

static const int JACK_PLUGIN_ID = 31;

UnavailablePlugin::UnavailablePlugin(int pluginId):
	m_PluginId(pluginId)
{
	m_PluginInfo.Name = "Unavailable";
	m_PluginInfo.Width = 40;
	m_PluginInfo.Height = 50;
	m_PluginInfo.NumInputs = 0;
	m_PluginInfo.NumOutputs = 0;
}

string UnavailablePlugin::SavedName() const
{
	if (!m_SavedName.empty())
		return m_SavedName;

	return m_PluginInfo.Name;
}

void UnavailablePlugin::EnsurePorts(int inputs, int outputs)
{
	if (!m_HostInfo)
		return;

	if (inputs < m_PluginInfo.NumInputs)
		inputs = m_PluginInfo.NumInputs;

	if (outputs < m_PluginInfo.NumOutputs)
		outputs = m_PluginInfo.NumOutputs;

	if (inputs > 4096)
		inputs = 4096;

	if (outputs > 4096)
		outputs = 4096;

	if (inputs == m_PluginInfo.NumInputs && outputs == m_PluginInfo.NumOutputs)
		return;

	while (m_PluginInfo.NumInputs < inputs)
	{
		AddInput();
		m_PluginInfo.NumInputs++;
	}

	while (m_PluginInfo.NumOutputs < outputs)
	{
		AddOutput();
		m_PluginInfo.NumOutputs++;
	}

	m_PluginInfo.PortTips.clear();
	m_PluginInfo.PortTypes.assign(inputs + outputs, Sample::AUDIO);
	for (int direction = 0; direction < 2; ++direction)
	{
		const bool input = direction == 0;
		const std::vector<Spiral::Core::PortName> &ports = input ? m_Definition.Inputs : m_Definition.Outputs;
		for (int i = 0; i < (input ? inputs : outputs); ++i)
		{
			const bool known = size_t(i) < ports.size();
			m_PluginInfo.PortTips.push_back(known ? ports[i].Name : input ? "Unavailable input" : "Unavailable output");
			const Sample::SampleType type = known && ports[i].Type == "MusicalEventPort" ? Sample::EVENT
				: known && ports[i].Type == "MidiEventPort" ? Sample::MIDI : Sample::AUDIO;
			if (input)
				SetInPortType(m_PluginInfo, i, type);
			else
				SetOutPortType(m_PluginInfo, i, type);
		}
	}

	UpdatePluginInfoWithHost();
}

void UnavailablePlugin::ApplyKnownLayout()
{
	// JackPlugin::Apply. LADSPA's numbers are control-port limits, not
	// the canvas port count, so those stay at whatever the wires require.
	if (m_PluginId != JACK_PLUGIN_ID)
		return;

	int version = 1;
	int inputs = 16;
	int outputs = 16;
	Spiral::Core::Description::Reader in(m_State);

	if (in.More())
		in.Value(version);

	if (version == 2)
	{
		in.Value(inputs).Value(outputs);

		if (in.Failed())
		{
			inputs = 16;
			outputs = 16;
		}
	}

	if (inputs < 2)
		inputs = 2;

	if (outputs < 2)
		outputs = 2;

	if (inputs > 64)
		inputs = 64;

	if (outputs > 64)
		outputs = 64;

	EnsurePorts(inputs, outputs);
}

void UnavailablePlugin::Execute()
{
	// Same as Testing's UnavailableDevice::Process: a missing plugin
	// writes silence. It does not copy audio across the hole.
	for (int n = 0; n < m_PluginInfo.NumOutputs; n++)
	{
		Sample *out = GetOutputBuf(n);

		if (out)
			out->Zero();
	}
}

void UnavailablePlugin::Describe(Spiral::Core::Description &d)
{
	d = m_State;
}

void UnavailablePlugin::Apply(Spiral::Core::Description::Reader &r)
{
	r.Rest(m_State);
	ApplyKnownLayout();
}

void UnavailablePlugin::SetDefinition(const Spiral::Core::DeviceDescription &device)
{
	m_Definition = device;
	SetSavedName(device.Value("name"));
	EnsurePorts(device.Inputs.size(), device.Outputs.size());
}
