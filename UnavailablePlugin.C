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
	int fallback = -1;
	int frame;
	int length;

	// Carry the signal across the hole. A same-numbered input wins; if
	// that input is not connected (LADSPA audio is often not port 0),
	// use the first connected input. Outputs with nothing to copy are
	// silence.
	for (n = 0; n < m_PluginInfo.NumInputs; n++)
	{
		if (GetInput(n))
		{
			fallback = n;
			break;
		}
	}

	for (n = 0; n < m_PluginInfo.NumOutputs; n++)
	{
		Sample *out = GetOutputBuf(n);
		int inIndex = fallback;
		const Sample *in;

		if (!out)
			continue;
		if (n < m_PluginInfo.NumInputs && GetInput(n))
			inIndex = n;
		in = (inIndex >= 0) ? GetInput(inIndex) : NULL;
		if (!in)
		{
			out->Zero();
			continue;
		}

		length = out->GetLength();
		if (in->GetLength() < length)
			length = in->GetLength();
		for (frame = 0; frame < length; frame++)
			out->Set(frame, (*in)[frame]);
		for (; frame < out->GetLength(); frame++)
			out->Set(frame, 0);
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
