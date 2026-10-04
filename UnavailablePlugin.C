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
	int n;

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
	for (n = 0; n < m_PluginInfo.NumInputs; n++)
		m_PluginInfo.PortTips.push_back("Unavailable input");
	for (n = 0; n < m_PluginInfo.NumOutputs; n++)
		m_PluginInfo.PortTips.push_back("Unavailable output");

	m_PluginInfo.PortTypes.clear();
	for (n = 0; n < m_PluginInfo.NumInputs + m_PluginInfo.NumOutputs; n++)
		m_PluginInfo.PortTypes.push_back(0);

	UpdatePluginInfoWithHost();
}

void UnavailablePlugin::ApplyKnownLayout()
{
	int version = 1;
	int inputs = 16;
	int outputs = 16;
	spiralcore::Description::Reader in(m_State);

	// JackPlugin::Apply. LADSPA's numbers are control-port limits, not
	// the canvas port count, so those stay at whatever the wires require.
	if (m_PluginId != JACK_PLUGIN_ID)
		return;

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
	int n;

	// Same as Testing's UnavailableDevice::Process: a missing plugin
	// writes silence. It does not copy audio across the hole.
	for (n = 0; n < m_PluginInfo.NumOutputs; n++)
	{
		Sample *out = GetOutputBuf(n);
		if (out)
			out->Zero();
	}
}

void UnavailablePlugin::Describe(spiralcore::Description &d)
{
	// The state as the host found it: every value, every gap.
	const vector<string> &values = m_State.Values();
	const vector<string> &between = m_State.Between();
	d.Separator(between[0].c_str());
	for (size_t i = 0; i < values.size(); ++i)
		d.Value(values[i]).Separator(between[i + 1].c_str());
}

void UnavailablePlugin::Apply(spiralcore::Description::Reader &r)
{
	r.Rest(m_State);
	ApplyKnownLayout();
}
