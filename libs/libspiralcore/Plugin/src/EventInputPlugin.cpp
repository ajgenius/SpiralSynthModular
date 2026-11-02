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

#include "EventInputPlugin.h"
#include "EventBuffer.h"
#include "EventInputIcon.xpm"

using Spiral::Core::Event;
using Spiral::Core::EventBuffer;

static SpiralPlugin *CreateInstance() { return new EventInputPlugin; }

const Spiral::Core::DeviceClass &EventInputPlugin::Class()
{
	static Spiral::Core::DeviceClass device;
	device.ID = 289;
	device.Name = "Event Input";
	device.Category = "Control";
	device.Icon = SpiralIcon_xpm;
	device.Create = CreateInstance;
	return device;
}

EventInputPlugin::EventInputPlugin() :
m_First(NULL),
m_Voice(0),
m_Count(1),
m_Source(NULL),
m_ResetEvents(false)
{
	m_PluginInfo.Name="Event Input";
	m_PluginInfo.Width=60;
	m_PluginInfo.Height=40;
	m_PluginInfo.NumInputs=1;
	m_PluginInfo.NumOutputs=3;
	m_PluginInfo.PortTips.push_back("Events");
	m_PluginInfo.PortTips.push_back("Events");
	m_PluginInfo.PortTips.push_back("Pitch");
	m_PluginInfo.PortTips.push_back("Trigger");
}

PluginInfo &EventInputPlugin::Initialise(const HostInfo *Host)
{
	PluginInfo &Info = SpiralPlugin::Initialise(Host);
	SetInPortType(m_PluginInfo, 0, Sample::EVENT);
	SetOutPortType(m_PluginInfo, 0, Sample::EVENT);
	return Info;
}

void EventInputPlugin::SetVoice(SpiralPlugin *first, unsigned voice, unsigned voices)
{
	m_First = voice ? dynamic_cast<EventInputPlugin *>(first) : NULL;
	m_Voice = m_First ? voice : 0;
	m_Count = voices;
}

void EventInputPlugin::Reset()
{
	SpiralPlugin::Reset();
	m_Voices.Reset();
	m_ResetEvents = m_ResetEvents || m_Source != NULL;
	m_Source = NULL;
}

void EventInputPlugin::Execute()
{
	const unsigned samples = m_HostInfo->BUFSIZE;
	EventBuffer &output = *GetEventOutput(0);

	if (m_First)
	{
		output = *m_First->GetEventOutput(0);
		const bool lane = m_Voice < m_First->m_Voices.Count() && m_First->m_Pitch.size() >= (m_Voice + 1) * samples;
		for (unsigned n = 0; n < samples; n++)
		{
			SetOutputPitch(1, n, lane ? m_First->m_Pitch[m_Voice * samples + n] : 0);
			SetOutput(2, n, lane ? m_First->m_Gate[m_Voice * samples + n] : 0);
		}
		return;
	}

	m_Voices.Configure(m_Count);
	const EventBuffer *input = GetEventInput(0);
	if (m_Source && input != m_Source)
	{
		m_Voices.Reset();
		m_ResetEvents = true;
	}
	m_Source = input;

	output.Clear(samples);
	if (m_ResetEvents && samples)
	{
		Event reset;
		reset.Type = Event::Reset;
		output.Append(0, reset);
		m_ResetEvents = false;
	}
	for (unsigned i = 0; input && i < input->Count(); ++i)
		if (!output.Append(input->Frame(i), input->At(i)))
		{
			m_Voices.Reset();
			input = NULL;
			break;
		}

	m_Pitch.resize(m_Voices.Count() * samples);
	m_Gate.resize(m_Voices.Count() * samples);
	unsigned next = 0;
	for (unsigned n = 0; n < samples; n++)
	{
		while (input && next < input->Count() && input->Frame(next) <= n)
			m_Voices.Apply(input->At(next++));
		for (unsigned voice = 0; voice < m_Voices.Count(); ++voice)
		{
			m_Pitch[voice * samples + n] = m_Voices.Pitch(voice);
			m_Gate[voice * samples + n] = m_Voices.Gate(voice);
		}
		SetOutputPitch(1, n, m_Pitch[n]);
		SetOutput(2, n, m_Gate[n]);
	}
}
